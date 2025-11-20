#ifndef CHIA_OPERATIONS_H
#define CHIA_OPERATIONS_H

#include <cstddef>
#include <stdint.h>
#include <stdbool.h>

// Состояние синхронизации с блокчейном
typedef struct {
    uint32_t current_height;
    uint32_t synced_height;
    uint64_t network_space;        // Сетевое пространство в bytes
    double progress;               // Прогресс синхронизации (0.0-1.0)
    bool is_syncing;
    uint64_t last_peak_timestamp;
} blockchain_sync_state_t;

// Информация о блоке
typedef struct {
    uint32_t height;
    uint8_t block_hash[32];
    uint8_t farmer_puzzle_hash[32];
    uint8_t pool_puzzle_hash[32];
    uint64_t timestamp;
    uint64_t difficulty;
    uint64_t total_iterations;
} block_info_t;

// Точка сигнейджа (signage point)
typedef struct {
    uint8_t challenge_hash[32];
    uint8_t challenge_chain_sp[32];
    uint8_t reward_chain_sp[32];
    uint32_t signage_point_index;
    uint64_t timestamp;
    uint32_t peak_height;
} signage_point_t;

// Инициализация блокчейн модуля
bool chia_operations_init(const char* rpc_host, uint16_t rpc_port, 
                         const char* cert_path, const char* key_path);
bool chia_operations_cleanup(void);

// Синхронизация с блокчейном
bool chia_sync_to_peak(void);
blockchain_sync_state_t chia_get_sync_state(void);

// Работа с точками сигнейджа
bool chia_subscribe_to_signage_points(void);
signage_point_t chia_get_current_signage_point(void);
bool chia_validate_signage_point(const signage_point_t* sp);

// Мониторинг блоков
bool chia_monitor_new_blocks(void);
block_info_t chia_get_block_info(uint32_t height);
bool chia_validate_proof_of_time(const block_info_t* block);

// RPC вызовы к ноде
bool chia_rpc_get_blockchain_state(void);
bool chia_rpc_get_network_space(uint64_t start_height, uint64_t end_height);
bool chia_rpc_get_coin_records_by_puzzle_hash(const uint8_t* puzzle_hash, uint32_t start_height);

// Утилиты
void chia_log_sync_state(void);
bool chia_verify_network_connection(void);

#endif // CHIA_OPERATIONS_H
