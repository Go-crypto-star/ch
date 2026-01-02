#ifndef OPTIMIZATIONS_H
#define OPTIMIZATIONS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Типы кешей
typedef enum {
    CACHE_TYPE_PROOF_VERIFICATION,
    CACHE_TYPE_SIGNATURE_VERIFICATION,
    CACHE_TYPE_SINGLETON_STATE,
    CACHE_TYPE_DIFFICULTY_CALCULATION
} cache_type_t;

// Статистика кеша
typedef struct {
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    size_t memory_used;
    size_t max_memory;
} cache_stats_t;

// Конфигурация оптимизаций
typedef struct {
    bool enable_proof_cache;
    bool enable_signature_cache;
    bool enable_vectorization;
    bool enable_asm_optimizations;
    size_t max_cache_memory;
    uint32_t cache_ttl_seconds;
} optimizations_config_t;

// Инициализация оптимизаций
bool optimizations_init(const optimizations_config_t* config);
bool optimizations_cleanup(void);

// Управление кешем
bool cache_put(cache_type_t type, const uint8_t* key, size_t key_len, 
               const void* value, size_t value_size);
void* cache_get(cache_type_t type, const uint8_t* key, size_t key_len);
bool cache_remove(cache_type_t type, const uint8_t* key, size_t key_len);

// Векторизованные операции
void vector_sha256(const uint8_t** inputs, const size_t* input_lens, 
                   uint8_t** outputs, size_t count);
void vector_bls_verify(const uint8_t** public_keys, const uint8_t** messages,
                      const size_t* message_lens, const uint8_t** signatures,
                      bool* results, size_t count);

// Предварительные вычисления
bool optimizations_precompute_proof_verification(uint32_t k_size);
bool optimizations_precompute_difficulty_params(void);

// Мониторинг производительности
cache_stats_t cache_get_stats(cache_type_t type);
void optimizations_log_performance_stats(void);

// Управление памятью
bool optimizations_set_cache_size(cache_type_t type, size_t max_size);
bool optimizations_clear_cache(cache_type_t type);

// ASM оптимизации
bool optimizations_enable_asm_sha256(bool enable);
bool optimizations_enable_asm_bls(bool enable);

#endif // OPTIMIZATIONS_H
