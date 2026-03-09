#include <sstream>
#include <algorithm>
#include <iomanip>

#include <mcv/output/tables.hpp>
#include <mcv/util/logging.hpp>
#include <mcv/core/types.hpp>
#include <mcv/core/GlobalVariables.hpp>

// Helper to format float nicely
std::string format_scale(float s) 
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << s;
    return oss.str();
}

// PASS SUMMARY (boxed table)
void print_pass_summary()
{    
    size_t nameW = 9; // "Pass Name"
    for (const auto& s : g_passStats)
        nameW = std::max(nameW, s.name.size());
    nameW = std::min<size_t>(nameW, 48);

    auto hr = [&]() 
    {
        std::ostringstream oss;
        oss << '+' << std::string(nameW + 2, '-')
            << '+' << std::string(6 + 2, '-')
            << '+' << std::string(8 + 2, '-')
            << '+' << std::string(10 + 2, '-') << '+';
        LOG(INFO, oss.str());
    };

    print_section_header("PASS SUMMARY");
    hr();
    {
        std::ostringstream oss;
        oss << "| " << std::left << std::setw(static_cast<int>(nameW)) << "Pass Name"
            << " | " << std::right << std::setw(6) << "Raw"
            << " | " << std::right << std::setw(8) << "Added"
            << " | " << std::right << std::setw(10) << "Time(ms)"
            << " |";
        LOG(INFO, oss.str());
    }
    hr();

    for (const auto& s : g_passStats) 
    {
        std::string n = s.name;
        if (n.size() > nameW)
        {
            if (nameW > 3) n = n.substr(0, nameW - 3) + "...";
            else n = n.substr(0, nameW);
        }
        std::ostringstream oss;
        oss << "| " << std::left << std::setw(static_cast<int>(nameW)) << n
            << " | " << std::right << std::setw(6) << s.raw
            << " | " << std::right << std::setw(8) << s.added
            << " | " << std::right << std::setw(10) << std::fixed << std::setprecision(3) << s.ms
            << " |";
        LOG(INFO, oss.str());
    }
    hr();
    int totalRaw = 0;
    int totalAdded = 0;
    double totalMs = 0.0;
    for (const auto& s : g_passStats) 
    {
        totalRaw += s.raw;
        totalAdded += s.added;
        totalMs += s.ms;
    }
    {
        std::ostringstream oss;
        oss << "| " << std::left << std::setw(static_cast<int>(nameW)) << "TOTAL"
            << " | " << std::right << std::setw(6) << totalRaw
            << " | " << std::right << std::setw(8) << totalAdded
            << " | " << std::right << std::setw(10) << std::fixed << std::setprecision(3) << totalMs
            << " |";
        LOG(INFO, oss.str());
    }
    hr();
}

// Helper: Print section header for console output
void print_section_header(const std::string& title)
{
    const int width = 100;
    std::string line(width, '=');
    LOG(INFO, "\n" << line);
    int pad = (width - static_cast<int>(title.size()) - 2);
    if (pad < 0) pad = 0;
    int left = pad / 2;
    int right = pad - left;
    LOG(INFO, std::string(left, ' ') << "[" << title << "]" << std::string(right, ' '));
    LOG(INFO, line);
}

void print_results_table(const std::vector<QRResult>& results)
{
    if (results.empty()) 
    {
        LOG(INFO, "[RESULTS] No decoded QR strings.");
        return;
    }

    // Copy + sort for stable presentation
    std::vector<std::string> values;
    values.reserve(results.size());
    for (const auto& r : results)
        values.push_back(r.text);
    std::sort(values.begin(), values.end());

    // Layout config
    constexpr size_t kCols = 3; // multi-column output
    size_t textW = 7; // "Decoded"
    for (const auto& v : values)
        textW = std::max(textW, v.size());
    textW = std::min<size_t>(textW + 4, 40); // allow room for numbering

    const size_t rows = (values.size() + kCols - 1) / kCols;

    auto hr = [&]() 
    {
        std::ostringstream oss;
        for (size_t c = 0; c < kCols; ++c)
            oss << '+' << std::string(textW + 2, '-');
        oss << '+';
        LOG(INFO, oss.str());
    };

    print_section_header("RESULTS TABLE");
    hr();
    {
        std::ostringstream oss;
        for (size_t c = 0; c < kCols; ++c)
            oss << "| " << std::left << std::setw(static_cast<int>(textW)) << (std::string("Decoded #") + std::to_string(c+1)) << ' ';
        oss << '|';
        LOG(INFO, oss.str());
    }
    hr();

    for (size_t r = 0; r < rows; ++r) 
    {
        std::ostringstream oss;
        for (size_t c = 0; c < kCols; ++c) 
        {
            const size_t idx = c * rows + r; // column-major fill for compactness
            std::string cell;
            if (idx < values.size()) 
            {
                cell = std::to_string(idx) + ": " + values[idx];
                if (cell.size() > textW) 
                {
                    if (textW > 3) cell = cell.substr(0, textW - 3) + "...";
                    else cell = cell.substr(0, textW);
                }
            }
            oss << "| " << std::left << std::setw(static_cast<int>(textW)) << cell << ' ';
        }
        oss << '|';
        LOG(INFO, oss.str());
    }

    hr();
    LOG(INFO, "Total decoded: " << values.size());
}

void print_folder_summary_table(std::vector<AggregatePassStats>& agg,
                                       int imageCount,
                                       long long totalDecodedCnt,
                                       double totalDecodingTime)
{

    // sort by usefulness: added desc, then time asc
    std::sort(agg.begin(), agg.end(), [](const auto& a, const auto& b) 
    {
        if (a.added != b.added)
            return a.added > b.added;
        return a.ms < b.ms;
    });

    print_section_header("FOLDER SUMMARY");
    LOG(INFO, "Images processed              : " << imageCount);
    LOG(INFO, "Total QRs decoded (aggregate): " << totalDecodedCnt);

    if (agg.empty()) 
    {
        LOG(INFO, "No pass statistics available.");
        return;
    }

    size_t nameW = 9; // "Pass Name"
    for (const auto& a : agg)
        nameW = std::max(nameW, a.name.size());

    nameW = std::min<size_t>(nameW, 48);

    auto hr = [&]() 
    {
        std::ostringstream oss;
        oss << '+' << std::string(nameW + 2, '-')
            << '+' << std::string(8 + 2, '-')
            << '+' << std::string(8 + 2, '-')
            << '+' << std::string(12 + 2, '-')
            << '+' << std::string(11 + 2, '-')
            << '+' << std::string(11 + 2, '-') << '+';
        LOG(INFO, oss.str());
    };

    hr();
    {
        std::ostringstream oss;
        oss << "| " << std::left << std::setw(static_cast<int>(nameW)) << "Pass Name"
            << " | " << std::right << std::setw(8) << "RawTot"
            << " | " << std::right << std::setw(8) << "AddedTot"
            << " | " << std::right << std::setw(12) << "Avg Time(ms)"
            << " | " << std::right << std::setw(10) << "Img Contrib"
            << " | " << std::right << std::setw(11) << "Add/Img"
            << " |";
        LOG(INFO, oss.str());
    }
    hr();

    for (const auto& a : agg) 
    {
        std::string n = a.name;
        if (n.size() > nameW) 
        {
            if (nameW > 3) n = n.substr(0, nameW - 3) + "...";
            else n = n.substr(0, nameW);
        }
        double addPerImg = imageCount > 0 ? static_cast<double>(a.added) / imageCount : 0.0;
        double avgTimePerImg = imageCount > 0 ? a.ms / imageCount : 0.0;
        std::ostringstream oss;
        oss << "| " << std::left << std::setw(static_cast<int>(nameW)) << n
            << " | " << std::right << std::setw(8) << a.raw
            << " | " << std::right << std::setw(8) << a.added
            << " | " << std::right << std::setw(12) << std::fixed << std::setprecision(2) << avgTimePerImg
            << " | " << std::right << std::setw(9) << a.imagesContributed << "/" << imageCount
            << " | " << std::right << std::setw(11) << std::fixed << std::setprecision(2) << addPerImg
            << " |";
        LOG(INFO, oss.str());
    }
    hr();

    double avgDecodingTimePerImage = imageCount > 0 ? totalDecodingTime / imageCount : 0.0;
    LOG(INFO, "Avg Time for Complete Decoding per Image : " << avgDecodingTimePerImage << " ms" << std::endl);

    //No Longer Relevant after OpenMP multi-threading for decode stage

    // double avgPassTimePerImage = imageCount > 0 ? totalPassTime / imageCount : 0.0;
    // LOG(INFO, "Avg Time for Processing all Passes per Image : " << avgPassTimePerImage << " ms" << std::endl);

    // double avgTimeToApplyPass = avgDecodingTimePerImage - avgPassTimePerImage;
    // LOG(INFO, "Avg Time for Preprocessing per Image   : " << avgTimeToApplyPass << " ms" << std::endl);
}