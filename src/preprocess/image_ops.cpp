#include <vector>
#include <mcv/preprocess/image_ops.hpp>
#include <mcv/output/tables.hpp>
#include <mcv/core/config.hpp>

std::vector<ZXPass> build_preprocess_passes(const cv::Mat& img)
{
    cv::Mat gray;
    if (img.channels() == 3)
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    else
        gray = img;

    std::vector<ZXPass> passes;

    passes.push_back({gray, 1.0f, "Original Gray"});

    if(ADAPTIVE_THRESHOLD_ENABLED)
    {
        cv::Mat bw;
        cv::adaptiveThreshold(gray, bw, 255,
            cv::ADAPTIVE_THRESH_GAUSSIAN_C,
            cv::THRESH_BINARY,
            ADAPTIVE_BLOCK_SIZE, ADAPTIVE_C);

        passes.push_back({bw, 1.0f, "Original Gray + Adaptive Threshold"});
    }

    cv::Mat baseImage = gray;  // use gray as base

    if(UPSCALE_GRAY_PASSES_ENABLED){

        for (float scale : UPSCALE_GRAY_PASSES)
        {
            cv::Mat scaled;
            cv::resize(baseImage, scaled, cv::Size(), scale, scale, cv::INTER_CUBIC);

            passes.push_back({scaled, 1.0f / scale, "Original Gray + Upscaled x" + format_scale(scale)});
        }
    }

    if(CLAHE_ENABLED){

        cv::Mat claheImg;
        auto clahe = cv::createCLAHE(CLAHE_CLIP_LIMIT, cv::Size(CLAHE_TILE_SIZE, CLAHE_TILE_SIZE));
        clahe->apply(baseImage, claheImg);

        passes.push_back({claheImg, 1.0f, "CLAHE"});

        if(UPSCALE_CLAHE_PASSES_ENABLED){
            // use CLAHE as base for more aggressive upscaling (helps with very small/distant QRs, especially on shiny cans)
            for (float scale : UPSCALE_CLAHE_PASSES) {

                cv::Mat scaled;
                cv::resize(claheImg, scaled, cv::Size(), scale, scale, cv::INTER_CUBIC);
                
                passes.push_back({scaled, 1.0f / scale, "CLAHE + Upscaled x" + format_scale(scale) });
            }
        }
    }

    return passes;
}