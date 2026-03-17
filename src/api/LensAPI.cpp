#include <cstring>
#include <algorithm>
#include <exception>

#include <opencv2/opencv.hpp>

#include <mcv/api/LensAPI.h>
#include <mcv/core/pipeline.hpp>
#include <mcv/core/GlobalVariables.hpp>
#include <mcv/core/config.hpp>


LENS_API int Lens_Init(void)
{
    return 0;
}

LENS_API void Lens_SetShowWindow(int enabled)
{
    showWindow = (enabled != 0);
}

LENS_API int Lens_DecodeImage(LensHandle handle, 
                              const char* imagePath,
                              LensDecodeResult* results)
{
    try
    {
        if (!imagePath || !results)
            return -1;

        cv::Mat img = cv::imread(imagePath, cv::IMREAD_COLOR);

        if (img.empty())
            return -2;

        auto decoded = run_pipeline(img);

        int count = (int)decoded.size();

        for (int i = 0; i < count; ++i)
        {
            const auto& r = decoded[i];

            std::strncpy(results[i].text,
                         r.text.c_str(),
                         sizeof(results[i].text) - 1);

            results[i].text[sizeof(results[i].text) - 1] = '\0';

            if (r.corners.size() < 4)
            {
                results[i].x = 0;
                results[i].y = 0;
                results[i].width = 0;
                results[i].height = 0;
                continue;
            }

            float minX = r.corners[0].x;
            float minY = r.corners[0].y;
            float maxX = r.corners[0].x;
            float maxY = r.corners[0].y;

            for (const auto& p : r.corners)
            {
                minX = std::min(minX, p.x);
                minY = std::min(minY, p.y);
                maxX = std::max(maxX, p.x);
                maxY = std::max(maxY, p.y);
            }

            results[i].x = static_cast<int>(minX);
            results[i].y = static_cast<int>(minY);
            results[i].width  = static_cast<int>(maxX - minX);
            results[i].height = static_cast<int>(maxY - minY);
        }

        return count;
    }
    catch (...)
    {
        return -99;
    }
}

LENS_API void Lens_Shutdown(void)
{
}

LensHandle Lens_Create()
{
    return new LensEngine();
}

void Lens_Destroy(LensHandle handle)
{
    delete static_cast<LensEngine*>(handle);
}

static BaseImage convertOp(LensPreprocessOp op)
{
    switch (op)
    {
        case LENS_OP_GRAY: return BaseImage::GRAY;
        case LENS_OP_ADAPTIVE_THRESHOLD: return BaseImage::ADAPTIVE_THRESHOLD;
        case LENS_OP_CLAHE: return BaseImage::CLAHE;
        case LENS_OP_UPSCALE: return BaseImage::UPSCALE;
        default: return BaseImage::GRAY;
    }
}

LENS_API void Lens_SetPipeline(
    LensHandle handle,
    LensPipelineConfig pipeline)
{
    PREPROCESS_PIPELINE.clear();

    for (int i = 0; i < pipeline.passCount; i++)
    {
        PreprocessPassConfig pass;

        pass.scale = pipeline.passes[i].scale;

        for (int j = 0; j < pipeline.passes[i].opCount; j++)
        {
            pass.ops.push_back(
                convertOp(pipeline.passes[i].ops[j]));
        }

        PREPROCESS_PIPELINE.push_back(pass);
    }
}

LENS_API LensPipelineConfig Lens_GetDefaultPipeline()
{
    LensPipelineConfig cfg;

    cfg.passCount = (int)PREPROCESS_PIPELINE.size();

    for (int i = 0; i < cfg.passCount; i++)
    {
        cfg.passes[i].scale = PREPROCESS_PIPELINE[i].scale;

        int j = 0;

        for (auto op : PREPROCESS_PIPELINE[i].ops)
        {
            cfg.passes[i].ops[j++] = (LensPreprocessOp)op;
        }

        cfg.passes[i].opCount = j;
    }

    return cfg;
}

LENS_API void Lens_SetConfig(LensHandle handle, LensConfig cfg)
{
    ZXING_TRY_HARDER = cfg.tryHarder != 0;
    ZXING_TRY_ROTATE = cfg.tryRotate != 0;
    ZXING_TRY_INVERT = cfg.tryInvert != 0;

    ZXING_MAX_SYMBOLS = cfg.maxSymbols;

    switch(cfg.decodeFormat)
    {
        case LENS_FORMAT_QRCODE:
            ZXING_DECODE_FORMAT = ZXing::BarcodeFormat::QRCode;
            break;
        default:
            ZXING_DECODE_FORMAT = ZXing::BarcodeFormat::QRCode;
    }

    switch(cfg.binarizer)
    {
        case LENS_BINARIZER_GLOBAL:
            ZXING_BINARIZER = ZXing::Binarizer::GlobalHistogram;
            break;
        case LENS_BINARIZER_LOCAL_AVERAGE:
            ZXING_BINARIZER = ZXing::Binarizer::LocalAverage;
            break;
        default:
            ZXING_BINARIZER = ZXing::Binarizer::LocalAverage;
    }

    ADAPTIVE_BLOCK_SIZE = cfg.adaptiveBlockSize;
    ADAPTIVE_C = cfg.adaptiveC;

    CLAHE_CLIP_LIMIT = cfg.claheClipLimit;
    CLAHE_TILE_SIZE = cfg.claheTileSize;
}

LENS_API int Lens_DecodeRawImage(
    void* handle,
    uint8_t* data,
    int width,
    int height,
    LensDecodeResult* results)
{
    cv::Mat img(height, width, CV_8UC1, data);

    auto decoded = run_pipeline(img);

    int count = (int)decoded.size();

    for (int i = 0; i < count; ++i)
    {
        const auto& r = decoded[i];

        std::strncpy(results[i].text,
                        r.text.c_str(),
                        sizeof(results[i].text) - 1);

        results[i].text[sizeof(results[i].text) - 1] = '\0';

        if (r.corners.size() < 4)
        {
            results[i].x = 0;
            results[i].y = 0;
            results[i].width = 0;
            results[i].height = 0;
            continue;
        }

        float minX = r.corners[0].x;
        float minY = r.corners[0].y;
        float maxX = r.corners[0].x;
        float maxY = r.corners[0].y;

        for (const auto& p : r.corners)
        {
            minX = std::min(minX, p.x);
            minY = std::min(minY, p.y);
            maxX = std::max(maxX, p.x);
            maxY = std::max(maxY, p.y);
        }

        results[i].x = static_cast<int>(minX);
        results[i].y = static_cast<int>(minY);
        results[i].width  = static_cast<int>(maxX - minX);
        results[i].height = static_cast<int>(maxY - minY);
    }

    return count;
}