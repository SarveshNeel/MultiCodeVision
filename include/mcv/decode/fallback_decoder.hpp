#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

std::string decode_with_fallback(const cv::Mat& full_img, const std::vector<cv::Point2f>& corners);