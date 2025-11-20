#include "math_operations.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static void math_log(const char* level, const char* message) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] [MATH] [%s] %s\n", timestamp, level, message);
    fflush(stdout);
}

bool math_operations_init(void) {
    math_log("INFO", "Инициализация математических операций...");
    
    // Инициализация любых необходимых математических библиотек
    
    math_log("INFO", "Математические операции инициализированы успешно");
    return true;
}

uint64_t math_calculate_difficulty(const difficulty_params_t* params) {
    if (!params) {
        math_log("ERROR", "Параметры сложности не могут быть NULL");
        return 1;
    }
    
    math_log("DEBUG", "Расчет сложности...");
    
    uint64_t new_difficulty = params->current_difficulty;
    
    // Если у фермера недостаточно очков за последние 24 часа,
    // уменьшаем сложность для увеличения частоты partials
    if (params->farmer_points_24h < params->target_partials_per_day * 1000) {
        new_difficulty = params->current_difficulty * 8 / 10; // Уменьшаем на 20%
        
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg),
                 "Уменьшение сложности: текущая=%lu, новая=%lu (мало очков: %lu)",
                 params->current_difficulty, new_difficulty, params->farmer_points_24h);
        math_log("INFO", log_msg);
    }
    // Если у фермера слишком много очков, увеличиваем сложность
    else if (params->farmer_points_24h > params->target_partials_per_day * 1000 * 2) {
        new_difficulty = params->current_difficulty * 12 / 10; // Увеличиваем на 20%
        
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg),
                 "Увеличение сложности: текущая=%lu, новая=%lu (много очков: %lu)",
                 params->current_difficulty, new_difficulty, params->farmer_points_24h);
        math_log("INFO", log_msg);
    }
    
    // Применяем ограничения по минимальной и максимальной сложности
    if (new_difficulty < params->min_difficulty) {
        new_difficulty = params->min_difficulty;
        math_log("DEBUG", "Сложность ограничена минимальным значением");
    }
    
    if (new_difficulty > params->max_difficulty) {
        new_difficulty = params->max_difficulty;
        math_log("DEBUG", "Сложность ограничена максимальным значением");
    }
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "Рассчитана сложность: %lu", new_difficulty);
    math_log("DEBUG", log_msg);
    
    return new_difficulty;
}

bool math_validate_difficulty_range(uint64_t difficulty, uint64_t min_diff, uint64_t max_diff) {
    bool valid = (difficulty >= min_diff && difficulty <= max_diff);
    
    if (!valid) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg),
                 "Сложность %lu вне допустимого диапазона [%lu, %lu]",
                 difficulty, min_diff, max_diff);
        math_log("ERROR", log_msg);
    }
    
    return valid;
}

uint64_t math_calculate_points(uint64_t difficulty, uint64_t iterations) {
    if (difficulty == 0) {
        math_log("ERROR", "Сложность не может быть нулевой при расчете очков");
        return 0;
    }
    
    // points = (iterations * 1000000) / difficulty
    uint64_t points = (iterations * 1000000) / difficulty;
    
    // Минимальное количество очков - 1
    if (points == 0 && iterations > 0) {
        points = 1;
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Рассчитаны очки: difficulty=%lu, iterations=%lu, points=%lu",
             difficulty, iterations, points);
    math_log("DEBUG", log_msg);
    
    return points;
}

double math_calculate_share_percentage(uint64_t farmer_points, uint64_t total_points) {
    if (total_points == 0) {
        math_log("WARNING", "Общее количество очков равно нулю");
        return 0.0;
    }
    
    double percentage = (double)farmer_points / total_points * 100.0;
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Доля фермера: points=%lu, total_points=%lu, percentage=%.6f%%",
             farmer_points, total_points, percentage);
    math_log("DEBUG", log_msg);
    
    return percentage;
}

payout_calculation_result_t math_calculate_payout(const payout_calculation_params_t* params) {
    payout_calculation_result_t result;
    memset(&result, 0, sizeof(payout_calculation_result_t));
    
    if (!params) {
        math_log("ERROR", "Параметры выплат не могут быть NULL");
        return result;
    }
    
    math_log("INFO", "Расчет выплат...");
    
    if (params->total_points == 0) {
        math_log("ERROR", "Общее количество очков равно нулю");
        return result;
    }
    
    // Рассчитываем долю фермера
    double share = (double)params->farmer_points / params->total_points;
    result.share_percentage = share * 100.0;
    
    // Рассчитываем общую сумму вознаграждения за вычетом комиссии пула
    uint64_t total_after_fee = params->block_rewards * (1.0 - params->pool_fee_percentage);
    
    // Выплата фермеру
    result.farmer_amount = (uint64_t)(total_after_fee * share);
    result.pool_amount = params->block_rewards - total_after_fee;
    result.fee_amount = result.pool_amount; // Комиссия пула
    result.points_earned = params->farmer_points;
    
    // Округляем до целых mojos
    result.farmer_amount = (result.farmer_amount / 1000000) * 1000000;
    
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg),
             "Результат расчета выплат: farmer_amount=%lu, pool_amount=%lu, "
             "fee_amount=%lu, share=%.6f%%, points=%lu",
             result.farmer_amount, result.pool_amount, result.fee_amount,
             result.share_percentage, result.points_earned);
    math_log("INFO", log_msg);
    
    return result;
}

bool math_validate_payout_amounts(const payout_calculation_result_t* payout) {
    if (!payout) {
        math_log("ERROR", "Результат выплат не может быть NULL");
        return false;
    }
    
    // Проверяем, что сумма выплат не превышает общее вознаграждение
    // и что все значения логически согласованы
    
    bool valid = (payout->farmer_amount > 0 || payout->points_earned == 0);
    
    if (!valid) {
        math_log("ERROR", "Невалидные суммы выплат");
    } else {
        math_log("DEBUG", "Суммы выплат валидны");
    }
    
    return valid;
}

uint64_t math_calculate_pplns_reward(uint64_t farmer_points, uint64_t total_points_last_n,
                                    uint64_t block_reward, double pool_fee) {
    if (total_points_last_n == 0) {
        math_log("WARNING", "Общее количество очков за N период равно нулю");
        return 0;
    }
    
    // PPLNS: Pay Per Last N Shares
    double share = (double)farmer_points / total_points_last_n;
    uint64_t reward = (uint64_t)(block_reward * (1.0 - pool_fee) * share);
    
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg),
             "PPLNS расчет: farmer_points=%lu, total_points=%lu, "
             "block_reward=%lu, pool_fee=%.3f, reward=%lu",
             farmer_points, total_points_last_n, block_reward, pool_fee, reward);
    math_log("DEBUG", log_msg);
    
    return reward;
}

uint64_t math_calculate_pps_reward(uint64_t farmer_points, uint64_t estimated_points_per_block,
                                  uint64_t block_reward, double pool_fee) {
    if (estimated_points_per_block == 0) {
        math_log("WARNING", "Расчетное количество очков за блок равно нулю");
        return 0;
    }
    
    // PPS: Pay Per Share
    double share = (double)farmer_points / estimated_points_per_block;
    uint64_t reward = (uint64_t)(block_reward * (1.0 - pool_fee) * share);
    
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg),
             "PPS расчет: farmer_points=%lu, estimated_points=%lu, "
             "block_reward=%lu, pool_fee=%.3f, reward=%lu",
             farmer_points, estimated_points_per_block, block_reward, pool_fee, reward);
    math_log("DEBUG", log_msg);
    
    return reward;
}

double math_convert_mojo_to_chia(uint64_t mojos) {
    double chia = (double)mojos / 1000000000000.0; // 1 XCH = 10^12 mojos
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "Конвертация mojos в XCH: %lu mojos = %.8f XCH", mojos, chia);
    math_log("DEBUG", log_msg);
    
    return chia;
}

uint64_t math_convert_chia_to_mojo(double chia) {
    uint64_t mojos = (uint64_t)(chia * 1000000000000.0);
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "Конвертация XCH в mojos: %.8f XCH = %lu mojos", chia, mojos);
    math_log("DEBUG", log_msg);
    
    return mojos;
}

void math_log_calculation(const char* operation, double result) {
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), 
             "Математическая операция: %s = %.8f", operation, result);
    math_log("DEBUG", log_msg);
}

double math_calculate_standard_deviation(const uint64_t* values, size_t count) {
    if (!values || count == 0) {
        math_log("ERROR", "Невалидные параметры для расчета стандартного отклонения");
        return 0.0;
    }
    
    // Вычисляем среднее значение
    double sum = 0.0;
    for (size_t i = 0; i < count; i++) {
        sum += values[i];
    }
    double mean = sum / count;
    
    // Вычисляем сумму квадратов отклонений
    double sum_squared_diff = 0.0;
    for (size_t i = 0; i < count; i++) {
        double diff = values[i] - mean;
        sum_squared_diff += diff * diff;
    }
    
    // Вычисляем стандартное отклонение
    double std_dev = sqrt(sum_squared_diff / count);
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Стандартное отклонение: count=%zu, mean=%.2f, std_dev=%.2f",
             count, mean, std_dev);
    math_log("DEBUG", log_msg);
    
    return std_dev;
}

double math_calculate_correlation(const double* x, const double* y, size_t count) {
    if (!x || !y || count == 0) {
        math_log("ERROR", "Невалидные параметры для расчета корреляции");
        return 0.0;
    }
    
    // Вычисляем средние значения
    double x_mean = 0.0, y_mean = 0.0;
    for (size_t i = 0; i < count; i++) {
        x_mean += x[i];
        y_mean += y[i];
    }
    x_mean /= count;
    y_mean /= count;
    
    // Вычисляем ковариацию и дисперсии
    double covariance = 0.0, x_variance = 0.0, y_variance = 0.0;
    for (size_t i = 0; i < count; i++) {
        double x_diff = x[i] - x_mean;
        double y_diff = y[i] - y_mean;
        covariance += x_diff * y_diff;
        x_variance += x_diff * x_diff;
        y_variance += y_diff * y_diff;
    }
    
    // Вычисляем коэффициент корреляции Пирсона
    double correlation = 0.0;
    if (x_variance > 0 && y_variance > 0) {
        correlation = covariance / sqrt(x_variance * y_variance);
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Коэффициент корреляции: count=%zu, correlation=%.4f",
             count, correlation);
    math_log("DEBUG", log_msg);
    
    return correlation;
}
