HOW TO BUILD?

From project root, Configure Project:
    cmake -S . -B build -G Ninja

Compile:
    cmake --build build

EXAMPLE RUN

./runtime/windows/MultiCodeVisionCLI.exe datasets/mixed/real.bmp
                or
./runtime/windows/MultiCodeVisionCLI.exe datasets/mixed --batch