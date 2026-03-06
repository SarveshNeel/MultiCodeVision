#include <mcv/decode/zxing_decoder.hpp>
#include <opencv2/opencv.hpp>
#include <mcv/core/config.hpp>

ZXing::ReaderOptions hints;

ZXing::ImageView to_zxing_imageview(const cv::Mat& img, cv::Mat& gray_out)
{
    if (img.channels() == 3)
        cv::cvtColor(img, gray_out, cv::COLOR_BGR2GRAY);
    else
        gray_out = img;

    return ZXing::ImageView(gray_out.data, gray_out.cols, gray_out.rows, ZXing::ImageFormat::Lum);
}

void configure_zxing_decoder(){

    hints.setFormats(ZXING_DECODE_FORMAT);
    hints.setTryHarder(ZXING_TRY_HARDER);
    hints.setTryRotate(ZXING_TRY_ROTATE);
    hints.setTryInvert(ZXING_TRY_INVERT);
    hints.setMaxNumberOfSymbols(ZXING_MAX_SYMBOLS);
    hints.setBinarizer(ZXING_BINARIZER);

}

std::vector<ZXing::Barcode> run_zxing_decoder(const cv::Mat& img){

    cv::Mat tmp;
    auto iv = to_zxing_imageview(img, tmp);
    
    return ZXing::ReadBarcodes(iv, hints);
}