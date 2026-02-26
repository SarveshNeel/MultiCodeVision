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
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

enum LogLevel {
    ERROR,
    INFO,
    DEBUG,
    TRACE
};

static LogLevel LOG_LEVEL = INFO;

#define LOG(level, msg) \
    do { if ((level) <= LOG_LEVEL) { std::cout << msg << std::endl; } } while (0)

struct QRResult {
    std::string text;
    std::vector<cv::Point2f> corners;
};

// preprocessing + multi-scale passes
struct ZXPass {
    cv::Mat img;
    float coordScale; // Factor to convert coordinates from this pass back to the original image
    std::string type; // for debugging
};

struct PassStats {
    std::string name;
    int raw = 0;      // raw ZXing results in this pass
    int added = 0;    // unique results added after dedup
    double ms = 0.0;  // elapsed time for this pass
};

struct ScopedTimer {
    std::string name;
    std::chrono::steady_clock::time_point start;

    ScopedTimer(const std::string &n) : name(n), start(std::chrono::steady_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        LOG(DEBUG, "[TIME] " << name << ": " << ms << " ms");
    }
};

static inline double elapsed_ms(const std::chrono::steady_clock::time_point& a,
                                const std::chrono::steady_clock::time_point& b)
{
    return std::chrono::duration<double, std::milli>(b - a).count();
}

// Helper to format float nicely
std::string format_scale(float s) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << s;
    return oss.str();
}

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
std::vector<QRResult> detect_and_decode(const cv::Mat& img)
{
    std::vector<QRResult> results;
    std::vector<PassStats> passStats;
    passStats.reserve(16);

    // ----- ZXing detection + decode (single pass, robust for small/rotated QR) -----
    // Allow more symbols per frame (we expect up to 24+ codes)
    ZXing::ReaderOptions hints;
    hints.setFormats(ZXing::BarcodeFormat::QRCode);
    hints.setTryHarder(true);
    hints.setTryRotate(true);
    hints.setTryInvert(true);
    hints.setMaxNumberOfSymbols(40);
    hints.setBinarizer(ZXing::Binarizer::LocalAverage);

    cv::Mat gray;
    if (img.channels() == 3)
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    else
        gray = img;

    std::vector<ZXPass> passes;
    passes.push_back({gray, 1.0f, "Original Gray"});

    // cv::Mat sharpened;
    // cv::GaussianBlur(gray, sharpened, cv::Size(0, 0), 1.0);
    // cv::addWeighted(gray, 1.5, sharpened, -0.5, 0, sharpened);
    // passes.push_back({sharpened, 1.0f});

    // adaptive threshold
    cv::Mat bw;
    cv::adaptiveThreshold(gray, bw, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY, 31, 3);
    passes.push_back({bw, 1.0f, "Original Gray + Adaptive Threshold"});

    // Upscaled CLAHE pass to help with very small / distant QRs
    // {
    //     cv::Mat srcForUpscale = claheImg.empty() ? gray : claheImg;
    //     cv::Mat upScaled;
    //     double upscale = 2.0;
    //     cv::resize(srcForUpscale, upScaled, cv::Size(), upscale, upscale, cv::INTER_CUBIC);
    //     // Coordinates from this pass need to be scaled back down
    //     passes.push_back({upScaled, static_cast<float>(1.0 / upscale)});
    // }

    // build a scale pyramid instead of just one upscaled pass, to better handle a range of small QR sizes
    std::vector<float> scales = {1.5f, 3.0f};
    cv::Mat baseImage = gray;  // use gray as base

    for (float scale : scales) {
        cv::Mat scaled;
        cv::resize(baseImage, scaled, cv::Size(), scale, scale, cv::INTER_CUBIC);
        passes.push_back({scaled, 1.0f / scale, "Original Gray + Upscaled x" + format_scale(scale)});
    }

    // CLAHE (VERY important for shiny cans)
    cv::Mat claheImg;
    {
        auto clahe = cv::createCLAHE(2.5, cv::Size(8,8));
        clahe->apply(gray, claheImg);
        passes.push_back({claheImg, 1.0f, "CLAHE"});
    }

    scales = {4.0f};
    baseImage = claheImg; // use CLAHE as base for more aggressive upscaling (helps with very small/distant QRs, especially on shiny cans)
    for (float scale : scales) {
        cv::Mat scaled;
        cv::resize(baseImage, scaled, cv::Size(), scale, scale, cv::INTER_CUBIC);
        passes.push_back({scaled, 1.0f / scale, "CLAHE + Upscaled x" + format_scale(scale) });
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
        PassStats stats;
        stats.name = pass.type;

        const auto start_timer = std::chrono::steady_clock::now();

        cv::Mat tmp;
        auto iv = to_zxing_imageview(pass.img, tmp);
        auto zresults = ZXing::ReadBarcodes(iv, hints);

        stats.raw = static_cast<int>(zresults.size());

        for (const auto& zr : zresults)
        {
            if (!zr.isValid())
                continue;

            QRResult r;
            r.text = zr.text();

            // Debug: classify ZXing result state
            // detected + decoded  => text non-empty
            // detected but decode failed => text empty
            // if (r.text.empty()) {
            //     std::cout << "[ZXing] DETECTED (decode FAILED)" << std::endl;
            // } else {
            //     std::cout << "[ZXing] DETECTED + DECODED" << std::endl;
            // }

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
            if (!dup && !r.text.empty()) {
                results.push_back(std::move(r));
                stats.added++;
            }
        }
        const auto end_timer = std::chrono::steady_clock::now();

        stats.ms = elapsed_ms(start_timer, end_timer);
        passStats.push_back(std::move(stats));
    }

    // PASS SUMMARY (boxed table)
    {
        size_t nameW = 9; // "Pass Name"
        for (const auto& s : passStats)
            nameW = std::max(nameW, s.name.size());
        nameW = std::min<size_t>(nameW, 48);

        auto hr = [&]() {
            std::ostringstream oss;
            oss << '+' << std::string(nameW + 2, '-')
                << '+' << std::string(6 + 2, '-')
                << '+' << std::string(8 + 2, '-')
                << '+' << std::string(10 + 2, '-') << '+';
            LOG(INFO, oss.str());
        };

        LOG(INFO, "\n[PASS SUMMARY]");
        hr();
        {
            std::ostringstream oss;
            oss << "| " << std::left << std::setw(static_cast<int>(nameW)) << "Pass Name"
                << " | " << std::right << std::setw(6) << "Raw"
                << " | " << std::right << std::setw(8) << "Added"
                << " | " << std::right << std::setw(10) << "Time(ms)"
                << " |";
            LOG(INFO, oss.str());
        }
        hr();

        for (const auto& s : passStats) {
            std::string n = s.name;
            if (n.size() > nameW) {
                if (nameW > 3) n = n.substr(0, nameW - 3) + "...";
                else n = n.substr(0, nameW);
            }
            std::ostringstream oss;
            oss << "| " << std::left << std::setw(static_cast<int>(nameW)) << n
                << " | " << std::right << std::setw(6) << s.raw
                << " | " << std::right << std::setw(8) << s.added
                << " | " << std::right << std::setw(10) << std::fixed << std::setprecision(3) << s.ms
                << " |";
            LOG(INFO, oss.str());
        }
        hr();
        LOG(INFO, "Total unique decoded: " << results.size());
    }

    return results;
}

static void print_results_table(const std::vector<QRResult>& results)
{
    if (results.empty()) {
        LOG(INFO, "[RESULTS] No decoded QR strings.");
        return;
    }

    // Copy + sort for stable presentation
    std::vector<std::string> values;
    values.reserve(results.size());
    for (const auto& r : results)
        values.push_back(r.text);
    std::sort(values.begin(), values.end());

    // Layout config
    constexpr size_t kCols = 3; // multi-column output
    size_t textW = 7; // "Decoded"
    for (const auto& v : values)
        textW = std::max(textW, v.size());
    textW = std::min<size_t>(textW, 32); // keep table compact

    const size_t rows = (values.size() + kCols - 1) / kCols;

    auto hr = [&]() {
        std::ostringstream oss;
        for (size_t c = 0; c < kCols; ++c)
            oss << '+' << std::string(textW + 2, '-');
        oss << '+';
        LOG(INFO, oss.str());
    };

    LOG(INFO, "\n[RESULTS TABLE]");
    hr();
    {
        std::ostringstream oss;
        for (size_t c = 0; c < kCols; ++c)
            oss << "| " << std::left << std::setw(static_cast<int>(textW)) << (std::string("Decoded")) << ' ';
        oss << '|';
        LOG(INFO, oss.str());
    }
    hr();

    for (size_t r = 0; r < rows; ++r) {
        std::ostringstream oss;
        for (size_t c = 0; c < kCols; ++c) {
            const size_t idx = c * rows + r; // column-major fill for compactness
            std::string cell;
            if (idx < values.size()) {
                cell = values[idx];
                if (cell.size() > textW) {
                    if (textW > 3) cell = cell.substr(0, textW - 3) + "...";
                    else cell = cell.substr(0, textW);
                }
            }
            oss << "| " << std::left << std::setw(static_cast<int>(textW)) << cell << ' ';
        }
        oss << '|';
        LOG(INFO, oss.str());
    }

    hr();
    LOG(INFO, "Total decoded: " << values.size());
}

static void process_image_with_zxing(const fs::path& imagePath, bool showWindow)
{
    cv::Mat img = cv::imread(imagePath.string(), cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "[ERROR] Failed to load image: " << imagePath << std::endl;
        return;
    }

    std::cout << "\n======================================== " << imagePath.filename().string() << " ========================================" << std::endl;
    using clock = std::chrono::steady_clock;

    const auto start_timer = clock::now();
    auto results = detect_and_decode(img);
    const auto end_timer = clock::now();

    const auto decode_ms = elapsed_ms(start_timer, end_timer);

    if (results.empty()) {
        // std::cout << "No QR codes detected." << std::endl;
    } else {
        for (size_t i = 0; i < results.size(); ++i) {
            if(showWindow)
            {
                // std::cout << "QR " << i << ": " << results[i].text << std::endl;
            }

            std::vector<cv::Point> poly;
            for (const auto& p : results[i].corners)
                poly.emplace_back(cvRound(p.x), cvRound(p.y));

            if (poly.size() >= 4)
                cv::polylines(img, poly, true, {0,255,0}, 2);
            
            cv::putText(img,
                "QR " + std::to_string(i),
                poly[0],
                cv::FONT_HERSHEY_SIMPLEX,
                1.6,
                cv::Scalar(0, 255, 0),
                2,
                cv::LINE_AA);
        }
        // std::cout << std::endl << "Total QR codes detected: " << results.size() << std::endl << std::endl;
    }

    std::cout << "Detection & Decode Time: " << decode_ms << " ms" << std::endl << std::endl;

    print_results_table(results);

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