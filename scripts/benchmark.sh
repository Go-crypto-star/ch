#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

BUILD_DIR="build"
BENCHMARK_EXEC="tests/performance_benchmark"

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_requirements() {
    if [ ! -d "$BUILD_DIR" ]; then
        print_error "Build directory not found. Run build.sh first."
        exit 1
    fi
    
    if [ ! -f "$BUILD_DIR/$BENCHMARK_EXEC" ]; then
        print_error "Benchmark executable not found: $BUILD_DIR/$BENCHMARK_EXEC"
        exit 1
    fi
}

run_performance_benchmarks() {
    local benchmark_type=$1
    
    print_info "Running performance benchmarks ($benchmark_type)..."
    
    cd $BUILD_DIR
    
    case $benchmark_type in
        "math")
            ./$BENCHMARK_EXEC --gtest_filter="MathOperations*"
            ;;
        "security")
            ./$BENCHMARK_EXEC --gtest_filter="Security*"
            ;;
        "pool")
            ./$BENCHMARK_EXEC --gtest_filter="PoolOperations*"
            ;;
        "all")
            ./$BENCHMARK_EXEC
            ;;
    esac
    
    cd ..
}

run_memory_benchmark() {
    print_info "Running memory usage benchmark..."
    
    if command -v valgrind &> /dev/null; then
        cd $BUILD_DIR
        valgrind --tool=massif --massif-out-file=massif.out ./$BENCHMARK_EXEC --gtest_filter="MemoryIntensive*"
        print_info "Memory profile saved: $BUILD_DIR/massif.out"
        cd ..
    else
        print_warning "valgrind not installed - skipping memory benchmark"
    fi
}

run_scalability_test() {
    local max_threads=$1
    local step=$2
    
    print_info "Running scalability test (1 to $max_threads threads)..."
    
    cd $BUILD_DIR
    
    for ((threads=1; threads<=max_threads; threads+=step)); do
        print_info "Testing with $threads threads..."
        ./$BENCHMARK_EXEC --gtest_filter="ScalabilityTest*" --threads=$threads
    done
    
    cd ..
}

run_go_benchmarks() {
    print_info "Running Go benchmarks..."
    
    if [ -f "go.mod" ]; then
        cd go
        go test -bench=. -benchmem ./...
        cd ..
    else
        print_warning "Go benchmarks skipped - Go module not found"
    fi
}

generate_report() {
    local report_file="benchmark_report_$(date +%Y%m%d_%H%M%S).md"
    
    print_info "Generating benchmark report: $report_file"
    
    cat > $report_file << EOF
# Performance Benchmark Report
Generated: $(date)

## System Information
- CPU: $(lscpu | grep "Model name" | cut -d: -f2 | sed 's/^ *//')
- Cores: $(nproc)
- Memory: $(free -h | grep Mem | awk '{print $2}')
- OS: $(uname -srm)

## Benchmark Results

### C++ Performance
\`\`\`
$(cd $BUILD_DIR && ./$BENCHMARK_EXEC --gtest_filter="Performance*" 2>/dev/null || echo "Performance benchmarks not available")
\`\`\`

### Go Performance
\`\`\`
$(cd go && go test -bench=. -benchmem ./... 2>/dev/null || echo "Go benchmarks not available")
\`\`\`

## Recommendations
- Consider optimizations for identified bottlenecks
- Review memory usage patterns
- Evaluate thread scalability
EOF

    print_info "Benchmark report saved: $report_file"
}

main() {
    local benchmark_type="all"
    local scalability=false
    local memory=false
    local report=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --math)
                benchmark_type="math"
                shift
                ;;
            --security)
                benchmark_type="security"
                shift
                ;;
            --pool)
                benchmark_type="pool"
                shift
                ;;
            --scalability)
                scalability=true
                shift
                ;;
            --memory)
                memory=true
                shift
                ;;
            --report)
                report=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    check_requirements
    
    print_info "Starting benchmarks..."
    
    run_performance_benchmarks $benchmark_type
    run_go_benchmarks
    
    if [ "$scalability" = true ]; then
        run_scalability_test 16 2
    fi
    
    if [ "$memory" = true ]; then
        run_memory_benchmark
    fi
    
    if [ "$report" = true ]; then
        generate_report
    fi
    
    print_info "Benchmarks completed successfully!"
}

main "$@"
