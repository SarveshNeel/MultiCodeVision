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

void show_debug_frame(const std::string& title, const cv::Mat& img)
{
    static bool initialized = false;

    if (!initialized)
    {
        cv::namedWindow(title, cv::WINDOW_NORMAL);
        cv::resizeWindow(title, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        initialized = true;
    }

    cv::imshow(title, img);
}

void draw_timing(cv::Mat& img, double ms)
{
    std::string txt = "Decode: " + std::to_string(ms) + " ms";

    cv::putText(img,
                txt,
                {20,40},
                cv::FONT_HERSHEY_SIMPLEX,
                1.6,
                cv::Scalar(255,0,0),
                2,
                cv::LINE_AA);
}