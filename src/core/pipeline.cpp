#include <vector>
#include <opencv2/opencv.hpp>

#include <mcv/core/types.hpp>
#include <mcv/core/pipeline.hpp>
#include <mcv/util/timer.hpp>
#include <mcv/output/tables.hpp>
#include <mcv/core/GlobalVariables.hpp>
#include <mcv/core/config.hpp>

#include <mcv/preprocess/image_ops.hpp>
#include <mcv/decode/zxing_decoder.hpp>
#include <mcv/decode/fallback_decoder.hpp>

std::vector<PassStats> g_lastPassStats;
std::vector<PassStats> g_passStats;

auto center_of(const QRResult& q) {
    cv::Point2f c(0.f, 0.f);
    if (q.corners.size() != 4)
        return c;
    for (const auto& p : q.corners)
        c += p;
    c *= 0.25f;
    return c;
}

auto is_duplicate(const QRResult& a, const QRResult& b) {
    // Prefer exact text match when available
    if (!a.text.empty() && !b.text.empty() && a.text == b.text)
        return true;

    // Fallback: same spatial location (helps when text decode differs across passes)
    if (a.corners.size() == 4 && b.corners.size() == 4) {
        const cv::Point2f ca = center_of(a);
        const cv::Point2f cb = center_of(b);
        return cv::norm(ca - cb) < 18.0f;
    }
    return false;
}

std::vector<QRResult> run_pipeline(const cv::Mat& img)
{
    std::vector<QRResult> results;

    g_passStats.clear();
    g_passStats.reserve(16);

    configure_zxing_decoder();

    // Build preprocessing passes
    std::vector<ZXPass> passes = build_preprocess_passes(img);

    for (const auto& pass : passes)
    {
        PassStats stats;
        stats.name = pass.type;

        const auto start_timer = std::chrono::steady_clock::now();

        auto zresults = run_zxing_decoder(pass.img);

        stats.raw = static_cast<int>(zresults.size());

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

            // Fallback for difficult symbols (includes upscaled retry)
            if (r.text.empty() && r.corners.size() == 4) {
                r.text = decode_with_fallback(img, r.corners);
            }

            bool dup = false;
            for (const auto& existing : results) {
                if (is_duplicate(existing, r)) {
                    dup = true;
                    break;
                }
            }
            if (!dup && !r.text.empty()) {
                results.push_back(std::move(r));
                stats.added++;
            }
        }
        const auto end_timer = std::chrono::steady_clock::now();

        stats.ms = elapsed_ms(start_timer, end_timer);
        g_passStats.push_back(std::move(stats));
    }

    if (showWindow) {
        print_pass_summary();
    }

    LOG(INFO, "Total unique QRs decoded: " << results.size());

    g_lastPassStats = g_passStats;

    return results;
}