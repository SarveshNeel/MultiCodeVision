#include <cstring>
#include <algorithm>
#include <exception>

#include <opencv2/opencv.hpp>

#include <mcv/api/LensAPI.h>
#include <mcv/core/pipeline.hpp>
#include <mcv/core/GlobalVariables.hpp>


// ------------------------------------------------------------
// Initialize library
// ------------------------------------------------------------

LENS_API int Lens_Init(void)
{
    return 0;
}


// ------------------------------------------------------------
// Enable / disable debug window
// ------------------------------------------------------------

LENS_API void Lens_SetShowWindow(int enabled)
{
    showWindow = (enabled != 0);
}


// ------------------------------------------------------------
// Decode image
// ------------------------------------------------------------

LENS_API int Lens_DecodeImage(const char* imagePath,
                              LensDecodeResult* results,
                              int maxResults)
{
    try
    {
        if (!imagePath || !results || maxResults <= 0)
            return -1;

        cv::Mat img = cv::imread(imagePath, cv::IMREAD_COLOR);

        if (img.empty())
            return -2;

        auto decoded = run_pipeline(img);

        int count = std::min((int)decoded.size(), maxResults);

        for (int i = 0; i < count; ++i)
        {
            const auto& r = decoded[i];

            //------------------------------------------------
            // Safe text copy
            //------------------------------------------------

            std::strncpy(results[i].text,
                         r.text.c_str(),
                         sizeof(results[i].text) - 1);

            results[i].text[sizeof(results[i].text) - 1] = '\0';

            //------------------------------------------------
            // Validate corners
            //------------------------------------------------

            if (r.corners.size() < 4)
            {
                results[i].x = 0;
                results[i].y = 0;
                results[i].width = 0;
                results[i].height = 0;
                continue;
            }

            //------------------------------------------------
            // Compute bounding box
            //------------------------------------------------

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


// ------------------------------------------------------------
// Shutdown
// ------------------------------------------------------------

LENS_API void Lens_Shutdown(void)
{
}