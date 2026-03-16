#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include <ZXing/ReadBarcode.h>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/ReaderOptions.h>
#include <ZXing/ImageView.h>
#include <ZXing/Result.h>

// Run ZXing decoding on an image
ZXing::Results run_zxing_decoder(const cv::Mat& img);

// Convert OpenCV image to ZXing ImageView
ZXing::ImageView to_zxing_imageview(const cv::Mat& img, cv::Mat& gray_out);

// Configure ZXing reader options
void configure_zxing_decoder();