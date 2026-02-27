MultiCodeVision/
│
├── CMakeLists.txt
│
├── include/
│   └── mcv/                     <-- public API (library headers)
│       ├── core/
│       │   ├── types.hpp
│       │   ├── pipeline.hpp
│       │   └── config.hpp
│       │
│       ├── decode/
│       │   ├── zxing_bridge.hpp
│       │   └── fallback_decoder.hpp
│       │
│       ├── output/
│       │   └── tables.hpp
│       │
│       └── util/
│           ├── timer.hpp
│           └── logging.hpp
│
├── src/                         <-- library implementation only
│   ├── core/
│   │   └── pipeline.cpp
│   │
│   ├── decode/
│   │   ├── zxing_bridge.cpp
│   │   └── fallback_decoder.cpp
│   │
│   ├── output/
│   │   └── tables.cpp
│   │
│   └── util/
│       └── logging.cpp
│
├── apps/                        <-- executables (CLI tools)
│   └── cli/
│       ├── main.cpp
│       └── runner.cpp
│
├── tests/                       <-- unit tests
│   ├── test_pipeline.cpp
│   └── test_dedup.cpp
│
├── benchmarks/                  <-- performance tests
│   └── benchmark_pipeline.cpp
│
├── datasets/                    <-- optional local datasets
│   ├── small_qr/
│   └── shiny_cans/
│
├── scripts/                     <-- automation
│   ├── run_batch.sh
│   └── export_stats.py
│
└── docs/
    └── architecture.md