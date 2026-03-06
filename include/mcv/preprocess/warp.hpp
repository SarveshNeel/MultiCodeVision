#pragma once

#include <vector>
#include <opencv2/opencv.hpp>

cv::Mat warp_qr_patch(const cv::Mat& img, const std::vector<cv::Point2f>& corners, int out_size = 256);