#include <mcv/core/config.hpp>

ZXing::BarcodeFormat ZXING_DECODE_FORMAT = ZXing::BarcodeFormat::QRCode;
ZXing::Binarizer ZXING_BINARIZER = ZXing::Binarizer::LocalAverage;

bool ZXING_TRY_HARDER = true;
bool ZXING_TRY_ROTATE = true;
bool ZXING_TRY_INVERT = false;
int ZXING_MAX_SYMBOLS = 40;

int ADAPTIVE_BLOCK_SIZE = 31;
int ADAPTIVE_C = 3;

double CLAHE_CLIP_LIMIT = 2.5;
int CLAHE_TILE_SIZE = 10;

std::vector<PreprocessPassConfig> PREPROCESS_PIPELINE = {
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