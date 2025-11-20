# Makefile for Chia Pool Reference Implementation

CC = gcc
CXX = g++
GO = go
CFLAGS = -Wall -Wextra -std=c99 -O2 -fPIC -I./include -I./third_party
CXXFLAGS = -Wall -Wextra -std=c++11 -O2 -fPIC -I./include -I./third_party
LDFLAGS = -lpthread -lcurl -lssl -lcrypto -lgmp -lm

# Директории
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
SRC_DIRS = src src/protocol src/blockchain src/security

# Источники
C_SOURCES = $(shell find src -name "*.c")
CPP_SOURCES = $(shell find src -name "*.cpp")
ASM_SOURCES = $(wildcard asm/*.asm)

# Объектные файлы с правильными путями
C_OBJECTS = $(C_SOURCES:%.c=$(OBJ_DIR)/%.o)
CPP_OBJECTS = $(CPP_SOURCES:%.cpp=$(OBJ_DIR)/%.o)
ASM_OBJECTS = $(ASM_SOURCES:asm/%.asm=$(OBJ_DIR)/asm/%.o)

# Цели
TARGET = $(BUILD_DIR)/libpool.so
STATIC_TARGET = $(BUILD_DIR)/libpool.a
GO_TARGET = $(BUILD_DIR)/pool.a

.PHONY: all clean test install debug asm_lib check_cpu

all: $(TARGET) $(STATIC_TARGET)

# Динамическая библиотека
$(TARGET): $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)
	@echo "Динамическая библиотека собрана: $@"

# Статическая библиотека
$(STATIC_TARGET): $(C_OBJECTS) $(CPP_OBJECTS) $(ASM_OBJECTS)
	@mkdir -p $(BUILD_DIR)
	ar rcs $@ $^
	@echo "Статическая библиотека собрана: $@"

# Go архив
go_build: $(TARGET)
	@mkdir -p $(BUILD_DIR)
	cd go && $(GO) build -buildmode=c-shared -o ../$(GO_TARGET) .
	@echo "Go архив собран: $(GO_TARGET)"

# Создание директорий для объектных файлов
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ассемблерные файлы
$(OBJ_DIR)/asm/%.o: asm/%.asm
	@mkdir -p $(dir $@)
	nasm -f elf64 -O3 $< -o $@

# Отдельная цель для ассемблерной библиотеки
asm_lib: $(ASM_OBJECTS)
	@mkdir -p $(BUILD_DIR)/asm
	ar rcs $(BUILD_DIR)/asm/libpool_asm.a $(ASM_OBJECTS)
	@echo "Ассемблерная библиотека создана: $(BUILD_DIR)/asm/libpool_asm.a"

# Проверка поддержки CPU
check_cpu:
	@echo "Проверка поддержки инструкций CPU:"
	@if grep -q avx /proc/cpuinfo 2>/dev/null; then \
		echo "✓ AVX поддерживается"; \
	else \
		echo "✗ AVX не поддерживается"; \
	fi
	@if grep -q avx2 /proc/cpuinfo 2>/dev/null; then \
		echo "✓ AVX2 поддерживается"; \
	else \
		echo "✗ AVX2 не поддерживается"; \
	fi
	@if grep -q avx512 /proc/cpuinfo 2>/dev/null; then \
		echo "✓ AVX-512 поддерживается"; \
	else \
		echo "✗ AVX-512 не поддерживается"; \
	fi

# Очистка
clean:
	rm -rf $(BUILD_DIR)
	@echo "Проект очищен"

# Установка
install: $(TARGET)
	cp $(TARGET) /usr/local/lib/
	cp -r include/* /usr/local/include/
	@echo "Библиотека установлена в /usr/local"

# Тесты
test: all
	$(MAKE) -C tests all

# Отладочная сборка
debug: CFLAGS += -g -DDEBUG -O0
debug: CXXFLAGS += -g -DDEBUG -O0
debug: all
	@echo "Отладочная сборка завершена"

# Release сборка с максимальной оптимизацией
release: CFLAGS += -O3 -march=native
release: CXXFLAGS += -O3 -march=native
release: all
	@echo "Release сборка с оптимизацией завершена"

# Показ информации о проекте
info:
	@echo "=== Chia Pool Reference Implementation ==="
	@echo "C источники: $(words $(C_SOURCES)) файлов"
	@echo "C++ источники: $(words $(CPP_SOURCES)) файлов" 
	@echo "ASM источники: $(words $(ASM_SOURCES)) файлов"
	@echo "Цели: $(TARGET) $(STATIC_TARGET)"
	@echo "Флаги компиляции C: $(CFLAGS)"
	@echo "Флаги компиляции C++: $(CXXFLAGS)"

# Зависимости
deps:
	@echo "Установка системных зависимостей..."
	sudo apt-get update
	sudo apt-get install -y build-essential cmake nasm golang-go \
		libcurl4-openssl-dev libssl-dev libgmp-dev pkg-config

# Помощь
help:
	@echo "Доступные цели:"
	@echo "  all       - Сборка всех библиотек"
	@echo "  clean     - Очистка проекта"
	@echo "  debug     - Сборка с отладочной информацией"
	@echo "  release   - Release сборка с оптимизацией"
	@echo "  test      - Запуск тестов"
	@echo "  install   - Установка библиотек в систему"
	@echo "  go_build  - Сборка Go компонентов"
	@echo "  asm_lib   - Сборка ассемблерной библиотеки"
	@echo "  check_cpu - Проверка поддержки CPU инструкций"
	@echo "  info      - Информация о проекте"
	@echo "  deps      - Установка зависимостей"
	@echo "  help      - Эта справка"
