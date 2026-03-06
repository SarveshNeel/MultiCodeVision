#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include <mcv/core/types.hpp>

std::vector<ZXPass> build_preprocess_passes(const cv::Mat& gray);