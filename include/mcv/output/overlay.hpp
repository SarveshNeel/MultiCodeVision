#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include <mcv/core/types.hpp>

#define DISPLAY_WIDTH 1500
#define DISPLAY_HEIGHT 1400

void draw_overlay(const cv::Mat& img, std::vector<QRResult> &results);

void show_debug_frame(const std::string& title, const cv::Mat& img);

void draw_timing(cv::Mat& img, ImageResultData resultData);