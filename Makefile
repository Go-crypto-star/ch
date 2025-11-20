# Makefile for Chia Pool Reference Implementation

CC = gcc
CXX = g++
GO = go
CFLAGS = -Wall -Wextra -std=c99 -O2 -fPIC -I./include
CXXFLAGS = -Wall -Wextra -std=c++11 -O2 -fPIC -I./include
LDFLAGS = -lpthread -lcurl -lm

# Источники
C_SOURCES = $(wildcard src/*.c) $(wildcard src/protocol/*.c) $(wildcard src/blockchain/*.c) $(wildcard src/security/*.c)
CPP_SOURCES = $(wildcard src/*.cpp) $(wildcard src/protocol/*.cpp) $(wildcard src/blockchain/*.cpp) $(wildcard src/security/*.cpp)
ASM_SOURCES = $(wildcard asm/*.asm)

# Объектные файлы
C_OBJECTS = $(C_SOURCES:.c=.o)
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)
ASM_OBJECTS = $(ASM_SOURCES:.asm=.o)

# Цели
TARGET = build/libpool.so
STATIC_TARGET = build/libpool.a
GO_TARGET = build/pool.a

.PHONY: all clean test install

all: $(TARGET) $(STATIC_TARGET) $(GO_TARGET)

# Динамическая библиотека
$(TARGET): $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)
	@mkdir -p build
	$(CXX) -shared -o $@ $^ $(LDFLAGS)
	@echo "Динамическая библиотека собрана: $@"

# Статическая библиотека
$(STATIC_TARGET): $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)
	@mkdir -p build
	ar rcs $@ $^
	@echo "Статическая библиотека собрана: $@"

# Go архив
$(GO_TARGET): $(TARGET)
	@mkdir -p build
	$(GO) build -buildmode=c-shared -o $@ ./go
	@echo "Go архив собран: $@"

# Компиляция C файлов
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция C++ файлов
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ассемблерные файлы (нужна настройка под конкретный ассемблер)
%.o: %.asm
	nasm -f elf64 $< -o $@

# Очистка
clean:
	rm -f $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS) $(TARGET) $(STATIC_TARGET) $(GO_TARGET)
	rm -rf build

# Установка
install: $(TARGET)
	cp $(TARGET) /usr/local/lib/
	cp include/*.h /usr/local/include/
	cp -r include/protocol /usr/local/include/
	cp -r include/blockchain /usr/local/include/
	cp -r include/security /usr/local/include/
	@echo "Библиотека установлена"

# Тесты
test: all
	$(MAKE) -C tests all

# Отладочная сборка
debug: CFLAGS += -g -DDEBUG
debug: CXXFLAGS += -g -DDEBUG
debug: all

.PHONY: all clean test install debug
