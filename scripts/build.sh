#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PROJECT_NAME="pool-reference-cpp"
BUILD_DIR="build"
THIRD_PARTY_DIR="third_party"

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_dependencies() {
    print_info "Checking dependencies..."
    
    # Check C++ compiler
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        print_error "C++ compiler (g++ or clang++) not found"
        exit 1
    fi
    
    # Check Go
    if ! command -v go &> /dev/null; then
        print_error "Go compiler not found"
        exit 1
    fi
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found"
        exit 1
    fi
    
    # Check nasm for assembly optimizations
    if ! command -v nasm &> /dev/null; then
        print_warning "nasm not found - assembly optimizations will be disabled"
    fi
}

setup_third_party() {
    print_info "Setting up third-party dependencies..."
    
    # Initialize and update submodules if this is a git repository
    if [ -d ".git" ]; then
        git submodule init
        git submodule update
    fi
    
    # Build jsoncpp if not exists
    if [ ! -f "$THIRD_PARTY_DIR/jsoncpp/build/lib/libjsoncpp.a" ]; then
        print_info "Building jsoncpp..."
        mkdir -p $THIRD_PARTY_DIR/jsoncpp/build
        cd $THIRD_PARTY_DIR/jsoncpp/build
        cmake -DCMAKE_BUILD_TYPE=Release ..
        make -j$(nproc)
        cd ../../..
    fi
    
    # Build googletest if not exists
    if [ ! -f "$THIRD_PARTY_DIR/googletest/build/lib/libgtest.a" ]; then
        print_info "Building googletest..."
        mkdir -p $THIRD_PARTY_DIR/googletest/build
        cd $THIRD_PARTY_DIR/googletest/build
        cmake -DCMAKE_BUILD_TYPE=Release ..
        make -j$(nproc)
        cd ../../..
    fi
}

build_cpp() {
    local build_type=$1
    print_info "Building C++ components ($build_type)..."
    
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR
    
    # Configure CMake
    cmake -DCMAKE_BUILD_TYPE=$build_type \
          -DJSONCPP_ROOT=../$THIRD_PARTY_DIR/jsoncpp \
          -DGTEST_ROOT=../$THIRD_PARTY_DIR/googletest \
          ..
    
    # Build
    make -j$(nproc)
    
    cd ..
}

build_go() {
    print_info "Building Go components..."
    
    # Build Go bridge and components
    cd go
    go mod download
    go build -o ../$BUILD_DIR/pool_bridge ./pool_bridge.go
    go build -o ../$BUILD_DIR/pool_api ./api/server.go
    
    cd ..
}

build_asm() {
    if command -v nasm &> /dev/null; then
        print_info "Building assembly optimizations..."
        
        mkdir -p $BUILD_DIR/asm
        cd asm
        
        # Build x86-64 assembly files
        for asm_file in *.asm; do
            if [ -f "$asm_file" ]; then
                nasm -f elf64 -o ../$BUILD_DIR/asm/${asm_file%.asm}.o $asm_file
            fi
        done
        
        cd ..
    fi
}

main() {
    local build_type="Release"
    local clean_build=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -d|--debug)
                build_type="Debug"
                shift
                ;;
            -c|--clean)
                clean_build=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    print_info "Building $PROJECT_NAME ($build_type)"
    
    if [ "$clean_build" = true ]; then
        print_info "Performing clean build..."
        rm -rf $BUILD_DIR
    fi
    
    check_dependencies
    setup_third_party
    build_asm
    build_cpp $build_type
    build_go
    
    print_info "Build completed successfully!"
    print_info "Output files are in: $BUILD_DIR/"
    print_info "Libraries:"
    ls -la $BUILD_DIR/*.so $BUILD_DIR/*.a 2>/dev/null || echo "No libraries found"
    print_info "Executables:"
    ls -la $BUILD_DIR/pool_* 2>/dev/null || echo "No executables found"
}

main "$@"
