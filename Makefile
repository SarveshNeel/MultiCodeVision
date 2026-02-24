# ===== Compiler =====
CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra

# ===== OpenCV =====
OPENCV_CFLAGS := $(shell pkg-config --cflags opencv4)
OPENCV_LIBS := $(shell pkg-config --libs opencv4)

# ===== Project =====
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
TARGET := $(BUILD_DIR)/app

# ===== Source files =====
SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# ===== Include flags =====
INCLUDES := -I$(INC_DIR) $(OPENCV_CFLAGS)

# ===== Default target =====
all: $(TARGET)

# ===== Link step =====
$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(OBJS) $(OPENCV_LIBS) -o $@

# ===== Compile step =====
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ===== Utilities =====
clean:
	rm -rf $(BUILD_DIR)

run: all
	./$(TARGET)

.PHONY: all clean run