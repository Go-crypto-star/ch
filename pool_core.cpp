#include "pool_core.h"
#include "math_operations.h"
#include <cstring>
#include <stdexcept>

extern "C" {

bool pool_initialize(PoolState* pool, address_t token0, address_t token1, uint32_t fee) {
    if (pool == nullptr) return false;
    if (fee > MAX_FEE) return false;
    
    // Инициализация пула
    memset(pool, 0, sizeof(PoolState));
    pool->token0 = token0;
    pool->token1 = token1;
    pool->fee = fee;
    
    return true;
}

bool pool_mint(PoolState* pool, address_t to, uint256_t amount0, uint256_t amount1, uint256_t* liquidity_out) {
    if (pool == nullptr || liquidity_out == nullptr) return false;
    
    uint256_t total_liquidity = pool->total_supply;
    uint256_t liquidity;
    
    if (total_liquidity.data[0] == 0 && total_liquidity.data[1] == 0) {
        // Первое пополнение ликвидности
        asm_u256_mul(&amount0, &amount1, &liquidity);
        asm_u256_sqrt(&liquidity, &liquidity);
    } else {
        // Последующие пополнения
        uint256_t liquidity0, liquidity1;
        
        asm_u256_mul(&amount0, &total_liquidity, &liquidity0);
        asm_u256_div(&liquidity0, &pool->reserve0, &liquidity0);
        
        asm_u256_mul(&amount1, &total_liquidity, &liquidity1);
        asm_u256_div(&liquidity1, &pool->reserve1, &liquidity1);
        
        // liquidity = min(liquidity0, liquidity1)
        if (u256_compare(&liquidity0, &liquidity1) <= 0) {
            liquidity = liquidity0;
        } else {
            liquidity = liquidity1;
        }
    }
    
    if (u256_is_zero(&liquidity)) return false;
    
    // Обновление резервов
    safe_u256_add(&pool->reserve0, &pool->reserve0, &amount0);
    safe_u256_add(&pool->reserve1, &pool->reserve1, &amount1);
    safe_u256_add(&pool->total_supply, &pool->total_supply, &liquidity);
    
    *liquidity_out = liquidity;
    
    // Эмитация события
    LiquidityAdded event;
    event.sender = to;
    event.amount0 = amount0;
    event.amount1 = amount1;
    event.to = to;
    go_emit_liquidity_added(&event);
    
    return true;
}

bool pool_burn(PoolState* pool, address_t to, uint256_t liquidity, uint256_t* amount0_out, uint256_t* amount1_out) {
    if (pool == nullptr || amount0_out == nullptr || amount1_out == nullptr) return false;
    
    uint256_t total_liquidity = pool->total_supply;
    if (u256_compare(&liquidity, &total_liquidity) > 0) return false;
    
    // Вычисление amounts
    asm_u256_mul(&liquidity, &pool->reserve0, amount0_out);
    asm_u256_div(amount0_out, &total_liquidity, amount0_out);
    
    asm_u256_mul(&liquidity, &pool->reserve1, amount1_out);
    asm_u256_div(amount1_out, &total_liquidity, amount1_out);
    
    // Обновление резервов
    safe_u256_sub(&pool->reserve0, &pool->reserve0, amount0_out);
    safe_u256_sub(&pool->reserve1, &pool->reserve1, amount1_out);
    safe_u256_sub(&pool->total_supply, &pool->total_supply, &liquidity);
    
    // Эмитация события
    LiquidityRemoved event;
    event.sender = to;
    event.amount0 = *amount0_out;
    event.amount1 = *amount1_out;
    event.to = to;
    go_emit_liquidity_removed(&event);
    
    return true;
}

bool pool_swap(PoolState* pool, uint256_t amount0_out, uint256_t amount1_out, address_t to) {
    if (pool == nullptr) return false;
    
    // Проверка выходных amounts
    if (u256_is_zero(&amount0_out) && u256_is_zero(&amount1_out)) return false;
    
    uint256_t reserve0 = pool->reserve0;
    uint256_t reserve1 = pool->reserve1;
    
    // Проверка достаточности резервов
    if (!u256_is_zero(&amount0_out) && u256_compare(&amount0_out, &reserve0) >= 0) return false;
    if (!u256_is_zero(&amount1_out) && u256_compare(&amount1_out, &reserve1) >= 0) return false;
    
    // Обновление резервов
    if (!u256_is_zero(&amount0_out)) {
        safe_u256_sub(&pool->reserve0, &reserve0, &amount0_out);
    }
    if (!u256_is_zero(&amount1_out)) {
        safe_u256_sub(&pool->reserve1, &reserve1, &amount1_out);
    }
    
    // Эмитация события
    SwapEvent event;
    memset(&event, 0, sizeof(SwapEvent));
    event.amount0Out = amount0_out;
    event.amount1Out = amount1_out;
    event.to = to;
    go_emit_swap(&event);
    
    return true;
}

uint256_t pool_get_amount_out(uint256_t amount_in, uint256_t reserve_in, uint256_t reserve_out, uint32_t fee) {
    if (u256_is_zero(&amount_in)) return amount_in;
    
    uint256_t amount_in_with_fee;
    asm_u256_mul(&amount_in, &(uint256_t){(uint64_t)(FEE_DENOMINATOR - fee)}, &amount_in_with_fee);
    
    uint256_t numerator;
    asm_u256_mul(&amount_in_with_fee, &reserve_out, &numerator);
    
    uint256_t denominator;
    asm_u256_mul(&reserve_in, &(uint256_t){(uint64_t)FEE_DENOMINATOR}, &denominator);
    asm_u256_add(&denominator, &amount_in_with_fee, &denominator);
    
    uint256_t amount_out;
    asm_u256_div(&numerator, &denominator, &amount_out);
    
    return amount_out;
}

} // extern "C"

// Вспомогательные функции
static bool u256_is_zero(const uint256_t* a) {
    for (int i = 0; i < 4; i++) {
        if (((uint64_t*)a->data)[i] != 0) return false;
    }
    return true;
}

static int u256_compare(const uint256_t* a, const uint256_t* b) {
    for (int i = 3; i >= 0; i--) {
        uint64_t a_val = ((uint64_t*)a->data)[i];
        uint64_t b_val = ((uint64_t*)b->data)[i];
        if (a_val > b_val) return 1;
        if (a_val < b_val) return -1;
    }
    return 0;
}
