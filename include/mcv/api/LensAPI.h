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

typedef struct LensDecodeResult
{
    char text[512];
    int x;
    int y;
    int width;
    int height;
} LensDecodeResult;

LENS_API int Lens_Init(void);
LENS_API void Lens_SetShowWindow(int enabled);
LENS_API int Lens_DecodeImage(const char* imagePath, LensDecodeResult* results, int maxResults);
LENS_API void Lens_Shutdown(void);

#ifdef __cplusplus
}
#endif