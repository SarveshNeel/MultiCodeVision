#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>

#include <ZXing/ReadBarcode.h>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/ReaderOptions.h>
#include <ZXing/ImageView.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>

namespace fs = std::filesystem;

struct QRResult {
    std::string text;
    std::vector<cv::Point2f> corners;
};

static ZXing::ImageView to_zxing_imageview(const cv::Mat& img, cv::Mat& gray_out)
{
    if (img.channels() == 3)
        cv::cvtColor(img, gray_out, cv::COLOR_BGR2GRAY);
    else
        gray_out = img;

    return ZXing::ImageView(gray_out.data, gray_out.cols, gray_out.rows, ZXing::ImageFormat::Lum);
}

static cv::Mat warp_qr_patch(const cv::Mat& img, const std::vector<cv::Point2f>& corners, int out_size = 256)
{
    if (corners.size() != 4) return cv::Mat();

    std::vector<cv::Point2f> src = corners;
    std::vector<cv::Point2f> dst = {
        {0.f, 0.f},
        {static_cast<float>(out_size - 1), 0.f},
        {static_cast<float>(out_size - 1), static_cast<float>(out_size - 1)},
        {0.f, static_cast<float>(out_size - 1)}
    };

    cv::Mat H = cv::getPerspectiveTransform(src, dst);
    cv::Mat warped;
    cv::warpPerspective(img, warped, H, cv::Size(out_size, out_size), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
    return warped;
}

static std::string decode_with_fallback(const cv::Mat& full_img,
                                        const std::vector<cv::Point2f>& corners)
{
    // 1) Fast path: padded ROI decode
    cv::Rect roi = cv::boundingRect(corners);
    int pad = 8;
    roi.x -= pad; roi.y -= pad;
    roi.width += 2 * pad; roi.height += 2 * pad;
    roi &= cv::Rect(0, 0, full_img.cols, full_img.rows);

    if (roi.width > 0 && roi.height > 0) {
        cv::Mat crop = full_img(roi).clone();
        cv::QRCodeDetector d;
        std::string txt = d.detectAndDecode(crop);
        if (!txt.empty()) return txt;
    }

    // 2) Robust path: perspective-normalized patch
    cv::Mat warped = warp_qr_patch(full_img, corners, 320);
    if (!warped.empty()) {
        cv::QRCodeDetector d;
        std::string txt = d.detectAndDecode(warped);
        if (!txt.empty()) return txt;
    }

    // 3) Last try: grayscale + threshold on warped patch
    if (!warped.empty()) {
        cv::Mat gray, bw;
        cv::cvtColor(warped, gray, cv::COLOR_BGR2GRAY);
        cv::adaptiveThreshold(gray, bw, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                              cv::THRESH_BINARY, 31, 3);
        cv::QRCodeDetector d;
        std::string txt = d.detectAndDecode(bw);
        if (!txt.empty()) return txt;
    }

    return std::string();
}

static bool has_image_extension(const fs::path& p) {
    const std::string ext = p.extension().string();
    if (ext.empty()) return false;
    std::string e;
    e.reserve(ext.size());
    for (char c : ext) e.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return e == ".jpg" || e == ".jpeg" || e == ".png" || e == ".bmp" || e == ".tif" || e == ".tiff" || e == ".webp";
}

// ZXing-based detector/decoder.
// More robust than OpenCV detectMulti() for small and rotated QR codes.
std::vector<QRResult> detect_and_decode_parallel(const cv::Mat& img)
{
    std::vector<QRResult> results;

    // ----- ZXing detection + decode (single pass, robust for small/rotated QR) -----
    // Allow more symbols per frame (we expect up to 24+ codes)
    ZXing::ReaderOptions hints;
    hints.setFormats(ZXing::BarcodeFormat::QRCode);
    hints.setTryHarder(true);
    hints.setTryRotate(true);
    hints.setTryInvert(true);
    hints.setMaxNumberOfSymbols(40);
    hints.setBinarizer(ZXing::Binarizer::LocalAverage);

    // cv::Mat gray;
    // auto iv = to_zxing_imageview(img, gray);
    // auto zresults = ZXing::ReadBarcodes(iv, hints);

    cv::Mat gray;
    if (img.channels() == 3)
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    else
        gray = img;

    // preprocessing + multi-scale passes
    struct ZXPass {
        cv::Mat img;
        // Factor to convert coordinates from this pass back to the original image
        float coordScale;
    };

    std::vector<ZXPass> passes;
    passes.push_back({gray, 1.0f});

    // CLAHE (VERY important for shiny cans)
    cv::Mat claheImg;
    {
        auto clahe = cv::createCLAHE(2.5, cv::Size(8,8));
        clahe->apply(gray, claheImg);
        passes.push_back({claheImg, 1.0f});
    }

    // adaptive threshold
    cv::Mat bw;
    cv::adaptiveThreshold(gray, bw, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY, 31, 3);
    passes.push_back({bw, 1.0f});

    // Upscaled CLAHE pass to help with very small / distant QRs
    {
        cv::Mat srcForUpscale = claheImg.empty() ? gray : claheImg;
        cv::Mat upScaled;
        double upscale = 2.0;
        cv::resize(srcForUpscale, upScaled, cv::Size(), upscale, upscale, cv::INTER_CUBIC);
        // Coordinates from this pass need to be scaled back down
        passes.push_back({upScaled, static_cast<float>(1.0 / upscale)});
    }

    auto center_of = [](const QRResult& q) {
        cv::Point2f c(0.f, 0.f);
        if (q.corners.size() != 4)
            return c;
        for (const auto& p : q.corners)
            c += p;
        c *= 0.25f;
        return c;
    };

    auto is_duplicate = [&](const QRResult& a, const QRResult& b) {
        // Prefer exact text match when available
        if (!a.text.empty() && !b.text.empty() && a.text == b.text)
            return true;

        // Fallback: same spatial location (helps when text decode differs across passes)
        if (a.corners.size() == 4 && b.corners.size() == 4) {
            const cv::Point2f ca = center_of(a);
            const cv::Point2f cb = center_of(b);
            return cv::norm(ca - cb) < 18.0f;
        }
        return false;
    };

    // if (zresults.empty())
    //     return results;

    // results.reserve(zresults.size());

    // for (const auto& zr : zresults)
    for (const auto& pass : passes)
    {
        cv::Mat tmp;
        auto iv = to_zxing_imageview(pass.img, tmp);
        auto zresults = ZXing::ReadBarcodes(iv, hints);

        for (const auto& zr : zresults)
        {
            if (!zr.isValid())
                continue;

            QRResult r;
            r.text = zr.text();

            // Debug: classify ZXing result state
            // detected + decoded  => text non-empty
            // detected but decode failed => text empty
            if (r.text.empty()) {
                std::cout << "[ZXing] DETECTED (decode FAILED)" << std::endl;
            } else {
                std::cout << "[ZXing] DETECTED + DECODED" << std::endl;
            }

            auto pos = zr.position();
            const float s = pass.coordScale;
            r.corners.push_back(cv::Point2f(pos.topLeft().x * s, pos.topLeft().y * s));
            r.corners.push_back(cv::Point2f(pos.topRight().x * s, pos.topRight().y * s));
            r.corners.push_back(cv::Point2f(pos.bottomRight().x * s, pos.bottomRight().y * s));
            r.corners.push_back(cv::Point2f(pos.bottomLeft().x * s, pos.bottomLeft().y * s));

            // Fallback for difficult symbols (includes upscaled retry)
            if (r.text.empty() && r.corners.size() == 4) {
                r.text = decode_with_fallback(img, r.corners);
            }

            bool dup = false;
            for (const auto& existing : results) {
                if (is_duplicate(existing, r)) {
                    dup = true;
                    break;
                }
            }
            if (!dup && !r.text.empty())
                results.push_back(std::move(r));
        }
    }

    return results;
}

static void process_image_with_zxing(const fs::path& imagePath, bool showWindow)
{
    cv::Mat img = cv::imread(imagePath.string(), cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "[ERROR] Failed to load image: " << imagePath << std::endl;
        return;
    }

    using clock = std::chrono::steady_clock;
    const auto t0 = clock::now();
    auto results = detect_and_decode_parallel(img);
    const auto t1 = clock::now();
    const auto decode_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "\n=== " << imagePath.filename().string() << " ===" << std::endl;
    if (results.empty()) {
        std::cout << "No QR codes detected." << std::endl;
    } else {
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "QR " << i << ": " << results[i].text << std::endl;

            std::vector<cv::Point> poly;
            for (const auto& p : results[i].corners)
                poly.emplace_back(cvRound(p.x), cvRound(p.y));

            if (poly.size() >= 4)
                cv::polylines(img, poly, true, {0,255,0}, 2);
        }
    }

    std::cout << "Decode + Result Generation Time: " << decode_ms << " ms" << std::endl;

    if (showWindow) {
        cv::imshow("QR Decode - " + imagePath.filename().string(), img);
        cv::waitKey(0);
        cv::destroyAllWindows();
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage:\n"
                  << "  ./app <image_path> [--batch]\n"
                  << "  ./app <directory_path> [--batch]\n"
                  << "\nOptions:\n"
                  << "  --batch   Process without opening preview windows\n";
        return 1;
    }

    fs::path input = argv[1];

    bool showWindow = true;
    if (argc >= 3) {
        std::string opt = argv[2];
        if (opt == "--batch")
            showWindow = false;
    }

    if (!fs::exists(input)) {
        std::cerr << "[ERROR] Path does not exist: " << input << std::endl;
        return 1;
    }

    if (fs::is_regular_file(input)) {
        process_image_with_zxing(input, showWindow);
        return 0;
    }

    if (fs::is_directory(input)) {
        for (const auto& entry : fs::directory_iterator(input)) {
            if (!entry.is_regular_file())
                continue;
            if (!has_image_extension(entry.path()))
                continue;

            process_image_with_zxing(entry.path(), showWindow);
        }
        return 0;
    }

    std::cerr << "[ERROR] Unsupported input type." << std::endl;
    return 1;
}