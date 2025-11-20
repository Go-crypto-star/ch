#!/bin/bash

# Скрипт проверки поддержки CPU инструкций

echo "=== Проверка поддержки CPU инструкций ==="

# Проверка AVX
if grep -q avx /proc/cpuinfo; then
    echo "✓ AVX поддерживается"
    AVX_SUPPORT=1
else
    echo "✗ AVX не поддерживается"
    AVX_SUPPORT=0
fi

# Проверка AVX2
if grep -q avx2 /proc/cpuinfo; then
    echo "✓ AVX2 поддерживается"
    AVX2_SUPPORT=1
else
    echo "✗ AVX2 не поддерживается"
    AVX2_SUPPORT=0
fi

# Проверка AVX-512
if grep -q avx512 /proc/cpuinfo; then
    echo "✓ AVX-512 поддерживается"
    AVX512_SUPPORT=1
else
    echo "✗ AVX-512 не поддерживается"
    AVX512_SUPPORT=0
fi

# Проверка SSE4.2
if grep -q sse4_2 /proc/cpuinfo; then
    echo "✓ SSE4.2 поддерживается"
    SSE42_SUPPORT=1
else
    echo "✗ SSE4.2 не поддерживается"
    SSE42_SUPPORT=0
fi

# Рекомендации
echo ""
echo "=== Рекомендации ==="

if [ $AVX2_SUPPORT -eq 1 ]; then
    echo "✓ Ваш CPU поддерживает AVX2 - будут использованы все оптимизации"
elif [ $AVX_SUPPORT -eq 1 ]; then
    echo "⚠ Ваш CPU поддерживает только AVX - некоторые оптимизации недоступны"
else
    echo "⚠ Ваш CPU не поддерживает AVX - производительность будет ограничена"
fi

if [ $AVX512_SUPPORT -eq 1 ]; then
    echo "✓ Ваш CPU поддерживает AVX-512 - будут использованы расширенные оптимизации"
fi
