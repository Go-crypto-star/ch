#include <gtest/gtest.h>
#include "math_operations.h"
#include <cmath>

class MathOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        math_operations_init();
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

TEST_F(MathOperationsTest, CalculatePointsBasic) {
    uint64_t difficulty = 1000;
    uint64_t iterations = 500000;
    
    uint64_t points = math_calculate_points(difficulty, iterations);
    
    EXPECT_GT(points, 0);
    EXPECT_LE(points, iterations);
    
    // points = (iterations * 1,000,000) / difficulty
    uint64_t expected_points = (iterations * 1000000) / difficulty;
    EXPECT_EQ(points, expected_points);
}

TEST_F(MathOperationsTest, CalculatePointsZeroDifficulty) {
    uint64_t points = math_calculate_points(0, 1000);
    EXPECT_EQ(points, 0);
}

TEST_F(MathOperationsTest, CalculateDifficultyAdjustment) {
    difficulty_params_t params = {
        .target_partials_per_day = 300,
        .current_difficulty = 1000,
        .farmer_points_24h = 150000,  // Меньше целевого
        .time_since_last_partial = 3600,
        .min_difficulty = 100,
        .max_difficulty = 10000
    };
    
    uint64_t new_difficulty = math_calculate_difficulty(&params);
    
    EXPECT_GE(new_difficulty, params.min_difficulty);
    EXPECT_LE(new_difficulty, params.max_difficulty);
    EXPECT_LT(new_difficulty, params.current_difficulty); // Должна уменьшиться
}

TEST_F(MathOperationsTest, CalculateDifficultyIncrease) {
    difficulty_params_t params = {
        .target_partials_per_day = 300,
        .current_difficulty = 1000,
        .farmer_points_24h = 600000,  // Больше целевого
        .time_since_last_partial = 3600,
        .min_difficulty = 100,
        .max_difficulty = 10000
    };
    
    uint64_t new_difficulty = math_calculate_difficulty(&params);
    
    EXPECT_GT(new_difficulty, params.current_difficulty); // Должна увеличиться
}

TEST_F(MathOperationsTest, CalculatePayoutPPLNS) {
    payout_calculation_params_t params = {
        .total_points = 1000000,
        .pool_points = 10000,
        .farmer_points = 50000,
        .pool_fee_percentage = 0.01, // 1%
        .block_rewards = 1750000000000, // 1.75 XCH в mojos
        .total_netspace = 1000000000000000, // 1 EiB
        .farmer_netspace = 1000000000000 // 1 TiB
    };
    
    payout_calculation_result_t result = math_calculate_payout(&params);
    
    EXPECT_GT(result.farmer_amount, 0);
    EXPECT_GT(result.pool_amount, 0);
    EXPECT_GT(result.fee_amount, 0);
    EXPECT_GT(result.share_percentage, 0);
    EXPECT_EQ(result.points_earned, params.farmer_points);
    
    // Проверяем что сумма выплат не превышает награду за блок
    EXPECT_LE(result.farmer_amount + result.pool_amount, params.block_rewards);
    
    // Проверяем корректность расчета доли
    double expected_share = (double)params.farmer_points / params.total_points * 100.0;
    EXPECT_NEAR(result.share_percentage, expected_share, 0.001);
}

TEST_F(MathOperationsTest, CalculatePayoutZeroPoints) {
    payout_calculation_params_t params = {
        .total_points = 0,
        .pool_points = 0,
        .farmer_points = 0,
        .pool_fee_percentage = 0.01,
        .block_rewards = 1750000000000,
        .total_netspace = 1000000000000000,
        .farmer_netspace = 1000000000000
    };
    
    payout_calculation_result_t result = math_calculate_payout(&params);
    
    EXPECT_EQ(result.farmer_amount, 0);
    EXPECT_EQ(result.points_earned, 0);
}

TEST_F(MathOperationsTest, PPLNSRewardCalculation) {
    uint64_t farmer_points = 50000;
    uint64_t total_points_last_n = 1000000;
    uint64_t block_reward = 1750000000000;
    double pool_fee = 0.01;
    
    uint64_t reward = math_calculate_pplns_reward(
        farmer_points, total_points_last_n, block_reward, pool_fee);
    
    double share = (double)farmer_points / total_points_last_n;
    uint64_t expected_reward = (uint64_t)(block_reward * (1.0 - pool_fee) * share);
    
    EXPECT_NEAR(reward, expected_reward, 1000); // Допуск 1000 mojos
}

TEST_F(MathOperationsTest, PPSRewardCalculation) {
    uint64_t farmer_points = 50000;
    uint64_t estimated_points_per_block = 500000;
    uint64_t block_reward = 1750000000000;
    double pool_fee = 0.01;
    
    uint64_t reward = math_calculate_pps_reward(
        farmer_points, estimated_points_per_block, block_reward, pool_fee);
    
    double share = (double)farmer_points / estimated_points_per_block;
    uint64_t expected_reward = (uint64_t)(block_reward * (1.0 - pool_fee) * share);
    
    EXPECT_NEAR(reward, expected_reward, 1000);
}

TEST_F(MathOperationsTest, MojoToChiaConversion) {
    uint64_t mojos = 1000000000000; // 1 XCH
    double chia = math_convert_mojo_to_chia(mojos);
    
    EXPECT_DOUBLE_EQ(chia, 1.0);
}

TEST_F(MathOperationsTest, ChiaToMojoConversion) {
    double chia = 1.5; // 1.5 XCH
    uint64_t mojos = math_convert_chia_to_mojo(chia);
    
    EXPECT_EQ(mojos, 1500000000000);
}

TEST_F(MathOperationsTest, SharePercentageCalculation) {
    double percentage = math_calculate_share_percentage(25000, 100000);
    
    EXPECT_DOUBLE_EQ(percentage, 25.0);
}

TEST_F(MathOperationsTest, SharePercentageZeroTotal) {
    double percentage = math_calculate_share_percentage(1000, 0);
    
    EXPECT_DOUBLE_EQ(percentage, 0.0);
}

TEST_F(MathOperationsTest, StandardDeviationCalculation) {
    uint64_t values[] = {10, 12, 23, 23, 16, 23, 21, 16};
    size_t count = sizeof(values) / sizeof(values[0]);
    
    double std_dev = math_calculate_standard_deviation(values, count);
    
    // Предварительно рассчитанное стандартное отклонение
    double expected_std_dev = 5.2372293656638;
    EXPECT_NEAR(std_dev, expected_std_dev, 0.001);
}

TEST_F(MathOperationsTest, CorrelationCalculation) {
    double x[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double y[] = {2.0, 4.0, 6.0, 8.0, 10.0};
    size_t count = sizeof(x) / sizeof(x[0]);
    
    double correlation = math_calculate_correlation(x, y, count);
    
    // Идеальная положительная корреляция
    EXPECT_NEAR(correlation, 1.0, 0.001);
}

TEST_F(MathOperationsTest, ValidateDifficultyRange) {
    EXPECT_TRUE(math_validate_difficulty_range(500, 100, 1000));
    EXPECT_FALSE(math_validate_difficulty_range(50, 100, 1000));
    EXPECT_FALSE(math_validate_difficulty_range(1500, 100, 1000));
}

TEST_F(MathOperationsTest, ValidatePayoutAmounts) {
    payout_calculation_result_t valid_payout = {
        .farmer_amount = 1000000,
        .pool_amount = 100000,
        .fee_amount = 100000,
        .points_earned = 50000,
        .share_percentage = 5.0
    };
    
    EXPECT_TRUE(math_validate_payout_amounts(&valid_payout));
    
    payout_calculation_result_t invalid_payout = {
        .farmer_amount = 0,
        .pool_amount = 0,
        .fee_amount = 0,
        .points_earned = 10000, // Очки есть, но выплата 0
        .share_percentage = 1.0
    };
    
    EXPECT_FALSE(math_validate_payout_amounts(&invalid_payout));
}
