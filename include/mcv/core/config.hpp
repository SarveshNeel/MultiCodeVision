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

//-----ADAPTIVE THRESHOLD--
const bool ADAPTIVE_THRESHOLD_ENABLED = true;

const int ADAPTIVE_BLOCK_SIZE = 31;
const int ADAPTIVE_C = 3;

//-----UPSCALE GRAY--------
const bool UPSCALE_GRAY_PASSES_ENABLED = true;

const std::vector<float> UPSCALE_GRAY_PASSES = {1.5f, 3.0f};

//-----CLAHE---------------
const bool CLAHE_ENABLED = true;

const double CLAHE_CLIP_LIMIT = 2.5;
const int CLAHE_TILE_SIZE = 8;

//-----UPSCALE CLAHE-------
const bool UPSCALE_CLAHE_PASSES_ENABLED = true;

const std::vector<float> UPSCALE_CLAHE_PASSES = {4.0f};

