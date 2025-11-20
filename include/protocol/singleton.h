#ifndef SINGLETON_H
#define SINGLETON_H

#include <stdint.h>
#include <stdbool.h>

// Структура синглтона (Plot NFT)
typedef struct {
    uint8_t launcher_id[32];       // ID синглтона
    uint8_t p2_singleton_puzzle[32]; // Пазл синглтона
    uint8_t owner_public_key[48];  // Публичный ключ владельца
    uint64_t total_points;         // Всего очков
    uint64_t current_difficulty;   // Текущая сложность
    uint64_t last_partial_time;    // Время последнего partial
    bool is_pool_member;           // Принадлежит ли пулу
    uint64_t balance;              // Баланс в mojos
    uint32_t relative_lock_height; // Высота блокировки
} singleton_t;

// Состояние синхронизации синглтона
typedef struct {
    uint8_t launcher_id[32];
    uint32_t confirmed_height;
    uint32_t pending_height;
    bool needs_absorb;            // Требуется поглощение вознаграждения
    uint64_t pending_amount;      // Сумма к поглощению
} singleton_sync_state_t;

// Инициализация синглтона
bool singleton_init(const uint8_t* launcher_id, singleton_t* singleton);

// Валидация синглтона
bool singleton_validate_ownership(const singleton_t* singleton, const uint8_t* signature);
bool singleton_verify_pool_membership(const singleton_t* singleton);

// Управление состоянием
bool singleton_update_state(singleton_t* singleton);
bool singleton_absorb_rewards(singleton_t* singleton);

// Работа с блокчейном
bool singleton_sync_with_blockchain(singleton_t* singleton);
bool singleton_get_coin_records(const uint8_t* launcher_id, uint32_t start_height, uint32_t end_height);

// Утилиты
void singleton_log_state(const singleton_t* singleton);
bool singleton_can_leave_pool(const singleton_t* singleton);

#endif // SINGLETON_H
