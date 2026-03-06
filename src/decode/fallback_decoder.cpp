#include <string>
#include <opencv2/opencv.hpp>

#include <mcv/decode/fallback_decoder.hpp>
#include <mcv/preprocess/warp.hpp>


std::string decode_with_fallback(const cv::Mat& full_img,
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