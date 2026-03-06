#include <mcv/util/hashing.hpp>

void hash_combine(std::size_t& seed, std::size_t value) 
{
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

std::size_t build_hash(std::size_t currentHash, BaseImage op, float scale)
{
    hash_combine(currentHash, std::hash<int>{}(static_cast<int>(op)));

    if (op == BaseImage::UPSCALE)
        hash_combine(currentHash, std::hash<float>{}(scale));

    return currentHash;
}