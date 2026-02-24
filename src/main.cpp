#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>

#include <filesystem>
#include <future>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

namespace fs = std::filesystem;

struct QRResult {
    std::string text;
    std::vector<cv::Point2f> corners;
};

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

std::vector<QRResult> detect_and_decode_parallel(const cv::Mat& img)
{
    cv::QRCodeDetector detector;

    cv::Mat points;

    // -------- STAGE 1: detect --------
    bool found = detector.detectMulti(img, points);

    std::vector<QRResult> results;
    if (!found || points.empty())
        return results;

    int n_qr = points.rows;

    std::vector<std::future<QRResult>> futures;

    // -------- STAGE 2+3: ROI + parallel decode --------
    for (int i = 0; i < n_qr; ++i)
    {
        futures.push_back(
            std::async(std::launch::async,
                [&, i]() -> QRResult {

                    QRResult r;

                    std::vector<cv::Point2f> poly;
                    for (int j = 0; j < points.cols; ++j)
                    {
                        poly.push_back(points.at<cv::Point2f>(i, j));
                    }

                    r.corners = poly;

                    // Decode using ROI + perspective-warp fallback
                    r.text = decode_with_fallback(img, poly);

                    return r;
                }));
    }

    // Collect results
    for (auto& f : futures)
    {
        results.push_back(f.get());
        if (results.back().text.empty()) {
            // Keep empty string if decode failed; caller can decide how to handle it.
        }
    }

    return results;
}

//decode_qr_in_image() is a simple single-threaded implementation that uses OpenCV's detectAndDecodeMulti.
//detect → warp → decode → repeat (serially)
//Problems:
// •	single-threaded
// •	decode becomes bottleneck
// •	harder to control preprocessing
static void decode_qr_in_image(const fs::path& image_path) {
    cv::Mat img = cv::imread(image_path.string(), cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "[ERROR] Failed to load image: " << image_path << std::endl;
        return;
    }

    cv::QRCodeDetector detector;
    std::vector<std::string> decoded;
    cv::Mat points;

    bool ok = detector.detectAndDecodeMulti(img, decoded, points);

    std::cout << "\n=== " << image_path.filename().string() << " ===" << std::endl;
    if (!ok || decoded.empty()) {
        std::cout << "No QR codes detected." << std::endl;
        return;
    }

    for (size_t i = 0; i < decoded.size(); ++i) {
        std::cout << "QR " << i << ": " << decoded[i] << std::endl;
    }

    // Optional visualization: draw polygons around detected QRs
    // OpenCV returns points as an Nx4 CV_32FC2 matrix for detectAndDecodeMulti.
    // Shape is typically [N, 4, 2] internally; access it as Point2f.
    if (!points.empty()) {
        // Ensure expected type
        if (points.type() == CV_32FC2) {
            int n_qr = points.rows;
            int n_corners = points.cols;

            for (int i = 0; i < n_qr; ++i) {
                std::vector<cv::Point> poly;
                poly.reserve(n_corners);

                for (int j = 0; j < n_corners; ++j) {
                    cv::Point2f pt = points.at<cv::Point2f>(i, j);
                    poly.emplace_back(cvRound(pt.x), cvRound(pt.y));
                }

                if (poly.size() >= 4) {
                    cv::polylines(img, poly, true, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

                    // Label near first corner for easier debugging
                    cv::putText(img,
                                "QR " + std::to_string(i),
                                poly[0],
                                cv::FONT_HERSHEY_SIMPLEX,
                                0.6,
                                cv::Scalar(0, 255, 0),
                                2,
                                cv::LINE_AA);
                }
            }
        } else {
            std::cerr << "[WARN] Unexpected points type from detectAndDecodeMulti: "
                      << points.type() << std::endl;
        }
    }

    // Show annotated result (press any key to continue)
    cv::imshow("QR Decode - " + image_path.filename().string(), img);
    cv::waitKey(0);
    cv::destroyAllWindows();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage:\n"
                  << "  ./app <image_path>\n"
                  << "  ./app <directory_path>\n";
        return 1;
    }

    fs::path input = argv[1];

    if (!fs::exists(input)) {
        std::cerr << "[ERROR] Path does not exist: " << input << std::endl;
        return 1;
    }

    if (fs::is_regular_file(input)) {
        // decode_qr_in_image(input);

        cv::Mat img = cv::imread(input.string(), cv::IMREAD_COLOR);
        if (img.empty()) {
            std::cerr << "[ERROR] Failed to load image: " << input << std::endl;
            return 0;
        }

        using clock = std::chrono::steady_clock;
        const auto t0 = clock::now();

        auto results = detect_and_decode_parallel(img);

        const auto t1 = clock::now();
        const auto decode_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        for (size_t i = 0; i < results.size(); ++i)
        {
            std::cout << "QR " << i << ": " << results[i].text << std::endl;

            std::vector<cv::Point> poly;
            for (auto& p : results[i].corners)
                poly.emplace_back(cvRound(p.x), cvRound(p.y));

            cv::polylines(img, poly, true, {0,255,0}, 2);

            // Label near first corner for easier debugging
            cv::putText(img,
                        "QR " + std::to_string(i),
                        poly[0],
                        cv::FONT_HERSHEY_SIMPLEX,
                        0.6,
                        cv::Scalar(0, 255, 0),
                        2,
                        cv::LINE_AA);
        }
        std::cout << "Decode + Result Generation Time: " << decode_ms << " ms" << std::endl;

        cv::imshow("QR Decode - " + input.filename().string(), img);
        cv::waitKey(0);
        cv::destroyAllWindows();
        return 0;
    }

    if (fs::is_directory(input)) {
        for (const auto& entry : fs::directory_iterator(input)) {
            if (!entry.is_regular_file()) continue;
            if (!has_image_extension(entry.path())) continue;
            decode_qr_in_image(entry.path());
        }
        return 0;
    }

    std::cerr << "[ERROR] Unsupported input type." << std::endl;
    return 1;
}