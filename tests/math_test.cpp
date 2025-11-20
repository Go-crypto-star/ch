#include "pool_core.h"
#include "math_operations.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>

// Вспомогательные функции для тестирования
void print_u256(const char* label, const uint256_t* value) {
    printf("%s: 0x", label);
    for (int i = 3; i >= 0; i--) {
        printf("%016lx", ((uint64_t*)value->data)[i]);
    }
    printf("\n");
}

bool u256_equal(const uint256_t* a, const uint256_t* b) {
    return memcmp(a->data, b->data, 32) == 0;
}

void test_u256_add() {
    printf("Testing u256_add...\n");
    
    uint256_t a, b, result, expected;
    
    // Test 1: Basic addition
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 100;
    ((uint64_t*)b.data)[0] = 200;
    ((uint64_t*)expected.data)[0] = 300;
    
    asm_u256_add(&a, &b, &result);
    assert(u256_equal(&result, &expected));
    printf("Test 1 passed: 100 + 200 = 300\n");
    
    // Test 2: Addition with carry
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 0xFFFFFFFFFFFFFFFF;
    ((uint64_t*)b.data)[0] = 1;
    ((uint64_t*)expected.data)[0] = 0;
    ((uint64_t*)expected.data)[1] = 1;
    
    asm_u256_add(&a, &b, &result);
    assert(u256_equal(&result, &expected));
    printf("Test 2 passed: Overflow handled correctly\n");
    
    // Test 3: Large numbers
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[3] = 1000;
    ((uint64_t*)b.data)[3] = 2000;
    ((uint64_t*)expected.data)[3] = 3000;
    
    asm_u256_add(&a, &b, &result);
    assert(u256_equal(&result, &expected));
    printf("Test 3 passed: Large numbers addition\n");
    
    printf("All u256_add tests passed!\n\n");
}

void test_u256_sub() {
    printf("Testing u256_sub...\n");
    
    uint256_t a, b, result, expected;
    
    // Test 1: Basic subtraction
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 300;
    ((uint64_t*)b.data)[0] = 200;
    ((uint64_t*)expected.data)[0] = 100;
    
    asm_u256_sub(&a, &b, &result);
    assert(u256_equal(&result, &expected));
    printf("Test 1 passed: 300 - 200 = 100\n");
    
    // Test 2: Subtraction with borrow
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[1] = 1;
    ((uint64_t*)b.data)[0] = 1;
    ((uint64_t*)expected.data)[0] = 0xFFFFFFFFFFFFFFFF;
    ((uint64_t*)expected.data)[1] = 0;
    
    asm_u256_sub(&a, &b, &result);
    assert(u256_equal(&result, &expected));
    printf("Test 2 passed: Borrow handled correctly\n");
    
    // Test 3: Underflow
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 100;
    ((uint64_t*)b.data)[0] = 200;
    
    uint8_t underflow = asm_u256_sub(&a, &b, &result);
    assert(underflow == 1);
    printf("Test 3 passed: Underflow detected\n");
    
    printf("All u256_sub tests passed!\n\n");
}

void test_u256_mul() {
    printf("Testing u256_mul...\n");
    
    uint256_t a, b, result, expected;
    
    // Test 1: Basic multiplication
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 100;
    ((uint64_t*)b.data)[0] = 200;
    ((uint64_t*)expected.data)[0] = 20000;
    
    asm_u256_mul(&a, &b, &result);
    assert(u256_equal(&result, &expected));
    printf("Test 1 passed: 100 * 200 = 20000\n");
    
    // Test 2: Multiplication with overflow
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 0x100000000;
    ((uint64_t*)b.data)[0] = 0x100000000;
    ((uint64_t*)expected.data)[0] = 0;
    ((uint64_t*)expected.data)[1] = 1;
    
    asm_u256_mul(&a, &b, &result);
    assert(u256_equal(&result, &expected));
    printf("Test 2 passed: 64-bit overflow handled\n");
    
    // Test 3: Large multiplication
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 0xFFFFFFFFFFFFFFFF;
    ((uint64_t*)b.data)[0] = 2;
    ((uint64_t*)expected.data)[0] = 0xFFFFFFFFFFFFFFFE;
    ((uint64_t*)expected.data)[1] = 1;
    
    asm_u256_mul(&a, &b, &result);
    assert(u256_equal(&result, &expected));
    printf("Test 3 passed: Large multiplication\n");
    
    printf("All u256_mul tests passed!\n\n");
}

void test_u256_div() {
    printf("Testing u256_div...\n");
    
    uint256_t a, b, quotient, remainder, expected_q, expected_r;
    
    // Test 1: Basic division
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 1000;
    ((uint64_t*)b.data)[0] = 200;
    ((uint64_t*)expected_q.data)[0] = 5;
    ((uint64_t*)expected_r.data)[0] = 0;
    
    asm_u256_div(&a, &b, &quotient, &remainder);
    assert(u256_equal(&quotient, &expected_q));
    assert(u256_equal(&remainder, &expected_r));
    printf("Test 1 passed: 1000 / 200 = 5\n");
    
    // Test 2: Division with remainder
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 1007;
    ((uint64_t*)b.data)[0] = 200;
    ((uint64_t*)expected_q.data)[0] = 5;
    ((uint64_t*)expected_r.data)[0] = 7;
    
    asm_u256_div(&a, &b, &quotient, &remainder);
    assert(u256_equal(&quotient, &expected_q));
    assert(u256_equal(&remainder, &expected_r));
    printf("Test 2 passed: 1007 / 200 = 5 remainder 7\n");
    
    // Test 3: Division by zero
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 1000;
    // b is zero
    
    asm_u256_div(&a, &b, &quotient, &remainder);
    assert(u256_is_zero(&quotient));
    assert(u256_is_zero(&remainder));
    printf("Test 3 passed: Division by zero handled\n");
    
    printf("All u256_div tests passed!\n\n");
}

void test_u256_cmp() {
    printf("Testing u256_cmp...\n");
    
    uint256_t a, b;
    
    // Test 1: Equal numbers
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 100;
    ((uint64_t*)b.data)[0] = 100;
    
    int result = asm_u256_cmp(&a, &b);
    assert(result == 0);
    printf("Test 1 passed: 100 == 100\n");
    
    // Test 2: a < b
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 100;
    ((uint64_t*)b.data)[0] = 200;
    
    result = asm_u256_cmp(&a, &b);
    assert(result == -1);
    printf("Test 2 passed: 100 < 200\n");
    
    // Test 3: a > b
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[0] = 300;
    ((uint64_t*)b.data)[0] = 200;
    
    result = asm_u256_cmp(&a, &b);
    assert(result == 1);
    printf("Test 3 passed: 300 > 200\n");
    
    // Test 4: Compare high bits
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    ((uint64_t*)a.data)[3] = 1;
    ((uint64_t*)b.data)[3] = 2;
    
    result = asm_u256_cmp(&a, &b);
    assert(result == -1);
    printf("Test 4 passed: High bits comparison\n");
    
    printf("All u256_cmp tests passed!\n\n");
}

void test_pool_operations() {
    printf("Testing pool operations...\n");
    
    PoolState pool;
    address_t token0, token1;
    memset(&token0, 0xAA, sizeof(token0));
    memset(&token1, 0xBB, sizeof(token1));
    
    // Test 1: Initialize pool
    bool success = pool_initialize(&pool, token0, token1, 300);
    assert(success);
    assert(pool.fee == 300);
    assert(memcmp(&pool.token0, &token0, sizeof(address_t)) == 0);
    printf("Test 1 passed: Pool initialized\n");
    
    // Test 2: Add liquidity (first mint)
    address_t user;
    memset(&user, 0xCC, sizeof(user));
    uint256_t amount0, amount1, liquidity;
    memset(&amount0, 0, sizeof(amount0));
    memset(&amount1, 0, sizeof(amount1));
    ((uint64_t*)amount0.data)[0] = 1000000; // 1 ETH
    ((uint64_t*)amount1.data)[0] = 3000000000; // 3000 USDC
    
    success = pool_mint(&pool, user, amount0, amount1, &liquidity);
    assert(success);
    assert(!u256_is_zero(&liquidity));
    assert(u256_equal(&pool.reserve0, &amount0));
    assert(u256_equal(&pool.reserve1, &amount1));
    printf("Test 2 passed: Liquidity added\n");
    
    // Test 3: Get amount out
    uint256_t amount_in, reserve_in, reserve_out, amount_out;
    memset(&amount_in, 0, sizeof(amount_in));
    ((uint64_t*)amount_in.data)[0] = 1000000000000000000; // 1 ETH
    reserve_in = pool.reserve0;
    reserve_out = pool.reserve1;
    
    amount_out = pool_get_amount_out(amount_in, reserve_in, reserve_out, 300);
    assert(!u256_is_zero(&amount_out));
    printf("Test 3 passed: Amount out calculated\n");
    
    // Test 4: Swap
    uint256_t amount0_out, amount1_out;
    memset(&amount0_out, 0, sizeof(amount0_out));
    memset(&amount1_out, 0, sizeof(amount1_out));
    ((uint64_t*)amount1_out.data)[0] = 1000000; // 1 USDC
    
    success = pool_swap(&pool, amount0_out, amount1_out, user);
    assert(success);
    printf("Test 4 passed: Swap executed\n");
    
    // Test 5: Remove liquidity
    uint256_t amount0_ret, amount1_ret;
    success = pool_burn(&pool, user, liquidity, &amount0_ret, &amount1_ret);
    assert(success);
    assert(!u256_is_zero(&amount0_ret));
    assert(!u256_is_zero(&amount1_ret));
    printf("Test 5 passed: Liquidity removed\n");
    
    printf("All pool operations tests passed!\n\n");
}

void run_all_tests() {
    printf("Running all tests...\n\n");
    
    test_u256_add();
    test_u256_sub();
    test_u256_mul();
    test_u256_div();
    test_u256_cmp();
    test_pool_operations();
    
    printf("All tests completed successfully!\n");
}

int main() {
    run_all_tests();
    return 0;
}
