#pragma once

#include <opencv2/opencv.hpp>
#include <mcv/core/types.hpp>

inline cv::Point2f center_of(const QRResult& q) 
{
    cv::Point2f c(0.f, 0.f);
    if (q.corners.size() != 4)
        return c;
    for (const auto& p : q.corners)
        c += p;
    c *= 0.25f;
    return c;
}

inline bool is_duplicate(const QRResult& a, const QRResult& b) 
{
    // Prefer exact text match when available
    if (!a.text.empty() && !b.text.empty() && a.text == b.text)
        return true;

    // Fallback: same spatial location (helps when text decode differs across passes)
    if (a.corners.size() == 4 && b.corners.size() == 4) 
    {
        const cv::Point2f ca = center_of(a);
        const cv::Point2f cb = center_of(b);
        return cv::norm(ca - cb) < 18.0f;
    }
    return false;
}