#include "pool_core.h"
#include "protocol/partials.h"
#include "protocol/singleton.h"
#include "blockchain/chia_operations.h"
#include "security/auth.h"
#include "security/proof_verification.h"
#include "math_operations.h"
#include "optimizations.h"
#include "go_bridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

static pool_context_t g_pool_context;
static char g_last_error[1024] = {0};

// Логирование с временными метками
void pool_log(const char* level, const char* message) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] [%s] %s\n", timestamp, level, message);
    fflush(stdout);
}

// Функция для основного цикла пула
static void* pool_main_loop(void* arg) {
    pool_context_t* ctx = (pool_context_t*)arg;
    
    pool_log("INFO", "Основной цикл пула запущен");
    
    while (!ctx->shutdown_requested && !ctx->emergency_stop) {
        // Синхронизация с блокчейном
        if (!chia_sync_to_peak()) {
            pool_log("ERROR", "Ошибка синхронизации с блокчейном");
            sleep(10);
            continue;
        }
        
        // Обновление статистики
        pool_log_statistics();
        
        // Проверка состояния
        pthread_mutex_lock(&ctx->state_mutex);
        pool_state_t state = ctx->state;
        pthread_mutex_unlock(&ctx->state_mutex);
        
        if (state != POOL_STATE_RUNNING) {
            pool_log("WARNING", "Пул перешел в состояние остановки");
            break;
        }
        
        sleep(30); // Цикл каждые 30 секунд
    }
    
    pool_log("INFO", "Основной цикл пула завершен");
    return NULL;
}

bool pool_init(const pool_config_t* config) {
    pool_log("INFO", "Инициализация пула...");
    
    if (!config) {
        pool_set_error("Конфигурация не может быть NULL");
        return false;
    }
    
    if (!pool_validate_config(config)) {
        pool_log("ERROR", "Невалидная конфигурация пула");
        return false;
    }
    
    // Объявление переменной в начале функции
    optimizations_config_t optim_config;
    
    memset(&g_pool_context, 0, sizeof(pool_context_t));
    g_pool_context.state = POOL_STATE_INIT;
    memcpy(&g_pool_context.config, config, sizeof(pool_config_t));
    g_pool_context.start_time = time(NULL);
    
    // Инициализация мьютексов
    if (pthread_mutex_init(&g_pool_context.state_mutex, NULL) != 0) {
        pool_set_error("Не удалось инициализировать мьютекс состояния");
        return false;
    }
    
    if (pthread_mutex_init(&g_pool_context.stats_mutex, NULL) != 0) {
        pool_set_error("Не удалось инициализировать мьютекс статистики");
        pthread_mutex_destroy(&g_pool_context.state_mutex);
        return false;
    }
    
    if (pthread_mutex_init(&g_pool_context.farmers_mutex, NULL) != 0) {
        pool_set_error("Не удалось инициализировать мьютекс фермеров");
        pthread_mutex_destroy(&g_pool_context.state_mutex);
        pthread_mutex_destroy(&g_pool_context.stats_mutex);
        return false;
    }
    
    pool_log("INFO", "Мьютексы инициализированы успешно");
    
    // Инициализация подсистем
    if (!chia_operations_init(config->node_rpc_host, config->node_rpc_port,
                             config->node_rpc_cert_path, config->node_rpc_key_path)) {
        pool_set_error("Не удалось инициализировать блокчейн операции");
        goto cleanup;
    }
    
    if (!proof_verification_init()) {
        pool_set_error("Не удалось инициализировать верификацию доказательств");
        goto cleanup;
    }
    
    bls_key_t pool_key;
    memset(&pool_key, 0, sizeof(bls_key_t));
    if (!auth_init(&pool_key)) {
        pool_set_error("Не удалось инициализировать аутентификацию");
        goto cleanup;
    }
    
    if (!math_operations_init()) {
        pool_set_error("Не удалось инициализировать математические операции");
        goto cleanup;
    }
    
    // Инициализация структуры optim_config
    optim_config.enable_proof_cache = true;
    optim_config.enable_signature_cache = true;
    optim_config.enable_vectorization = true;
    optim_config.enable_asm_optimizations = true;
    optim_config.max_cache_memory = 1024 * 1024 * 100; // 100 MB
    optim_config.cache_ttl_seconds = 300;
    
    if (!optimizations_init(&optim_config)) {
        pool_log("WARNING", "Не удалось инициализировать оптимизации, продолжаем без них");
    }
    
    if (!go_bridge_init()) {
        pool_set_error("Не удалось инициализировать Go мост");
        goto cleanup;
    }
    
    g_pool_context.state = POOL_STATE_RUNNING;
    pool_log("INFO", "Пул успешно инициализирован");
    return true;

cleanup:
    pool_cleanup();
    return false;
}

bool pool_start(void) {
    pool_log("INFO", "Запуск пула...");
    
    pthread_mutex_lock(&g_pool_context.state_mutex);
    if (g_pool_context.state != POOL_STATE_INIT) {
        pthread_mutex_unlock(&g_pool_context.state_mutex);
        pool_set_error("Пул должен быть в состоянии INIT для запуска");
        return false;
    }
    
    g_pool_context.state = POOL_STATE_RUNNING;
    g_pool_context.shutdown_requested = false;
    g_pool_context.emergency_stop = false;
    pthread_mutex_unlock(&g_pool_context.state_mutex);
    
    // Запуск основного цикла в отдельном потоке
    if (pthread_create(&g_pool_context.main_thread, NULL, 
                      pool_main_loop, &g_pool_context) != 0) {
        pool_set_error("Не удалось создать основной поток");
        return false;
    }
    
    pool_log("INFO", "Пул успешно запущен");
    return true;
}

bool pool_stop(void) {
    pool_log("INFO", "Остановка пула...");
    
    pthread_mutex_lock(&g_pool_context.state_mutex);
    g_pool_context.shutdown_requested = true;
    g_pool_context.state = POOL_STATE_SHUTTING_DOWN;
    pthread_mutex_unlock(&g_pool_context.state_mutex);
    
    // Ожидание завершения основного потока
    if (pthread_join(g_pool_context.main_thread, NULL) != 0) {
        pool_log("ERROR", "Ошибка при ожидании завершения основного потока");
    }
    
    pool_log("INFO", "Пул успешно остановлен");
    return true;
}

bool pool_cleanup(void) {
    pool_log("INFO", "Очистка ресурсов пула...");
    
    // Остановка всех подсистем
    go_bridge_cleanup();
    optimizations_cleanup();
    auth_cleanup();
    proof_verification_cleanup();
    chia_operations_cleanup();
    
    // Уничтожение мьютексов
    pthread_mutex_destroy(&g_pool_context.farmers_mutex);
    pthread_mutex_destroy(&g_pool_context.stats_mutex);
    pthread_mutex_destroy(&g_pool_context.state_mutex);
    
    memset(&g_pool_context, 0, sizeof(pool_context_t));
    pool_log("INFO", "Ресурсы пула успешно очищены");
    
    return true;
}

pool_context_t* pool_get_context(void) {
    return &g_pool_context;
}

const char* pool_state_to_string(pool_state_t state) {
    switch (state) {
        case POOL_STATE_INIT: return "INIT";
        case POOL_STATE_RUNNING: return "RUNNING";
        case POOL_STATE_SHUTTING_DOWN: return "SHUTTING_DOWN";
        case POOL_STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void pool_log_statistics(void) {
    pthread_mutex_lock(&g_pool_context.stats_mutex);
    pool_stats_t stats = g_pool_context.stats;
    pthread_mutex_unlock(&g_pool_context.stats_mutex);
    
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg),
             "Статистика пула: фермеры=%lu, partials=%lu (valid=%lu, invalid=%lu), "
             "блоки=%lu, netspace=%.2f TiB, очки=%lu, сложность=%lu",
             stats.total_farmers, stats.total_partials, stats.valid_partials,
             stats.invalid_partials, stats.total_blocks_found, stats.total_netspace,
             stats.total_points, stats.current_difficulty);
    
    pool_log("INFO", log_msg);
}

void pool_set_error(const char* error_msg) {
    if (error_msg) {
        strncpy(g_last_error, error_msg, sizeof(g_last_error) - 1);
        g_last_error[sizeof(g_last_error) - 1] = '\0';
        pool_log("ERROR", error_msg);
    }
}

const char* pool_get_last_error(void) {
    return g_last_error;
}

bool pool_validate_config(const pool_config_t* config) {
    if (!config) {
        pool_set_error("Конфигурация не может быть NULL");
        return false;
    }
    
    if (strlen(config->pool_name) == 0) {
        pool_set_error("Имя пула не может быть пустым");
        return false;
    }
    
    // Исправлено: убрана проверка > 65535, так как config->port уже uint16_t
    if (config->port == 0) {
        pool_set_error("Невалидный порт");
        return false;
    }
    
    if (config->pool_fee < 0 || config->pool_fee > 1.0) {
        pool_set_error("Комиссия пула должна быть между 0 и 1");
        return false;
    }
    
    if (strlen(config->node_rpc_host) == 0) {
        pool_set_error("Хост RPC ноды не может быть пустым");
        return false;
    }
    
    pool_log("INFO", "Конфигурация пула валидна");
    return true;
}

bool pool_load_default_config(pool_config_t* config) {
    if (!config) {
        return false;
    }
    
    memset(config, 0, sizeof(pool_config_t));
    
    strcpy(config->pool_name, "Chia Pool");
    strcpy(config->pool_url, "https://pool.example.com");
    config->port = 8444;
    config->pool_fee = 0.01; // 1%
    config->min_payout = 1000000000; // 0.001 XCH
    config->partial_deadline = 28; // 28 секунд
    config->difficulty_target = 300; // 300 partials в день
    strcpy(config->node_rpc_host, "localhost");
    config->node_rpc_port = 8555;
    strcpy(config->node_rpc_cert_path, "/root/.chia/mainnet/config/ssl/full_node/private_full_node.crt");
    strcpy(config->node_rpc_key_path, "/root/.chia/mainnet/config/ssl/full_node/private_full_node.key");
    
    pool_log("INFO", "Загружена конфигурация по умолчанию");
    return true;
}
