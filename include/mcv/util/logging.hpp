#pragma once

#include <iostream>

enum LogLevel {
    ERROR,
    INFO,
    DEBUG,
    TRACE
};

#define LOG(level, msg) std::cout << msg << std::endl;