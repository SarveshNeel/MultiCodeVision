#include <vector>
#include <unordered_map>
#include <string>
#include <omp.h>

#include <mcv/preprocess/image_ops.hpp>
#include <mcv/output/tables.hpp>
#include <mcv/core/config.hpp>
#include <mcv/util/hashing.hpp>

std::vector<ZXPass> build_preprocess_passes(const cv::Mat& img)
{
    cv::Mat gray;
    if (img.channels() == 3)
        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    else
        gray = img;

    std::unordered_map<std::size_t, cv::Mat> cache;
    std::vector<ZXPass> passes;

    for (const auto& passCfg : PREPROCESS_PIPELINE)
    {
        cv::Mat current = gray;
        std::size_t runningHash = 0;

        for (auto op : passCfg.ops)
        {
            std::size_t key = build_hash(runningHash, op, (op == BaseImage::UPSCALE ? passCfg.scale : 0.0));

            auto it = cache.find(key);
            if (it != cache.end())
            {
                current = it->second;
                runningHash = key;
                continue;
            }

            cv::Mat result;

            switch (op)
            {
                case BaseImage::GRAY:
                {
                    result = gray;
                    runningHash = key;
                    break;
                }

                case BaseImage::CLAHE:
                {
                    static auto clahe = cv::createCLAHE(CLAHE_CLIP_LIMIT, cv::Size(CLAHE_TILE_SIZE, CLAHE_TILE_SIZE));
                    clahe->apply(current, result);
                    break;
                }

                case BaseImage::ADAPTIVE_THRESHOLD:
                {
                    cv::adaptiveThreshold(
                        current,
                        result,
                        255,
                        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                        cv::THRESH_BINARY,
                        ADAPTIVE_BLOCK_SIZE,
                        ADAPTIVE_C
                    );
                    break;
                }

                case BaseImage::UPSCALE:
                {
                    cv::resize(
                        current,
                        result,
                        cv::Size(),
                        passCfg.scale,
                        passCfg.scale,
                        cv::INTER_CUBIC
                    );
                    break;
                }
            }

            cache[key] = result;
            current = result;
            runningHash = key;
        }

        std::string label;

        for (auto op : passCfg.ops)
        {
            if (!label.empty())
                label += "+";

            switch (op)
            {
                case BaseImage::GRAY: label += "GRAY"; break;
                case BaseImage::CLAHE: label += "CLAHE"; break;
                case BaseImage::ADAPTIVE_THRESHOLD: label += "ADAPT"; break;
                case BaseImage::UPSCALE:
                    label += "UPSCALE x" + format_scale(passCfg.scale);
                    break;
            }
        }

        passes.push_back({current, 1.0f / passCfg.scale, label});
    }

    return passes;
}