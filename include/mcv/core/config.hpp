#pragma once
#include <mcv/decode/zxing_decoder.hpp>

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

//-------------ZXING DECODER------------------
extern ZXing::BarcodeFormat ZXING_DECODE_FORMAT;
extern ZXing::Binarizer ZXING_BINARIZER;

extern bool ZXING_TRY_HARDER;
extern bool ZXING_TRY_ROTATE;
extern bool ZXING_TRY_INVERT;
extern int ZXING_MAX_SYMBOLS;

//------------ADAPTIVE THRESHOLD---------------
extern int ADAPTIVE_BLOCK_SIZE;
extern int ADAPTIVE_C;

//------------------CLAHE----------------------
extern double CLAHE_CLIP_LIMIT;
extern int CLAHE_TILE_SIZE;

extern int UPSCALE_INTERPOLATION;

extern std::vector<PreprocessPassConfig> PREPROCESS_PIPELINE;