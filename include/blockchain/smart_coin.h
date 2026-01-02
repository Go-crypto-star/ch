#ifndef SMART_COIN_H
#define SMART_COIN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Структура смарт-коина (Chialisp)
typedef struct {
    uint8_t coin_id[32];          // ID коина
    uint8_t puzzle_hash[32];      // Хэш пазла
    uint8_t parent_coin_id[32];   // ID родительского коина
    uint64_t amount;              // Сумма в mojos
    uint32_t confirmed_height;    // Высота подтверждения
} smart_coin_t;

// Транзакция поглощения (absorb)
typedef struct {
    uint8_t launcher_id[32];
    uint64_t amount;
    uint8_t signature[96];
    uint32_t fee;
    uint8_t transaction_bytes[4096]; // Сырые байты транзакции
    size_t transaction_size;
} absorb_transaction_t;

// Условия смарт-контракта
typedef struct {
    uint64_t create_coin_condition;
    uint64_t assert_coin_announcement;
    uint64_t assert_puzzle_announcement;
    uint64_t relative_time_lock;
    uint64_t absolute_time_lock;
} coin_conditions_t;

// Инициализация смарт-коинов
bool smart_coin_init(void);
bool smart_coin_parse(const uint8_t* coin_data, size_t data_size, smart_coin_t* coin);

// Создание транзакций
absorb_transaction_t* smart_coin_create_absorb_transaction(const uint8_t* launcher_id, uint64_t amount);
bool smart_coin_sign_absorb_transaction(absorb_transaction_t* transaction, const uint8_t* private_key);

// Валидация условий
bool smart_coin_validate_conditions(const smart_coin_t* coin, const coin_conditions_t* conditions);
bool smart_coin_verify_puzzle_hash(const smart_coin_t* coin, const uint8_t* expected_puzzle_hash);

// Работа с блокчейном
bool smart_coin_submit_transaction(const absorb_transaction_t* transaction);
bool smart_coin_wait_for_confirmation(const uint8_t* coin_id, uint32_t timeout_seconds);

// Утилиты
void smart_coin_log_transaction(const absorb_transaction_t* transaction);
bool smart_coin_calculate_coin_id(const uint8_t* parent_coin_id, const uint8_t* puzzle_hash, 
                                 uint64_t amount, uint8_t* coin_id);

#endif // SMART_COIN_H
