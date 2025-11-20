#!/bin/bash

# Скрипт полной сборки Chia Pool

set -e

echo "=== Сборка Chia Pool Reference Implementation ==="

# Проверка зависимостей
echo "Проверка зависимостей..."
command -v gcc >/dev/null 2>&1 || { echo "Ошибка: gcc не установлен"; exit 1; }
command -v g++ >/dev/null 2>&1 || { echo "Ошибка: g++ не установлен"; exit 1; }
command -v nasm >/dev/null 2>&1 || { echo "Ошибка: nasm не установлен"; exit 1; }
command -v go >/dev/null 2>&1 || { echo "Ошибка: go не установлен"; exit 1; }

# Создание директорий
mkdir -p build/obj/{src,protocol,blockchain,security,asm}

# Проверка CPU
echo "=== Проверка поддержки CPU ==="
./scripts/check_cpu.sh

# Сборка
echo "=== Начало сборки ==="

# Сборка ассемблерных оптимизаций
echo "Сборка ассемблерных оптимизаций..."
make asm_lib

# Основная сборка
echo "Основная сборка..."
make release

# Сборка Go компонентов
echo "Сборка Go компонентов..."
make go_build

# Проверка результатов
echo "=== Проверка результатов сборки ==="
if [ -f "build/libpool.so" ]; then
    echo "✓ Динамическая библиотека создана: build/libpool.so"
    file build/libpool.so
else
    echo "✗ Ошибка: Динамическая библиотека не создана"
    exit 1
fi

if [ -f "build/libpool.a" ]; then
    echo "✓ Статическая библиотека создана: build/libpool.a"
else
    echo "✗ Ошибка: Статическая библиотека не создана"
    exit 1
fi

if [ -f "build/pool.a" ]; then
    echo "✓ Go архив создан: build/pool.a"
else
    echo "⚠ Go архив не создан (требуются Go зависимости)"
fi

echo ""
echo "=== Сборка успешно завершена! ==="
echo "Библиотеки находятся в директории build/"
