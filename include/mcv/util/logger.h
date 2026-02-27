#pragma once

enum LogLevel {
    ERROR,
    INFO,
    DEBUG,
    TRACE
};

LogLevel LOG_LEVEL = INFO;

#define LOG(level, msg) if ((level) <= LOG_LEVEL) { std::cout << msg << std::endl; }
