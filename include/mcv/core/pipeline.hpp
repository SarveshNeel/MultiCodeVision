#pragma once 

#include <vector>
#include <mcv/core/types.hpp>
#include <opencv2/opencv.hpp>

std::vector<QRResult> run_pipeline(const cv::Mat& img);