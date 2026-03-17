#include <filesystem>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

#include <opencv2/opencv.hpp>

#include <mcv/util/filesystem.hpp>
#include <mcv/util/logging.hpp>
#include <mcv/core/types.hpp>
#include <mcv/output/tables.hpp>
#include <mcv/util/timer.hpp>
#include <mcv/core/pipeline.hpp>
#include <mcv/core/GlobalVariables.hpp>
#include <mcv/output/overlay.hpp>

namespace fs = std::filesystem;

ImageResultData process_image(const fs::path& imagePath)
{
    ImageResultData resultData;

    cv::Mat img = cv::imread(imagePath.string(), cv::IMREAD_COLOR);
    if (img.empty()) 
    {
        std::cerr << "[ERROR] Failed to load image: " << imagePath << std::endl;
        return resultData;
    }

    print_section_header(std::string("IMAGE: ") + imagePath.filename().string());
    using clock = std::chrono::steady_clock;

    const auto start_timer = clock::now();

    auto results = run_pipeline(img);

    const auto end_timer = clock::now();

    resultData.decode_ms = elapsed_ms(start_timer, end_timer);
    resultData.decodedCnt = (long long) results.size();

    LOG(INFO, "Decoding Time (total)        : " << std::fixed << std::setprecision(3) << resultData.decode_ms << " ms");

    if(showWindow)
    {
        print_results_table(results);
        draw_overlay(img, results);

        draw_timing(img, resultData);

        show_debug_frame("MultiCodeVision Debug", img);

        cv::waitKey(0);
    }

    return resultData;
}

// ------------------------------------------------------------
// CLI Runner
// ------------------------------------------------------------

void run_cli(const fs::path& input)
{
    if (fs::is_regular_file(input)) 
    {
        process_image(input);
        return;
    }

    if (fs::is_directory(input)) 
    {

        std::vector<AggregatePassStats> folderAgg;

        auto find_or_add = [&](const std::string& name) -> AggregatePassStats& 
        {
            for (auto& a : folderAgg) 
            {
                if (a.name == name)
                    return a;
            }
            folderAgg.push_back(AggregatePassStats{});
            folderAgg.back().name = name;
            return folderAgg.back();
        };

        double totalDecodingTime = 0.0;
        // double totalPassTime = 0.0;

        int imageCount = 0;
        long long totalDecodedCnt = 0;
        
        for (const auto& entry : fs::directory_iterator(input)) 
        {
            if (!entry.is_regular_file())
                continue;
            if (!has_image_extension(entry.path()))
                continue;

            ImageResultData resultData = process_image(entry.path());
            totalDecodingTime += resultData.decode_ms;
            totalDecodedCnt += resultData.decodedCnt;

            imageCount++;

            // Aggregate per-pass stats from last processed image
            for (const auto& ps : g_lastPassStats) 
            {
                auto& a = find_or_add(ps.name);
                a.raw += ps.raw;
                a.added += ps.added;
                a.ms += ps.ms;
                if (ps.added > 0)
                    a.imagesContributed += 1;
            }

            //Calculate total time spent for processing all passes across the folder
            // for(const auto& ps : g_lastPassStats) 
            // {
            //     totalPassTime += ps.ms;
            // }
        }

        print_folder_summary_table(folderAgg, imageCount, totalDecodedCnt, totalDecodingTime);
        return;
    }

    std::cerr << "[ERROR] Unsupported input type." << std::endl;
}
