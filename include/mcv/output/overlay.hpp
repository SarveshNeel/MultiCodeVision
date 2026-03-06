#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include <mcv/core/types.hpp>

void draw_overlay(const cv::Mat& img, std::vector<QRResult> &results);