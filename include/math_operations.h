#ifndef MATH_OPERATIONS_H
#define MATH_OPERATIONS_H

#include <stdint.h>
#include <stdbool.h>

// Параметры расчета сложности
typedef struct {
    uint64_t target_partials_per_day;
    uint64_t current_difficulty;
    uint64_t farmer_points_24h;
    uint64_t time_since_last_partial;
    uint32_t min_difficulty;
    uint32_t max_difficulty;
} difficulty_params_t;

// Статистика для расчета выплат
typedef struct {
    uint64_t total_points;
    uint64_t pool_points;
    uint64_t farmer_points;
    double pool_fee_percentage;
    uint64_t block_rewards;
    uint64_t total_netspace;
    uint64_t farmer_netspace;
} payout_calculation_params_t;

// Результат расчета выплат
typedef struct {
    uint64_t farmer_amount;
    uint64_t pool_amount;
    uint64_t fee_amount;
    uint64_t points_earned;
    double share_percentage;
} payout_calculation_result_t;

// Инициализация математического модуля
bool math_operations_init(void);

// Расчет сложности
uint64_t math_calculate_difficulty(const difficulty_params_t* params);
bool math_validate_difficulty_range(uint64_t difficulty, uint64_t min_diff, uint64_t max_diff);

// Расчет очков
uint64_t math_calculate_points(uint64_t difficulty, uint64_t iterations);
double math_calculate_share_percentage(uint64_t farmer_points, uint64_t total_points);

// Расчет выплат
payout_calculation_result_t math_calculate_payout(const payout_calculation_params_t* params);
bool math_validate_payout_amounts(const payout_calculation_result_t* payout);

// PPLNS расчет
uint64_t math_calculate_pplns_reward(uint64_t farmer_points, uint64_t total_points_last_n,
                                    uint64_t block_reward, double pool_fee);

// PPS расчет
uint64_t math_calculate_pps_reward(uint64_t farmer_points, uint64_t estimated_points_per_block,
                                  uint64_t block_reward, double pool_fee);

// Утилиты
double math_convert_mojo_to_chia(uint64_t mojos);
uint64_t math_convert_chia_to_mojo(double chia);
void math_log_calculation(const char* operation, double result);

// Статистические функции
double math_calculate_standard_deviation(const uint64_t* values, size_t count);
double math_calculate_correlation(const double* x, const double* y, size_t count);

#endif // MATH_OPERATIONS_H
