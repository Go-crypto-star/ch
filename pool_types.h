#ifndef POOL_TYPES_H
#define POOL_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Основные типы данных
typedef struct {
    uint8_t data[20];
} address_t;

typedef struct {
    uint8_t data[32];
} hash_t;

typedef struct {
    uint8_t data[32];
} uint256_t;

// Структура токена
typedef struct {
    char name[32];
    char symbol[8];
    uint8_t decimals;
    address_t address;
    uint256_t total_supply;
    uint256_t balance;
} token_t;

// Структура позиции ликвидности
typedef struct {
    address_t owner;
    uint256_t liquidity;
    uint256_t amount_a;
    uint256_t amount_b;
    uint64_t timestamp;
} liquidity_position_t;

// Структура состояния пула
typedef struct {
    token_t token_a;
    token_t token_b;
    
    uint256_t reserve_a;
    uint256_t reserve_b;
    uint256_t total_liquidity;
    
    uint32_t fee_rate;
    uint32_t protocol_fee_rate;
    uint256_t fee_collected_a;
    uint256_t fee_collected_b;
    
    bool paused;
    address_t admin;
    
    // Кэш для производительности
    liquidity_position_t* positions_cache;
    uint32_t cache_size;
    uint32_t cache_count;
} pool_state_t;

// Структура для свопа
typedef struct {
    address_t sender;
    uint256_t amount_in;
    uint256_t amount_out;
    uint256_t fee;
    bool exact_input;
    uint64_t timestamp;
} swap_event_t;

#ifdef __cplusplus
}
#endif

#endif // POOL_TYPES_H
