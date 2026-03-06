#pragma once 

#include <filesystem>
#include <mcv/core/config.hpp>

void hash_combine(std::size_t& seed, std::size_t value);
std::size_t build_hash(std::size_t currentHash, BaseImage op, float scale);