#ifndef POOL_CORE_H
#define POOL_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

// Состояния пула
typedef enum {
    POOL_STATE_INIT,
    POOL_STATE_RUNNING,
    POOL_STATE_SHUTTING_DOWN,
    POOL_STATE_ERROR
} pool_state_t;

// Конфигурация пула
typedef struct {
    char pool_name[256];
    char pool_url[512];
    uint16_t port;
    double pool_fee;           // Комиссия пула (0.01 = 1%)
    uint64_t min_payout;       // Минимальная выплата в mojos
    uint32_t partial_deadline; // Дедлайн для partial (28 секунд)
    uint32_t difficulty_target;// Целевое количество partials в день на фермера
    char node_rpc_host[256];
    uint16_t node_rpc_port;
    char node_rpc_cert_path[512];
    char node_rpc_key_path[512];
} pool_config_t;

// Статистика пула
typedef struct {
    uint64_t total_farmers;
    uint64_t total_partials;
    uint64_t valid_partials;
    uint64_t invalid_partials;
    uint64_t total_blocks_found;
    double total_netspace;     // В TiB
    uint64_t total_points;
    uint64_t current_difficulty;
} pool_stats_t;

// Основной контекст пула
typedef struct {
    pool_state_t state;
    pool_config_t config;
    pool_stats_t stats;
    
    // Потоки
    pthread_t main_thread;
    pthread_t partial_processor_thread;
    pthread_t blockchain_sync_thread;
    pthread_t payment_processor_thread;
    
    // Мьютексы
    pthread_mutex_t state_mutex;
    pthread_mutex_t stats_mutex;
    pthread_mutex_t farmers_mutex;
    
    // Флаги управления
    bool shutdown_requested;
    bool emergency_stop;
    
    // Временные метки
    uint64_t start_time;
    uint64_t last_block_time;
    uint64_t last_difficulty_adjustment;
} pool_context_t;

// Инициализация и управление пулом
bool pool_init(const pool_config_t* config);
bool pool_start(void);
bool pool_stop(void);
bool pool_cleanup(void);

// Утилиты
pool_context_t* pool_get_context(void);
const char* pool_state_to_string(pool_state_t state);
void pool_log_statistics(void);

// Обработчики ошибок
void pool_set_error(const char* error_msg);
const char* pool_get_last_error(void);

// Внутренние функции (для тестирования)
bool pool_validate_config(const pool_config_t* config);
bool pool_load_default_config(pool_config_t* config);

#endif // POOL_CORE_H
