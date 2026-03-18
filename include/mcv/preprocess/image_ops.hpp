#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include <mcv/core/types.hpp>

int get_resize_interpolation();
std::vector<ZXPass> build_preprocess_passes(const cv::Mat& gray);