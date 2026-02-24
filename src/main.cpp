#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static bool has_image_extension(const fs::path& p) {
    const std::string ext = p.extension().string();
    if (ext.empty()) return false;
    std::string e;
    e.reserve(ext.size());
    for (char c : ext) e.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return e == ".jpg" || e == ".jpeg" || e == ".png" || e == ".bmp" || e == ".tif" || e == ".tiff" || e == ".webp";
}

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
        decode_qr_in_image(input);
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