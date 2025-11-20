#ifndef PROOF_VERIFICATION_H
#define PROOF_VERIFICATION_H

#include <cstddef>
#include <stdint.h>
#include <stdbool.h>

// Параметры верификации Proof of Space
typedef struct {
    uint64_t challenge;
    uint32_t k_size;
    uint64_t sub_slot_iters;      // 37.6 миллиардов для пула
    uint32_t difficulty;
    uint64_t required_iterations;
} proof_verification_params_t;

// Результат верификации
typedef enum {
    PROOF_VALID,
    PROOF_INVALID_FORMAT,
    PROOF_INVALID_QUALITY,
    PROOF_INVALID_ITERATIONS,
    PROOF_INVALID_DIFFICULTY,
    PROOF_INVALID_K_SIZE,
    PROOF_INTERNAL_ERROR
} proof_verification_result_t;

// Метаданные доказательства
typedef struct {
    uint8_t plot_id[32];
    uint8_t plot_public_key[48];
    uint8_t farmer_public_key[48];
    uint8_t pool_public_key[48];
    uint64_t quality;
    uint64_t iterations;
    uint32_t proof_size;
} proof_metadata_t;

// Инициализация верификатора
bool proof_verification_init(void);
bool proof_verification_cleanup(void);

// Основная функция верификации
proof_verification_result_t proof_verify_space(const uint8_t* proof_data, 
                                              size_t proof_length,
                                              const proof_verification_params_t* params,
                                              proof_metadata_t* metadata);

// Валидация качества
bool proof_validate_quality(const uint8_t* proof_data, uint32_t k_size, 
                           uint64_t challenge, uint64_t* quality);

// Валидация итераций
bool proof_validate_iterations(uint64_t quality, uint64_t difficulty, 
                              uint64_t sub_slot_iters, uint64_t* iterations);

// Проверка размера плота
bool proof_validate_k_size(uint32_t k_size);

// Утилиты
void proof_log_verification_result(proof_verification_result_t result, 
                                  const uint8_t* plot_id);
bool proof_calculate_points(uint64_t quality, uint64_t difficulty, uint64_t* points);

// Оптимизированные версии (для ASM)
bool proof_verify_space_optimized(const uint8_t* proof_data, uint32_t k_size,
                                 uint64_t challenge, uint64_t* quality);

#endif // PROOF_VERIFICATION_H
