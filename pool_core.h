#ifndef POOL_CORE_H
#define POOL_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Базовые типы
typedef struct {
    uint8_t data[32];
} uint256_t;

typedef struct {
    uint8_t data[20];
} address_t;

typedef struct {
    uint8_t data[32];
} bytes32_t;

// Константы из Solidity
#define FEE_DENOMINATOR 10000
#define MAX_FEE 3000  // 30%

// Структуры данных пула
typedef struct {
    uint256_t reserve0;
    uint256_t reserve1;
    uint256_t total_supply;
    uint32_t fee;
    address_t token0;
    address_t token1;
    address_t factory;
} PoolState;

// События (аналоги Solidity events)
typedef struct {
    address_t sender;
    uint256_t amount0;
    uint256_t amount1;
    address_t to;
} LiquidityAdded;

typedef struct {
    address_t sender;
    uint256_t amount0;
    uint256_t amount1;
    address_t to;
} LiquidityRemoved;

typedef struct {
    address_t sender;
    uint256_t amount0In;
    uint256_t amount1In;
    uint256_t amount0Out;
    uint256_t amount1Out;
    address_t to;
} SwapEvent;

// Основные функции пула
bool pool_initialize(PoolState* pool, address_t token0, address_t token1, uint32_t fee);
bool pool_mint(PoolState* pool, address_t to, uint256_t amount0, uint256_t amount1, uint256_t* liquidity_out);
bool pool_burn(PoolState* pool, address_t to, uint256_t liquidity, uint256_t* amount0_out, uint256_t* amount1_out);
bool pool_swap(PoolState* pool, uint256_t amount0_out, uint256_t amount1_out, address_t to);

// Вспомогательные функции
uint256_t pool_get_amount_out(uint256_t amount_in, uint256_t reserve_in, uint256_t reserve_out, uint32_t fee);
uint256_t pool_get_amount_in(uint256_t amount_out, uint256_t reserve_in, uint256_t reserve_out, uint32_t fee);
uint256_t pool_quote(uint256_t amount_a, uint256_t reserve_a, uint256_t reserve_b);
uint256_t pool_get_reserves_product(PoolState* pool);

// Go-экспортируемые функции
extern void go_emit_liquidity_added(LiquidityAdded* event);
extern void go_emit_liquidity_removed(LiquidityRemoved* event);
extern void go_emit_swap(SwapEvent* event);

#ifdef __cplusplus
}
#endif

#endif
