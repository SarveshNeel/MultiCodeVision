#include <mcv/output/overlay.hpp>

void draw_overlay(const cv::Mat& img, std::vector<QRResult> &results)
{
    for (size_t i = 0; i < results.size(); ++i) 
    {
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
}