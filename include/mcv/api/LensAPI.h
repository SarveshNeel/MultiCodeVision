#pragma once

#ifdef _WIN32
  #ifdef LENS_EXPORTS
    #define LENS_API __declspec(dllexport)
  #else
    #define LENS_API __declspec(dllimport)
  #endif
#else
  #define LENS_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    LENS_OP_GRAY = 0,
    LENS_OP_ADAPTIVE_THRESHOLD = 1,
    LENS_OP_CLAHE = 2,
    LENS_OP_UPSCALE = 3

} LensPreprocessOp;

typedef enum
{
    LENS_FORMAT_QRCODE = 1

} LensDecodeFormat;

typedef enum
{
    LENS_BINARIZER_GLOBAL = 0,
    LENS_BINARIZER_LOCAL_AVERAGE = 1

} LensBinarizer;

typedef struct
{
    LensPreprocessOp ops[8];
    int opCount;

    float scale;

} LensPreprocessPass;

typedef struct
{
    LensPreprocessPass passes[16];
    int passCount;

} LensPipelineConfig;

typedef struct
{
    int tryHarder;
    int tryRotate;
    int tryInvert;

    int maxSymbols;

    LensDecodeFormat decodeFormat;
    LensBinarizer binarizer;

    int adaptiveBlockSize;
    int adaptiveC;

    double claheClipLimit;
    int claheTileSize;

} LensConfig;

typedef struct 
{
    LensConfig config;

} LensEngine;

typedef struct LensDecodeResult
{
    char text[512];
    int x;
    int y;
    int width;
    int height;
} LensDecodeResult;

typedef void* LensHandle;

LENS_API LensHandle Lens_Create();

LENS_API void Lens_Destroy(LensHandle handle);

LENS_API LensConfig Lens_GetDefaultConfig();

LENS_API void Lens_SetConfig(LensHandle handle, LensConfig cfg);

LENS_API int Lens_DecodeImage(
    LensHandle handle,
    const char* imagePath,
    LensDecodeResult* results
);

LENS_API int Lens_DecodeRawImage(
    void* handle,
    uint8_t* data,
    int width,
    int height,
    LensDecodeResult* results);

LENS_API int Lens_Init(void);
LENS_API void Lens_SetShowWindow(int enabled);
LENS_API void Lens_Shutdown(void);

LENS_API LensPipelineConfig Lens_GetDefaultPipeline();

LENS_API void Lens_SetPipeline(
    LensHandle handle,
    LensPipelineConfig pipeline
);

#ifdef __cplusplus
}
#endif