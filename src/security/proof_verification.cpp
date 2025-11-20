#include "security/proof_verification.h"
#include "../math_operations.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static void proof_log(const char* level, const char* message) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] [PROOF_VERIFICATION] [%s] %s\n", timestamp, level, message);
    fflush(stdout);
}

bool proof_verification_init(void) {
    proof_log("INFO", "Инициализация верификатора доказательств...");
    
    // Инициализация любых необходимых ресурсов
    // для верификации Proof of Space
    
    proof_log("INFO", "Верификатор доказательств успешно инициализирован");
    return true;
}

bool proof_verification_cleanup(void) {
    proof_log("INFO", "Очистка верификатора доказательств...");
    
    // Очистка ресурсов
    
    proof_log("INFO", "Верификатор доказательств очищен");
    return true;
}

proof_verification_result_t proof_verify_space(const uint8_t* proof_data, 
                                              size_t proof_length,
                                              const proof_verification_params_t* params,
                                              proof_metadata_t* metadata) {
    if (!proof_data || !params || !metadata) {
        proof_log("ERROR", "Невалидные параметры для верификации доказательства");
        return PROOF_INTERNAL_ERROR;
    }
    
    proof_log("DEBUG", "Начало верификации Proof of Space...");
    
    // Проверка размера плота (k-size)
    if (!proof_validate_k_size(params->k_size)) {
        proof_log("ERROR", "Невалидный размер плота (k-size)");
        return PROOF_INVALID_K_SIZE;
    }
    
    // Проверка качества доказательства
    uint64_t quality;
    if (!proof_validate_quality(proof_data, params->k_size, params->challenge, &quality)) {
        proof_log("ERROR", "Невалидное качество доказательства");
        return PROOF_INVALID_QUALITY;
    }
    
    metadata->quality = quality;
    
    // Проверка итераций
    uint64_t iterations;
    if (!proof_validate_iterations(quality, params->difficulty, 
                                  params->sub_slot_iters, &iterations)) {
        proof_log("ERROR", "Невалидное количество итераций");
        return PROOF_INVALID_ITERATIONS;
    }
    
    metadata->iterations = iterations;
    metadata->proof_size = proof_length;
    
    // Извлекаем информацию о плоте из доказательства
    // В реальной реализации здесь будет парсинг структуры доказательства
    
    // Упрощенное извлечение plot_id (первые 32 байта proof)
    memcpy(metadata->plot_id, proof_data, 32);
    
    proof_log("DEBUG", "Proof of Space верифицирован успешно");
    return PROOF_VALID;
}

bool proof_validate_quality(const uint8_t* proof_data, uint32_t k_size, 
                           uint64_t challenge, uint64_t* quality) {
    if (!proof_data || !quality) {
        proof_log("ERROR", "Невалидные параметры для проверки качества");
        return false;
    }
    
    // В реальной реализации здесь будет вычисление качества доказательства
    // по алгоритму Chia Proof of Space
    
    // Упрощенная версия - вычисляем "качество" на основе первых 8 байт proof
    *quality = *(uint64_t*)proof_data;
    
    // Нормализуем качество
    if (*quality == 0) {
        *quality = 1; // Минимальное качество
    }
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "Качество доказательства: %lu (k-size: %u)", *quality, k_size);
    proof_log("DEBUG", log_msg);
    
    return true;
}

bool proof_validate_iterations(uint64_t quality, uint64_t difficulty, 
                              uint64_t sub_slot_iters, uint64_t* iterations) {
    if (!iterations) {
        proof_log("ERROR", "Iterations pointer не может быть NULL");
        return false;
    }
    
    if (quality == 0) {
        proof_log("ERROR", "Качество не может быть нулевым");
        return false;
    }
    
    if (difficulty == 0) {
        proof_log("ERROR", "Сложность не может быть нулевой");
        return false;
    }
    
    // Вычисляем итерации по формуле Chia
    // iterations = (sub_slot_iters * difficulty * (2^64)) / (quality * 1000000000000)
    
    uint64_t numerator = sub_slot_iters * difficulty;
    uint64_t denominator = quality;
    
    // Предотвращаем деление на ноль
    if (denominator == 0) {
        *iterations = 0;
        proof_log("ERROR", "Деноминатор равен нулю при вычислении итераций");
        return false;
    }
    
    *iterations = numerator / denominator;
    
    // Применяем масштабирующий фактор (упрощенно)
    *iterations = *iterations / 1000000;
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Вычислены итерации: quality=%lu, difficulty=%lu, "
             "sub_slot_iters=%lu, iterations=%lu",
             quality, difficulty, sub_slot_iters, *iterations);
    proof_log("DEBUG", log_msg);
    
    return true;
}

bool proof_validate_k_size(uint32_t k_size) {
    // Проверяем, что k-size находится в допустимом диапазоне
    bool valid = (k_size >= 25 && k_size <= 50);
    
    if (!valid) {
        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), 
                 "Невалидный k-size: %u (допустимый диапазон: 25-50)", k_size);
        proof_log("ERROR", log_msg);
    } else {
        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "k-size валиден: %u", k_size);
        proof_log("DEBUG", log_msg);
    }
    
    return valid;
}

void proof_log_verification_result(proof_verification_result_t result, 
                                  const uint8_t* plot_id) {
    const char* result_str = "UNKNOWN";
    
    switch (result) {
        case PROOF_VALID: result_str = "VALID"; break;
        case PROOF_INVALID_FORMAT: result_str = "INVALID_FORMAT"; break;
        case PROOF_INVALID_QUALITY: result_str = "INVALID_QUALITY"; break;
        case PROOF_INVALID_ITERATIONS: result_str = "INVALID_ITERATIONS"; break;
        case PROOF_INVALID_DIFFICULTY: result_str = "INVALID_DIFFICULTY"; break;
        case PROOF_INVALID_K_SIZE: result_str = "INVALID_K_SIZE"; break;
        case PROOF_INTERNAL_ERROR: result_str = "INTERNAL_ERROR"; break;
    }
    
    char plot_id_hex[65] = {0};
    if (plot_id) {
        for (int i = 0; i < 32; i++) {
            sprintf(plot_id_hex + i * 2, "%02x", plot_id[i]);
        }
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), 
             "Результат верификации Proof: %s, plot=%s", 
             result_str, plot_id_hex);
    
    if (result == PROOF_VALID) {
        proof_log("INFO", log_msg);
    } else {
        proof_log("WARNING", log_msg);
    }
}

bool proof_calculate_points(uint64_t quality, uint64_t difficulty, uint64_t* points) {
    if (!points) {
        proof_log("ERROR", "Points pointer не может быть NULL");
        return false;
    }
    
    if (quality == 0 || difficulty == 0) {
        *points = 0;
        proof_log("ERROR", "Качество или сложность равны нулю");
        return false;
    }
    
    // Вычисляем очки на основе качества и сложности
    // points = (quality * 1000000) / difficulty
    
    *points = (quality * 1000000) / difficulty;
    
    // Минимальное количество очков - 1
    if (*points == 0) {
        *points = 1;
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Вычислены очки: quality=%lu, difficulty=%lu, points=%lu",
             quality, difficulty, *points);
    proof_log("DEBUG", log_msg);
    
    return true;
}

bool proof_verify_space_optimized(const uint8_t* proof_data, uint32_t k_size,
                                 uint64_t challenge, uint64_t* quality) {
    if (!proof_data || !quality) {
        proof_log("ERROR", "Невалидные параметры для оптимизированной верификации");
        return false;
    }
    
    // Оптимизированная версия верификации Proof of Space
    // В реальной реализации здесь будут ассемблерные оптимизации
    
    // Упрощенная реализация - используем базовую функцию
    bool result = proof_validate_quality(proof_data, k_size, challenge, quality);
    
    proof_log("DEBUG", "Оптимизированная верификация Proof of Space завершена");
    return result;
}
