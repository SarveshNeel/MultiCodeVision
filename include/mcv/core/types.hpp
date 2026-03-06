#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

struct ImageResultData 
{
    double decode_ms = 0.0;
    long long decodedCnt = 0;
};

struct QRResult 
{
    std::string text;
    std::vector<cv::Point2f> corners;
};

// preprocessing + multi-scale passes
struct ZXPass 
{
    cv::Mat img;
    float coordScale; // Factor to convert coordinates from this pass back to the original image
    std::string type; // for debugging
};

struct PassStats 
{
    std::string name;
    int raw = 0;      // raw ZXing results in this pass
    int added = 0;    // unique results added after dedup
    double ms = 0.0;  // elapsed time for this pass
};

struct AggregatePassStats 
{
    std::string name;
    long long raw = 0;
    long long added = 0;
    double ms = 0.0;
    int imagesContributed = 0; // images where this pass added >=1
};