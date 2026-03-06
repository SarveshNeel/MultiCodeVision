#include <string>
#include <filesystem>
#include <mcv/util/filesystem.hpp>

namespace fs = std::filesystem;

bool has_image_extension(const fs::path& p) 
{
    const std::string ext = p.extension().string();
    if (ext.empty()) return false;
    std::string e;
    e.reserve(ext.size());
    for (char c : ext) e.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return e == ".jpg" || e == ".jpeg" || e == ".png" || e == ".bmp" || e == ".tif" || e == ".tiff" || e == ".webp";
}