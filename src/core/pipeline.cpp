#include <vector>
#include <opencv2/opencv.hpp>
#include <omp.h>
#include <atomic>

#include <mcv/core/types.hpp>
#include <mcv/core/pipeline.hpp>
#include <mcv/util/timer.hpp>
#include <mcv/output/tables.hpp>
#include <mcv/core/GlobalVariables.hpp>
#include <mcv/core/config.hpp>

#include <mcv/preprocess/image_ops.hpp>
#include <mcv/decode/zxing_decoder.hpp>
#include <mcv/decode/fallback_decoder.hpp>
#include <mcv/util/geometry.hpp>

std::vector<QRResult> run_pipeline(const cv::Mat& img)
{
    std::vector<QRResult> results;

    g_passStats.clear();
    g_passStats.reserve(16);

    configure_zxing_decoder();

    // Build preprocessing passes
    std::vector<ZXPass> passes = build_preprocess_passes(img);

    g_passStats.resize(passes.size());

    #pragma omp parallel for
    for (int i = 0; i < static_cast<int>(passes.size()); ++i)
    {
        const auto& pass = passes[i];

        PassStats stats;
        stats.name = pass.type;

        // static std::atomic<int> dump_id{0};

        // int id = i;

        // std::string filename = "debug_pass_" + std::to_string(i) + "_" + std::to_string(id) + ".png";

        // cv::Mat debug_img = pass.img;

        // // Ensure grayscale consistency
        // if (debug_img.channels() == 3)
        //     cv::cvtColor(debug_img, debug_img, cv::COLOR_BGR2GRAY);

        // cv::imwrite(filename, debug_img);

        const auto start_timer = std::chrono::steady_clock::now();

        auto zresults = run_zxing_decoder(pass.img);
        stats.raw = static_cast<int>(zresults.size());

        // std::cout << "[PASS " << i << "] " << pass.type
        //   << " -> decoded: " << zresults.size()
        //   << std::endl;
        
        // for (const auto& zr : zresults)
        // {
        //     if (!zr.isValid())
        //         continue;

        //     std::string txt = zr.text();
        //     if (txt.empty())
        //         txt = "<empty>";

        //     auto pos = zr.position();
        //     std::cout << "    text: " << txt
        //             << "  TL=(" << pos.topLeft().x << "," << pos.topLeft().y << ")"
        //             << "  TR=(" << pos.topRight().x << "," << pos.topRight().y << ")"
        //             << "  BR=(" << pos.bottomRight().x << "," << pos.bottomRight().y << ")"
        //             << "  BL=(" << pos.bottomLeft().x << "," << pos.bottomLeft().y << ")"
        //             << std::endl;
        // }

        std::vector<QRResult> local_results;

        for (const auto& zr : zresults)
        {
            if (!zr.isValid())
                continue;

            QRResult r;
            r.text = zr.text();

            auto pos = zr.position();
            const float s = pass.coordScale;

            r.corners.push_back(cv::Point2f(pos.topLeft().x * s, pos.topLeft().y * s));
            r.corners.push_back(cv::Point2f(pos.topRight().x * s, pos.topRight().y * s));
            r.corners.push_back(cv::Point2f(pos.bottomRight().x * s, pos.bottomRight().y * s));
            r.corners.push_back(cv::Point2f(pos.bottomLeft().x * s, pos.bottomLeft().y * s));

            if (r.text.empty() && r.corners.size() == 4)
            {
                r.text = decode_with_fallback(img, r.corners);
            }

            if (!r.text.empty())
            {
                local_results.push_back(std::move(r));
            }
        }

        int added_local = 0;

    #pragma omp critical
        {
            for (auto& r : local_results)
            {
                bool dup = false;

                for (const auto& existing : results)
                {
                    if (is_duplicate(existing, r))
                    {
                        dup = true;
                        break;
                    }
                }

                if (!dup)
                {
                    results.push_back(std::move(r));
                    added_local++;
                }
            }
        }

        const auto end_timer = std::chrono::steady_clock::now();

        stats.added = added_local;
        stats.ms = elapsed_ms(start_timer, end_timer);

        g_passStats[i] = stats;
    }

    if (showWindow) 
    {
        print_pass_summary();
    }

    // std::cout << "\nFinal unique results:\n";
    // for (const auto& r : results) {
    //     std::cout << "  " << r.text << "\n";
    // }

    LOG(INFO, "Total unique QRs decoded: " << results.size());

    g_lastPassStats = g_passStats;

    return results;
}