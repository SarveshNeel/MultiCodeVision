#pragma once

#include <filesystem>

namespace fs = std::filesystem;

bool has_image_extension(const fs::path& p);

// implemented in runner.cpp
void run_cli(const fs::path& input);