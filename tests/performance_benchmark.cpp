#include "pool_core.h"
#include "math_operations.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>

class Benchmark {
private:
    std::mt19937_64 rng;
    std::uniform_int_distribution<uint64_t> dist;
    
public:
    Benchmark() : rng(std::random_device{}()), dist(0, 0xFFFFFFFFFFFFFFFF) {}
    
    void random_u256(uint256_t* value) {
        for (int i = 0; i < 4; i++) {
            ((uint64_t*)value->data)[i] = dist(rng);
        }
    }
    
    template<typename Func>
    double measure(int iterations, Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            func();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        return diff.count() / iterations;
    }
    
    void benchmark_arithmetic() {
        printf("Benchmarking arithmetic operations...\n");
        
        const int ITERATIONS = 100000;
        uint256_t a, b, result;
        
        random_u256(&a);
        random_u256(&b);
        
        // Benchmark addition
        double add_time = measure(ITERATIONS, [&]() {
            asm_u256_add(&a, &b, &result);
        });
        printf("u256_add: %.2f ns/op\n", add_time * 1e9);
        
        // Benchmark subtraction
        double sub_time = measure(ITERATIONS, [&]() {
            asm_u256_sub(&a, &b, &result);
        });
        printf("u256_sub: %.2f ns/op\n", sub_time * 1e9);
        
        // Benchmark multiplication
        double mul_time = measure(ITERATIONS, [&]() {
            asm_u256_mul(&a, &b, &result);
        });
        printf("u256_mul: %.2f ns/op\n", mul_time * 1e9);
        
        // Benchmark division (only if b != 0)
        if (!u256_is_zero(&b)) {
            uint256_t quotient, remainder;
            double div_time = measure(ITERATIONS / 10, [&]() {
                asm_u256_div(&a, &b, &quotient, &remainder);
            });
            printf("u256_div: %.2f ns/op\n", div_time * 1e9);
        }
        
        printf("\n");
    }
    
    void benchmark_pool_operations() {
        printf("Benchmarking pool operations...\n");
        
        const int ITERATIONS = 10000;
        
        // Setup pool
        PoolState pool;
        address_t token0, token1;
        memset(&token0, 0xAA, sizeof(token0));
        memset(&token1, 0xBB, sizeof(token1));
        pool_initialize(&pool, token0, token1, 300);
        
        // Add initial liquidity
        address_t user;
        memset(&user, 0xCC, sizeof(user));
        uint256_t amount0, amount1, liquidity;
        ((uint64_t*)amount0.data)[0] = 1000000000000000000ULL;
        ((uint64_t*)amount1.data)[0] = 3000000000ULL;
        pool_mint(&pool, user, amount0, amount1, &liquidity);
        
        // Benchmark mint
        double mint_time = measure(ITERATIONS, [&]() {
            uint256_t small_amount0, small_amount1, out_liquidity;
            ((uint64_t*)small_amount0.data)[0] = 1000000000000000ULL;
            ((uint64_t*)small_amount1.data)[0] = 3000ULL;
            pool_mint(&pool, user, small_amount0, small_amount1, &out_liquidity);
        });
        printf("pool_mint: %.2f ns/op\n", mint_time * 1e9);
        
        // Benchmark swap
        double swap_time = measure(ITERATIONS, [&]() {
            uint256_t amount0_out, amount1_out;
            ((uint64_t*)amount1_out.data)[0] = 1000ULL;
            pool_swap(&pool, amount0_out, amount1_out, user);
        });
        printf("pool_swap: %.2f ns/op\n", swap_time * 1e9);
        
        // Benchmark get_amount_out
        double amount_out_time = measure(ITERATIONS, [&]() {
            uint256_t amount_in, reserve_in, reserve_out, result;
            ((uint64_t*)amount_in.data)[0] = 1000000000000000ULL;
            reserve_in = pool.reserve0;
            reserve_out = pool.reserve1;
            result = pool_get_amount_out(amount_in, reserve_in, reserve_out, 300);
        });
        printf("get_amount_out: %.2f ns/op\n", amount_out_time * 1e9);
        
        printf("\n");
    }
};

int main() {
    Benchmark bench;
    
    printf("Performance Benchmark Results\n");
    printf("=============================\n\n");
    
    bench.benchmark_arithmetic();
    bench.benchmark_pool_operations();
    
    return 0;
}
