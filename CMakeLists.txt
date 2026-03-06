cmake_minimum_required(VERSION 3.16)

project(MultiCodeVision)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------
# Dependencies
# ------------------------------------------------

find_package(OpenCV REQUIRED)
find_package(ZXing REQUIRED)

# ------------------------------------------------
# Automatically collect sources
# ------------------------------------------------

file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS
    src/core/*.cpp
)

file(GLOB_RECURSE PREPROCESS_SRC CONFIGURE_DEPENDS
    src/preprocess/*.cpp
)

file(GLOB_RECURSE DECODE_SRC CONFIGURE_DEPENDS
    src/decode/*.cpp
)

file(GLOB_RECURSE OUTPUT_SRC CONFIGURE_DEPENDS
    src/output/*.cpp
)

file(GLOB_RECURSE UTIL_SRC CONFIGURE_DEPENDS
    src/util/*.cpp
)

set(MCV_CORE_SOURCES
    ${CORE_SRC}
    ${PREPROCESS_SRC}
    ${DECODE_SRC}
    ${OUTPUT_SRC}
    ${UTIL_SRC}
)

# ------------------------------------------------
# Core Library
# ------------------------------------------------

add_library(mcv_core
    ${MCV_CORE_SOURCES}
)

target_link_libraries(mcv_core
    PUBLIC
    ${OpenCV_LIBS}
    ZXing::ZXing
)

target_include_directories(mcv_core
    PUBLIC
    include
)

# ------------------------------------------------
# CLI Application
# ------------------------------------------------

file(GLOB CLI_SRC
    apps/cli/*.cpp
)

add_executable(app
    ${CLI_SRC}
)

target_link_libraries(app
    PRIVATE
    mcv_core
)

# ------------------------------------------------
# Compiler warnings
# ------------------------------------------------

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(mcv_core PRIVATE -Wall -Wextra -Wpedantic)
endif()