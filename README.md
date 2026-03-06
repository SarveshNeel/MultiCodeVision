MultiCodeVision/
│
├── CMakeLists.txt
├── cmake/                         # custom cmake modules
│   └── FindZXing.cmake
│
├── include/mcv/                   # public API
│
│   ├── core/
│   │   ├── pipeline.hpp
│   │   ├── config.hpp
│   │   └── types.hpp
│   │
│   ├── decode/
│   │   ├── decoder.hpp            # abstract decoder interface
│   │   ├── zxing_decoder.hpp
│   │   └── fallback_decoder.hpp
│   │
│   ├── preprocess/
│   │   ├── image_ops.hpp
│   │   └── warp.hpp
│   │
│   ├── output/
│   │   └── tables.hpp
│   │
│   └── util/
│       ├── timer.hpp
│       ├── logging.hpp
│       └── filesystem.hpp
│
│
├── src/                           # implementation
│
│   ├── core/
│   │   └── pipeline.cpp
│   │
│   ├── decode/
│   │   ├── zxing_decoder.cpp
│   │   └── fallback_decoder.cpp
│   │
│   ├── preprocess/
│   │   ├── image_ops.cpp
│   │   └── warp.cpp
│   │
│   ├── output/
│   │   └── tables.cpp
│   │
│   └── util/
│       ├── logging.cpp
│       └── filesystem.cpp
│
│
├── apps/                          # user-facing programs
│   └── cli/
│       ├── main.cpp
│       └── runner.cpp
│
│
├── tests/                         # unit tests
│   ├── test_pipeline.cpp
│   ├── test_decoder.cpp
│   └── test_dedup.cpp
│
│
├── benchmarks/                    # performance benchmarks
│   └── benchmark_pipeline.cpp
│
│
├── datasets/                      # local datasets (not committed)
│   ├── small_qr/
│   └── shiny_cans/
│
│
├── scripts/                       # automation tools
│   ├── run_batch.sh
│   └── export_stats.py
│
│
├── docs/
│   ├── architecture.md
│   └── pipeline_design.md
│
│
└── build/