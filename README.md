HOW TO BUILD?

From project root, Configure Project:
    cmake -S . -B build

Compile:
    cmake --build build


EXECUTABLES GENERATED

Inside build/ you will get:

    build/
    ├── app


EXAMPLE RUN

./build/app datasets/mixed/real.bmp
                or
./build/app datasets/mixed --batch