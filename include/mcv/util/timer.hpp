#pragma once

#include <string>
#include <chrono>
#include <mcv/util/logging.hpp>

struct ScopedTimer 
{
    std::string name;
    std::chrono::steady_clock::time_point start;

    ScopedTimer(const std::string &n) : name(n), start(std::chrono::steady_clock::now()) {}

    ~ScopedTimer() 
    {
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        LOG(DEBUG, "[TIME] " << name << ": " << ms << " ms");
    }
};

inline double elapsed_ms(const std::chrono::steady_clock::time_point& a,
                                const std::chrono::steady_clock::time_point& b)
{
    return std::chrono::duration<double, std::milli>(b - a).count();
}