#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

BUILD_DIR="build"
TEST_DIR="tests"

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

run_cpp_tests() {
    print_info "Running C++ unit tests..."
    
    if [ ! -d "$BUILD_DIR" ]; then
        print_error "Build directory not found. Run build.sh first."
        exit 1
    fi
    
    cd $BUILD_DIR
    
    # Run math tests
    if [ -f "tests/math_test" ]; then
        print_info "Running math tests..."
        ./tests/math_test
    fi
    
    # Run pool logic tests
    if [ -f "tests/pool_test" ]; then
        print_info "Running pool logic tests..."
        ./tests/pool_test
    fi
    
    # Run security tests
    if [ -f "tests/security_test" ]; then
        print_info "Running security tests..."
        ./tests/security_test
    fi
    
    cd ..
}

run_go_tests() {
    print_info "Running Go tests..."
    
    if [ ! -f "go.mod" ]; then
        print_error "Go module not found"
        return 1
    fi
    
    cd go
    go test -v ./...
    cd ..
}

run_integration_tests() {
    print_info "Running integration tests..."
    
    if [ -f "go.mod" ]; then
        cd go
        go test -v -tags=integration ./integration_test.go
        cd ..
    else
        print_warning "Integration tests skipped - Go module not found"
    fi
}

run_coverage() {
    local coverage_type=$1
    
    case $coverage_type in
        "cpp")
            print_info "Generating C++ coverage report..."
            if command -v gcov &> /dev/null && command -v lcov &> /dev/null; then
                cd $BUILD_DIR
                lcov --capture --directory . --output-file coverage.info
                lcov --remove coverage.info '/usr/*' '*/third_party/*' --output-file coverage.info
                genhtml coverage.info --output-directory coverage_report
                print_info "C++ coverage report generated: $BUILD_DIR/coverage_report/index.html"
                cd ..
            else
                print_warning "gcov or lcov not installed - skipping C++ coverage"
            fi
            ;;
        "go")
            print_info "Generating Go coverage report..."
            if command -v go &> /dev/null; then
                cd go
                go test -coverprofile=coverage.out ./...
                go tool cover -html=coverage.out -o coverage.html
                print_info "Go coverage report generated: go/coverage.html"
                cd ..
            fi
            ;;
        "all")
            run_coverage "cpp"
            run_coverage "go"
            ;;
    esac
}

main() {
    local test_type="all"
    local coverage=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --cpp)
                test_type="cpp"
                shift
                ;;
            --go)
                test_type="go"
                shift
                ;;
            --integration)
                test_type="integration"
                shift
                ;;
            --coverage)
                coverage=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    print_info "Running tests ($test_type)"
    
    case $test_type in
        "cpp")
            run_cpp_tests
            ;;
        "go")
            run_go_tests
            ;;
        "integration")
            run_integration_tests
            ;;
        "all")
            run_cpp_tests
            run_go_tests
            run_integration_tests
            ;;
    esac
    
    if [ "$coverage" = true ]; then
        run_coverage "all"
    fi
    
    print_info "All tests completed successfully!"
}

main "$@"
