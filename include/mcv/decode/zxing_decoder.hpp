#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include <ReadBarcode.h>
#include <BarcodeFormat.h>
#include <ReaderOptions.h>
#include <ImageView.h>
// #include <Result.h>

// Run ZXing decoding on an image
std::vector<ZXing::Barcode> run_zxing_decoder(const cv::Mat& img);

// Convert OpenCV image to ZXing ImageView
ZXing::ImageView to_zxing_imageview(const cv::Mat& img, cv::Mat& gray_out);

// Configure ZXing reader options
void configure_zxing_decoder();