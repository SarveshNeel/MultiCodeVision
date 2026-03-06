#pragma once
#include <mcv/decode/zxing_decoder.hpp>

//----------------------------------------ZXing Decoder Configuration
const auto ZXING_DECODE_FORMAT = ZXing::BarcodeFormat::QRCode;
const auto ZXING_BINARIZER = ZXing::Binarizer::LocalAverage;

const bool ZXING_TRY_HARDER = true;
const bool ZXING_TRY_ROTATE = true;
const bool ZXING_TRY_INVERT = true;
const int ZXING_MAX_SYMBOLS = 40;

//------------------------------------------------Image Preprocessing

enum BaseImage 
{
    GRAY,
    ADAPTIVE_THRESHOLD,
    CLAHE,
    UPSCALE
};

struct PreprocessPassConfig 
{
    std::vector<BaseImage> ops;
    float scale = 1.0f;
};

//------------ADAPTIVE THRESHOLD---------------
const int ADAPTIVE_BLOCK_SIZE = 31;
const int ADAPTIVE_C = 3;

//------------------CLAHE----------------------
const double CLAHE_CLIP_LIMIT = 2.5;
const int CLAHE_TILE_SIZE = 8;

inline std::vector<PreprocessPassConfig> PREPROCESS_PIPELINE = {
    // baseline
    { {BaseImage::GRAY}, 1.0f },

    // threshold attempt
    { {BaseImage::GRAY, BaseImage::ADAPTIVE_THRESHOLD}, 1.0f },

    // gray upscales
    { {BaseImage::GRAY, BaseImage::UPSCALE}, 1.5f },
    { {BaseImage::GRAY, BaseImage::UPSCALE}, 3.0f },

    // CLAHE passes
    { {BaseImage::GRAY, BaseImage::CLAHE}, 1.0f },

    // CLAHE upscales
    { {BaseImage::GRAY, BaseImage::CLAHE, BaseImage::UPSCALE}, 4.0f }
};

