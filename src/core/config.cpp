#include <mcv/core/config.hpp>

ZXing::BarcodeFormat ZXING_DECODE_FORMAT = ZXing::BarcodeFormat::QRCode;
ZXing::Binarizer ZXING_BINARIZER = ZXing::Binarizer::LocalAverage;

bool ZXING_TRY_HARDER = true;
bool ZXING_TRY_ROTATE = true;
bool ZXING_TRY_INVERT = true;
int ZXING_MAX_SYMBOLS = 40;

int ADAPTIVE_BLOCK_SIZE = 31;
int ADAPTIVE_C = 3;

double CLAHE_CLIP_LIMIT = 2.5;
int CLAHE_TILE_SIZE = 8;