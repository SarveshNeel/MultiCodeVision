cmake_minimum_required(VERSION 3.20)

project(MultiCodeVision VERSION 1.0 LANGUAGES CXX)

# ------------------------------------------------
# C++ standard
# ------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------
# Output directories
# ------------------------------------------------

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ------------------------------------------------
# Dependency paths
# ------------------------------------------------

set(OPENCV_ROOT "C:/deps/opencv")
set(ZXING_ROOT  "C:/deps/zxing-cpp")

set(OPENCV_3RDPARTY_DIR "${OPENCV_ROOT}/build/3rdparty/lib")

set(ZLIB_LIB     "${OPENCV_3RDPARTY_DIR}/libzlib.a")
set(PNG_LIB      "${OPENCV_3RDPARTY_DIR}/liblibpng.a")
set(JPEG_LIB     "${OPENCV_3RDPARTY_DIR}/liblibjpeg-turbo.a")
set(OPENJPEG_LIB "${OPENCV_3RDPARTY_DIR}/liblibopenjp2.a")

set(OPENCV_LIB "${OPENCV_ROOT}/build/lib/libopencv_world4140.a")
set(ZXING_LIB  "${ZXING_ROOT}/build/core/libZXing.a")

include_directories(${ZXING_ROOT}/core/src)
include_directories(${ZXING_ROOT}/build/core)

# ------------------------------------------------
# OpenMP
# ------------------------------------------------

find_package(OpenMP REQUIRED)

# ------------------------------------------------
# Collect sources
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

file(GLOB_RECURSE API_SRC CONFIGURE_DEPENDS
    src/api/*.cpp
)

file(GLOB CLI_SRC CONFIGURE_DEPENDS
    apps/cli/*.cpp
)

# ------------------------------------------------
# LensCore (Static library)
# ------------------------------------------------

add_library(LensCore STATIC
    ${CORE_SRC}
    ${PREPROCESS_SRC}
    ${DECODE_SRC}
    ${OUTPUT_SRC}
    ${UTIL_SRC}
)

target_include_directories(LensCore
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${OPENCV_ROOT}/include
        ${ZXING_ROOT}/include
)

target_link_libraries(LensCore
    PUBLIC
        -Wl,--whole-archive
        ${OPENCV_LIB}
        ${ZXING_LIB}
        ${ZLIB_LIB}
        ${PNG_LIB}
        ${JPEG_LIB}
        ${OPENJPEG_LIB}
        -Wl,--no-whole-archive
        OpenMP::OpenMP_CXX
)

target_compile_options(LensCore PRIVATE -Wall -Wextra -Wpedantic)

target_compile_features(LensCore PUBLIC cxx_std_20)

# ------------------------------------------------
# Lens DLL (API for C#)
# ------------------------------------------------

add_library(Lens SHARED
    ${API_SRC}
)

set_target_properties(Lens PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_compile_definitions(Lens PRIVATE LENS_EXPORTS)

target_include_directories(Lens
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Lens
    PRIVATE
        LensCore
)

target_link_options(Lens PRIVATE
    -static-libgcc
    -static-libstdc++
)

# ------------------------------------------------
# CLI Application
# ------------------------------------------------

add_executable(MultiCodeVisionCLI
    ${CLI_SRC}
)

target_link_libraries(MultiCodeVisionCLI
    PRIVATE
        LensCore
)

target_include_directories(MultiCodeVisionCLI
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(MultiCodeVisionCLI PRIVATE cxx_std_20)

# ------------------------------------------------
# Runtime copy
# ------------------------------------------------

set(MCV_RUNTIME_DIR ${CMAKE_SOURCE_DIR}/runtime/windows)

add_custom_command(TARGET Lens POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MCV_RUNTIME_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:Lens> ${MCV_RUNTIME_DIR}/
)

add_custom_command(TARGET MultiCodeVisionCLI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MCV_RUNTIME_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:MultiCodeVisionCLI> ${MCV_RUNTIME_DIR}/
)