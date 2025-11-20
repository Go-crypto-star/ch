#!/bin/bash

# Скрипт установки зависимостей

set -e

echo "=== Установка зависимостей для Chia Pool ==="

# Определение дистрибутива
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "Не удалось определить дистрибутив"
    exit 1
fi

echo "Обнаружен дистрибутив: $OS"

case $OS in
    ubuntu|debian)
        echo "Установка для Ubuntu/Debian..."
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            nasm \
            golang-go \
            libcurl4-openssl-dev \
            libssl-dev \
            libgmp-dev \
            pkg-config \
            autoconf \
            automake \
            libtool \
            git
        ;;
    centos|rhel|fedora)
        echo "Установка для CentOS/RHEL/Fedora..."
        sudo yum groupinstall -y "Development Tools"
        sudo yum install -y \
            cmake \
            nasm \
            golang \
            libcurl-devel \
            openssl-devel \
            gmp-devel \
            pkgconfig \
            autoconf \
            automake \
            libtool \
            git
        ;;
    arch|manjaro)
        echo "Установка для Arch/Manjaro..."
        sudo pacman -Syu --noconfirm \
            base-devel \
            cmake \
            nasm \
            go \
            curl \
            openssl \
            gmp \
            pkg-config \
            autoconf \
            automake \
            libtool \
            git
        ;;
    *)
        echo "Неизвестный дистрибутив: $OS"
        echo "Пожалуйста, установите зависимости вручную:"
        echo " - build-essential / base-devel"
        echo " - cmake, nasm, golang"
        echo " - libcurl, openssl, gmp development packages"
        exit 1
        ;;
esac

echo "=== Зависимости успешно установлены ==="
