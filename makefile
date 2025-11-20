# Makefile для сборки pool-reference

CC = gcc
CXX = g++
GO = go
ASM = nasm

CFLAGS = -O3 -march=native -fPIC -Wall -Wextra
CXXFLAGS = -O3 -march=native -fPIC -std=c++17 -Wall -Wextra
ASMFLAGS = -f elf64
GOTAGS = -tags=performance

INCLUDE_DIR = include
SRC_DIR = src
ASM_DIR = asm
GO_DIR = go
BUILD_DIR = build

C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
CXX_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
ASM_SOURCES = $(wildcard $(ASM_DIR)/*.asm)
GO_SOURCES = $(wildcard $(GO_DIR)/*.go)

C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
CXX_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CXX_SOURCES))
ASM_OBJS = $(patsubst $(ASM_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

TARGET = $(BUILD_DIR)/libpool.so
GO_TARGET = $(BUILD_DIR)/pool.a

all: $(TARGET) $(GO_TARGET)

$(TARGET): $(C_OBJS) $(CXX_OBJS) $(ASM_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) -shared -o $@ $^ -lm

$(GO_TARGET): $(GO_SOURCES)
	@mkdir -p $(BUILD_DIR)
	cd $(GO_DIR) && $(GO) build $(GOTAGS) -buildmode=c-archive -o ../$@ .

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

$(BUILD_DIR)/%.o: $(ASM_DIR)/%.asm
	@mkdir -p $(BUILD_DIR)
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)

test: all
	cd tests && $(GO) test -v

benchmark: all
	cd tests && $(GO) test -bench=. -benchmem

.PHONY: all clean test benchmark
