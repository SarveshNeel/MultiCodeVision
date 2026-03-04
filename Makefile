# ===== Compiler =====
CXX := g++

# ===== OpenCV =====
OPENCV_CFLAGS := $(shell pkg-config --cflags opencv4)
OPENCV_LIBS := $(shell pkg-config --libs opencv4)

# ===== ZXing =====
ZXING_CFLAGS := $(shell pkg-config --cflags zxing)
ZXING_LIBS := $(shell pkg-config --libs zxing)

# Build type: use DEBUG=1 for debugger-friendly builds
# Example: make clean && make DEBUG=1
ifeq ($(DEBUG),1)
CXXFLAGS := -std=c++17 -g -O0 -Wall -Wextra -fno-omit-frame-pointer $(OPENCV_CFLAGS) $(ZXING_CFLAGS)
else
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra $(OPENCV_CFLAGS) $(ZXING_CFLAGS)
endif

# ===== Project =====
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
TARGET := $(BUILD_DIR)/app

# ===== Source files =====
LIB_SRCS := $(shell find src -name '*.cpp')
APP_SRCS := $(shell find apps -name '*.cpp')

SRCS := $(LIB_SRCS) $(APP_SRCS)
OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

# ===== Include flags =====
INCLUDES := -I$(INC_DIR) $(OPENCV_CFLAGS) $(ZXING_CFLAGS)

# ===== Default target =====
all: $(TARGET)

# ===== Link step =====
$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(OBJS) $(OPENCV_LIBS) $(ZXING_LIBS) -o $@

# ===== Compile step =====
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ===== Utilities =====
clean:
	rm -rf $(BUILD_DIR)

run: all
	./$(TARGET)

# Debug convenience target
# Usage: make debug
# (performs a clean rebuild with debug symbols)
debug:
	$(MAKE) clean
	$(MAKE) DEBUG=1 all

.PHONY: all clean run debug