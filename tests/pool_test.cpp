#include <gtest/gtest.h>
#include "pool_core.h"
#include "protocol/partials.h"
#include "protocol/singleton.h"
#include <cstring>

class PoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Загружаем тестовую конфигурацию
        pool_config_t config;
        memset(&config, 0, sizeof(pool_config_t));
        strcpy(config.pool_name, "Test Pool");
        strcpy(config.pool_url, "https://test.pool.example.com");
        config.port = 8444;
        config.pool_fee = 0.01;
        config.min_payout = 1000000000;
        config.partial_deadline = 28;
        config.difficulty_target = 300;
        strcpy(config.node_rpc_host, "localhost");
        config.node_rpc_port = 8555;
        
        pool_init(&config);
    }

    void TearDown() override {
        pool_cleanup();
    }
};

TEST_F(PoolTest, PoolInitialization) {
    pool_context_t* context = pool_get_context();
    ASSERT_NE(context, nullptr);
    
    EXPECT_EQ(context->state, POOL_STATE_INIT);
    EXPECT_STREQ(context->config.pool_name, "Test Pool");
    EXPECT_EQ(context->config.pool_fee, 0.01);
}

TEST_F(PoolTest, PoolStartStop) {
    bool start_result = pool_start();
    EXPECT_TRUE(start_result);
    
    pool_context_t* context = pool_get_context();
    EXPECT_EQ(context->state, POOL_STATE_RUNNING);
    
    bool stop_result = pool_stop();
    EXPECT_TRUE(stop_result);
}

TEST_F(PoolTest, ValidateConfig) {
    pool_config_t valid_config;
    pool_load_default_config(&valid_config);
    
    bool result = pool_validate_config(&valid_config);
    EXPECT_TRUE(result);
}

TEST_F(PoolTest, ValidateInvalidConfig) {
    pool_config_t invalid_config;
    memset(&invalid_config, 0, sizeof(pool_config_t));
    
    bool result = pool_validate_config(&invalid_config);
    EXPECT_FALSE(result);
}

TEST_F(PoolTest, PartialQueueOperations) {
    partial_queue_t queue;
    bool init_result = partial_queue_init(&queue, 100);
    EXPECT_TRUE(init_result);
    
    // Тестируем добавление partial
    partial_t partial;
    memset(&partial, 0, sizeof(partial_t));
    partial.timestamp = time(NULL);
    partial.difficulty = 1000;
    
    bool push_result = partial_queue_push(&queue, &partial);
    EXPECT_TRUE(push_result);
    EXPECT_EQ(queue.size, 1);
    
    // Тестируем извлечение partial
    partial_t popped_partial;
    bool pop_result = partial_queue_pop(&queue, &popped_partial);
    EXPECT_TRUE(pop_result);
    EXPECT_EQ(queue.size, 0);
    EXPECT_EQ(popped_partial.difficulty, 1000);
    
    partial_queue_cleanup(&queue);
}

TEST_F(PoolTest, PartialQueueOverflow) {
    partial_queue_t queue;
    partial_queue_init(&queue, 2); // Маленький размер
    
    partial_t partial;
    memset(&partial, 0, sizeof(partial_t));
    
    // Добавляем до предела
    EXPECT_TRUE(partial_queue_push(&queue, &partial));
    EXPECT_TRUE(partial_queue_push(&queue, &partial));
    
    // Третья попытка должна fail
    EXPECT_FALSE(partial_queue_push(&queue, &partial));
    
    partial_queue_cleanup(&queue);
}

TEST_F(PoolTest, PartialValidation) {
    partial_t partial;
    memset(&partial, 0, sizeof(partial_t));
    
    // Устанавливаем актуальную временную метку
    partial.timestamp = time(NULL);
    
    partial_validation_result_t result = partial_validate(&partial);
    
    // В тестовом режиме ожидаем ошибку синглтона (нет реального синглтона)
    EXPECT_NE(result, PARTIAL_VALID);
}

TEST_F(PoolTest, PartialValidationLate) {
    partial_t partial;
    memset(&partial, 0, sizeof(partial_t));
    
    // Устанавливаем старую временную метку (больше 28 секунд)
    partial.timestamp = time(NULL) - 30;
    
    partial_validation_result_t result = partial_validate(&partial);
    
    EXPECT_EQ(result, PARTIAL_TOO_LATE);
}

TEST_F(PoolTest, SingletonInitialization) {
    uint8_t launcher_id[32] = {0x01, 0x02, 0x03};
    singleton_t singleton;
    
    bool result = singleton_init(launcher_id, &singleton);
    
    // В тестовом режиме без реального блокчейна ожидаем false
    EXPECT_FALSE(result);
}

TEST_F(PoolTest, SingletonOwnershipValidation) {
    singleton_t singleton;
    memset(&singleton, 0, sizeof(singleton_t));
    
    uint8_t signature[96] = {0};
    bool result = singleton_validate_ownership(&singleton, signature);
    
    // Без реальных данных ожидаем false
    EXPECT_FALSE(result);
}

TEST_F(PoolTest, PoolStatistics) {
    pool_context_t* context = pool_get_context();
    
    // Инициализируем статистику
    context->stats.total_farmers = 10;
    context->stats.total_partials = 100;
    context->stats.valid_partials = 95;
    context->stats.invalid_partials = 5;
    context->stats.total_points = 50000;
    
    // Логируем статистику (должно работать без ошибок)
    EXPECT_NO_FATAL_FAILURE(pool_log_statistics());
}

TEST_F(PoolTest, ErrorHandling) {
    const char* error_msg = "Test error message";
    pool_set_error(error_msg);
    
    const char* retrieved_error = pool_get_last_error();
    EXPECT_STREQ(retrieved_error, error_msg);
}

TEST_F(PoolTest, StateConversion) {
    EXPECT_STREQ(pool_state_to_string(POOL_STATE_INIT), "INIT");
    EXPECT_STREQ(pool_state_to_string(POOL_STATE_RUNNING), "RUNNING");
    EXPECT_STREQ(pool_state_to_string(POOL_STATE_SHUTTING_DOWN), "SHUTTING_DOWN");
    EXPECT_STREQ(pool_state_to_string(POOL_STATE_ERROR), "ERROR");
    EXPECT_STREQ(pool_state_to_string((pool_state_t)999), "UNKNOWN");
}

TEST_F(PoolTest, PartialProcessing) {
    partial_t partial;
    memset(&partial, 0, sizeof(partial_t));
    partial.timestamp = time(NULL);
    
    bool result = partial_process(&partial);
    
    // В тестовом режиме без реальных данных ожидаем false
    EXPECT_FALSE(result);
}

TEST_F(PoolTest, PartialsStatistics) {
    // Имитируем обработку нескольких partials
    partial_t partial;
    memset(&partial, 0, sizeof(partial_t));
    partial.timestamp = time(NULL);
    
    for (int i = 0; i < 10; i++) {
        partial_validate(&partial);
    }
    
    uint64_t valid, invalid, total;
    partials_get_stats(&valid, &invalid, &total);
    
    EXPECT_EQ(total, 10);
    EXPECT_LE(valid, 10);
    EXPECT_LE(invalid, 10);
    EXPECT_EQ(valid + invalid, total);
}
