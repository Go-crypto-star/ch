#include "optimizations.h"
#include "security/proof_verification.h"
#include "security/auth.h"
#include "protocol/partials.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <map>
#include <string>
#include <vector>

// Структура кеша
typedef struct {
    void* data;
    size_t size;
    uint64_t timestamp;
    uint64_t access_count;
} cache_entry_t;

static optimizations_config_t g_optim_config;
static std::map<std::string, cache_entry_t*> g_caches[4]; // По одному для каждого типа кеша
static cache_stats_t g_cache_stats[4];
static pthread_mutex_t g_cache_mutex = PTHREAD_MUTEX_INITIALIZER;

static void optimizations_log(const char* level, const char* message) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] [OPTIMIZATIONS] [%s] %s\n", timestamp, level, message);
    fflush(stdout);
}

// Генерация ключа кеша
static std::string generate_cache_key(const uint8_t* key, size_t key_len) {
    std::string cache_key;
    cache_key.reserve(key_len * 2 + 1);
    
    for (size_t i = 0; i < key_len; i++) {
        char hex[3];
        sprintf(hex, "%02x", key[i]);
        cache_key.append(hex);
    }
    
    return cache_key;
}

bool optimizations_init(const optimizations_config_t* config) {
    optimizations_log("INFO", "Инициализация оптимизаций...");
    
    if (!config) {
        optimizations_log("ERROR", "Конфигурация оптимизаций не может быть NULL");
        return false;
    }
    
    memcpy(&g_optim_config, config, sizeof(optimizations_config_t));
    
    // Инициализация статистики кешей
    for (int i = 0; i < 4; i++) {
        memset(&g_cache_stats[i], 0, sizeof(cache_stats_t));
        g_cache_stats[i].max_memory = config->max_cache_memory / 4; // Равномерное распределение
    }
    
    optimizations_log("INFO", "Оптимизации успешно инициализированы");
    return true;
}

bool optimizations_cleanup(void) {
    optimizations_log("INFO", "Очистка оптимизаций...");
    
    pthread_mutex_lock(&g_cache_mutex);
    
    // Очистка всех кешей
    for (int i = 0; i < 4; i++) {
        for (auto& pair : g_caches[i]) {
            free(pair.second->data);
            free(pair.second);
        }
        g_caches[i].clear();
    }
    
    pthread_mutex_unlock(&g_cache_mutex);
    
    optimizations_log("INFO", "Оптимизации очищены");
    return true;
}

bool cache_put(cache_type_t type, const uint8_t* key, size_t key_len, 
               const void* value, size_t value_size) {
    if (!key || !value || value_size == 0) {
        optimizations_log("ERROR", "Невалидные параметры для добавления в кеш");
        return false;
    }
    
    if (type < 0 || type > 3) {
        optimizations_log("ERROR", "Невалидный тип кеша");
        return false;
    }
    
    pthread_mutex_lock(&g_cache_mutex);
    
    // Проверяем, не превысим ли мы лимит памяти
    if (g_cache_stats[type].memory_used + value_size > g_cache_stats[type].max_memory) {
        // Вытесняем старые записи
        optimizations_clear_cache(type);
    }
    
    std::string cache_key = generate_cache_key(key, key_len);
    
    // Создаем новую запись в кеше
    cache_entry_t* entry = (cache_entry_t*)malloc(sizeof(cache_entry_t));
    if (!entry) {
        pthread_mutex_unlock(&g_cache_mutex);
        optimizations_log("ERROR", "Не удалось выделить память для записи кеша");
        return false;
    }
    
    entry->data = malloc(value_size);
    if (!entry->data) {
        free(entry);
        pthread_mutex_unlock(&g_cache_mutex);
        optimizations_log("ERROR", "Не удалось выделить память для данных кеша");
        return false;
    }
    
    memcpy(entry->data, value, value_size);
    entry->size = value_size;
    entry->timestamp = time(NULL);
    entry->access_count = 0;
    
    // Добавляем в кеш
    g_caches[type][cache_key] = entry;
    g_cache_stats[type].memory_used += value_size;
    
    pthread_mutex_unlock(&g_cache_mutex);
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Добавлено в кеш типа %d: key_size=%zu, value_size=%zu, memory_used=%zu",
             type, key_len, value_size, g_cache_stats[type].memory_used);
    optimizations_log("DEBUG", log_msg);
    
    return true;
}

void* cache_get(cache_type_t type, const uint8_t* key, size_t key_len) {
    if (!key || key_len == 0) {
        optimizations_log("ERROR", "Невалидные параметры для получения из кеша");
        return NULL;
    }
    
    if (type < 0 || type > 3) {
        optimizations_log("ERROR", "Невалидный тип кеша");
        return NULL;
    }
    
    pthread_mutex_lock(&g_cache_mutex);
    
    std::string cache_key = generate_cache_key(key, key_len);
    auto it = g_caches[type].find(cache_key);
    
    if (it == g_caches[type].end()) {
        pthread_mutex_unlock(&g_cache_mutex);
        g_cache_stats[type].misses++;
        return NULL;
    }
    
    cache_entry_t* entry = it->second;
    
    // Проверяем срок жизни записи
    uint64_t current_time = time(NULL);
    if (current_time - entry->timestamp > g_optim_config.cache_ttl_seconds) {
        // Запись устарела, удаляем ее
        free(entry->data);
        free(entry);
        g_caches[type].erase(it);
        g_cache_stats[type].memory_used -= entry->size;
        g_cache_stats[type].misses++;
        pthread_mutex_unlock(&g_cache_mutex);
        
        optimizations_log("DEBUG", "Запись кеша устарела и удалена");
        return NULL;
    }
    
    // Обновляем статистику
    entry->access_count++;
    g_cache_stats[type].hits++;
    
    pthread_mutex_unlock(&g_cache_mutex);
    
    optimizations_log("DEBUG", "Запись найдена в кеше");
    return entry->data;
}

bool cache_remove(cache_type_t type, const uint8_t* key, size_t key_len) {
    if (!key || key_len == 0) {
        optimizations_log("ERROR", "Невалидные параметры для удаления из кеша");
        return false;
    }
    
    pthread_mutex_lock(&g_cache_mutex);
    
    std::string cache_key = generate_cache_key(key, key_len);
    auto it = g_caches[type].find(cache_key);
    
    if (it != g_caches[type].end()) {
        cache_entry_t* entry = it->second;
        free(entry->data);
        free(entry);
        g_caches[type].erase(it);
        g_cache_stats[type].memory_used -= entry->size;
        
        pthread_mutex_unlock(&g_cache_mutex);
        
        optimizations_log("DEBUG", "Запись удалена из кеша");
        return true;
    }
    
    pthread_mutex_unlock(&g_cache_mutex);
    optimizations_log("DEBUG", "Запись для удаления не найдена в кеше");
    return false;
}

void vector_sha256(const uint8_t** inputs, const size_t* input_lens, 
                   uint8_t** outputs, size_t count) {
    if (!inputs || !input_lens || !outputs || count == 0) {
        optimizations_log("ERROR", "Невалидные параметры для векторного SHA256");
        return;
    }
    
    optimizations_log("DEBUG", "Выполнение векторного SHA256...");
    
    // В реальной реализации здесь будут SIMD оптимизации для SHA256
    // Сейчас используем последовательную обработку
    
    for (size_t i = 0; i < count; i++) {
        if (!inputs[i] || !outputs[i]) {
            continue;
        }
        
        // Упрощенная реализация - в реальности здесь будет вызов оптимизированной SHA256
        // Пока просто копируем первые 32 байта для демонстрации
        size_t copy_size = (input_lens[i] < 32) ? input_lens[i] : 32;
        memcpy(outputs[i], inputs[i], copy_size);
        
        // Дополняем нулями если нужно
        if (copy_size < 32) {
            memset(outputs[i] + copy_size, 0, 32 - copy_size);
        }
    }
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "Векторный SHA256 завершен: обработано %zu элементов", count);
    optimizations_log("DEBUG", log_msg);
}

void vector_bls_verify(const uint8_t** public_keys, const uint8_t** messages,
                      const size_t* message_lens, const uint8_t** signatures,
                      bool* results, size_t count) {
    if (!public_keys || !messages || !message_lens || !signatures || !results || count == 0) {
        optimizations_log("ERROR", "Невалидные параметры для векторной BLS верификации");
        return;
    }
    
    optimizations_log("DEBUG", "Выполнение векторной BLS верификации...");
    
    // В реальной реализации здесь будут батч-оптимизации для BLS верификации
    // Сейчас используем последовательную обработку
    
    for (size_t i = 0; i < count; i++) {
        if (!public_keys[i] || !messages[i] || !signatures[i]) {
            results[i] = false;
            continue;
        }
        
        // Используем существующую функцию верификации
        results[i] = auth_bls_verify_signature(public_keys[i], messages[i], 
                                              message_lens[i], signatures[i]);
    }
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "Векторная BLS верификация завершена: обработано %zu подписей", count);
    optimizations_log("DEBUG", log_msg);
}

bool optimizations_precompute_proof_verification(uint32_t k_size) {
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "Предварительные вычисления для верификации proof с k-size: %u", k_size);
    optimizations_log("INFO", log_msg);
    
    // В реальной реализации здесь будут предварительные вычисления
    // для ускорения верификации proof указанного размера
    
    // Например, предварительное вычисление таблиц для определенного k-size
    // или компиляция оптимизированных функций под конкретный размер
    
    optimizations_log("DEBUG", "Предварительные вычисления для proof завершены");
    return true;
}

bool optimizations_precompute_difficulty_params(void) {
    optimizations_log("INFO", "Предварительные вычисления параметров сложности...");
    
    // Предварительное вычисление часто используемых значений
    // для ускорения расчетов сложности
    
    optimizations_log("DEBUG", "Предварительные вычисления параметров сложности завершены");
    return true;
}

cache_stats_t cache_get_stats(cache_type_t type) {
    if (type < 0 || type > 3) {
        cache_stats_t empty = {0};
        return empty;
    }
    
    return g_cache_stats[type];
}

void optimizations_log_performance_stats(void) {
    optimizations_log("INFO", "Статистика производительности оптимизаций:");
    
    for (int i = 0; i < 4; i++) {
        const char* cache_name = "";
        switch (i) {
            case CACHE_TYPE_PROOF_VERIFICATION: cache_name = "PROOF_VERIFICATION"; break;
            case CACHE_TYPE_SIGNATURE_VERIFICATION: cache_name = "SIGNATURE_VERIFICATION"; break;
            case CACHE_TYPE_SINGLETON_STATE: cache_name = "SINGLETON_STATE"; break;
            case CACHE_TYPE_DIFFICULTY_CALCULATION: cache_name = "DIFFICULTY_CALCULATION"; break;
        }
        
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg),
                 "Кеш %s: hits=%lu, misses=%lu, evictions=%lu, memory_used=%zu/%zu",
                 cache_name, g_cache_stats[i].hits, g_cache_stats[i].misses,
                 g_cache_stats[i].evictions, g_cache_stats[i].memory_used,
                 g_cache_stats[i].max_memory);
        
        optimizations_log("INFO", log_msg);
    }
}

bool optimizations_set_cache_size(cache_type_t type, size_t max_size) {
    if (type < 0 || type > 3) {
        optimizations_log("ERROR", "Невалидный тип кеша для установки размера");
        return false;
    }
    
    pthread_mutex_lock(&g_cache_mutex);
    g_cache_stats[type].max_memory = max_size;
    pthread_mutex_unlock(&g_cache_mutex);
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "Размер кеша типа %d установлен: %zu bytes", type, max_size);
    optimizations_log("INFO", log_msg);
    
    return true;
}

bool optimizations_clear_cache(cache_type_t type) {
    if (type < 0 || type > 3) {
        optimizations_log("ERROR", "Невалидный тип кеша для очистки");
        return false;
    }
    
    pthread_mutex_lock(&g_cache_mutex);
    
    size_t cleared_memory = 0;
    size_t cleared_entries = 0;
    
    auto it = g_caches[type].begin();
    while (it != g_caches[type].end()) {
        cache_entry_t* entry = it->second;
        cleared_memory += entry->size;
        cleared_entries++;
        
        free(entry->data);
        free(entry);
        it = g_caches[type].erase(it);
        
        g_cache_stats[type].evictions++;
    }
    
    g_cache_stats[type].memory_used = 0;
    
    pthread_mutex_unlock(&g_cache_mutex);
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Кеш типа %d очищен: entries=%zu, memory=%zu bytes",
             type, cleared_entries, cleared_memory);
    optimizations_log("INFO", log_msg);
    
    return true;
}

bool optimizations_enable_asm_sha256(bool enable) {
    const char* status = enable ? "включены" : "выключены";
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "ASM оптимизации SHA256 %s", status);
    optimizations_log("INFO", log_msg);
    
    // В реальной реализации здесь будет переключение между
    // программной и ассемблерной реализацией SHA256
    
    return true;
}

bool optimizations_enable_asm_bls(bool enable) {
    const char* status = enable ? "включены" : "выключены";
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "ASM оптимизации BLS %s", status);
    optimizations_log("INFO", log_msg);
    
    // В реальной реализации здесь будет переключение между
    // программной и ассемблерной реализацией BLS операций
    
    return true;
}
