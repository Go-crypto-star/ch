#!/bin/bash

# Скрипт сборки ассемблерных оптимизаций для Chia пула

set -e

echo "Сборка ассемблерных оптимизаций..."

# Создаем директории
mkdir -p ../build/asm

# Компилируем ассемблерные файлы с разными опциями оптимизации
echo "Компиляция sha256_extended.asm..."
nasm -f elf64 -O3 -D AVX2_SUPPORT -D SHA_EXTENDED ../asm/sha256_extended.asm -o ../build/asm/sha256_extended.o

echo "Компиляция bls_ops.asm..."
nasm -f elf64 -O3 -D AVX2_SUPPORT -D BLS_OPTIMIZED ../asm/bls_ops.asm -o ../build/asm/bls_ops.o

echo "Компиляция vector_ops.asm..."
nasm -f elf64 -O3 -D AVX2_SUPPORT -D AVX512_SUPPORT ../asm/vector_ops.asm -o ../build/asm/vector_ops.o

# Проверяем поддержку AVX-512
if grep -q avx512 /proc/cpuinfo; then
    echo "AVX-512 поддерживается, компилируем расширенные версии..."
    nasm -f elf64 -O3 -D AVX512_SUPPORT ../asm/vector_ops.asm -o ../build/asm/vector_ops_avx512.o
fi

echo "Ассемблерные оптимизации успешно скомпилированы!"

# Создаем статическую библиотеку
echo "Создание статической библиотеки..."
ar rcs ../build/asm/libpool_asm.a ../build/asm/*.o

echo "Статическая библиотека создана: ../build/asm/libpool_asm.a"
