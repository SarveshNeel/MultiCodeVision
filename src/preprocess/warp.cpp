#include <mcv/preprocess/warp.hpp>

cv::Mat warp_qr_patch(const cv::Mat& img, const std::vector<cv::Point2f>& corners, int out_size)
{
    if (corners.size() != 4) return cv::Mat();

    std::vector<cv::Point2f> src = corners;
    std::vector<cv::Point2f> dst = 
    {
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