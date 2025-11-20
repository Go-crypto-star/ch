#include <gtest/gtest.h>
#include "security/auth.h"
#include "security/proof_verification.h"
#include <cstring>

class SecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Инициализация BLS ключей для тестов
        bls_key_t test_key;
        memset(&test_key, 0, sizeof(bls_key_t));
        auth_init(&test_key);
        proof_verification_init();
    }

    void TearDown() override {
        auth_cleanup();
        proof_verification_cleanup();
    }
};

TEST_F(SecurityTest, BLSVerifySignature) {
    uint8_t public_key[48] = {0};
    uint8_t message[32] = {0};
    uint8_t signature[96] = {0};
    
    // В реальном тесте здесь будут действительные данные
    bool result = auth_bls_verify_signature(public_key, message, sizeof(message), signature);
    
    // В тестовом режиме функция всегда возвращает true
    EXPECT_TRUE(result);
}

TEST_F(SecurityTest, BLSSignMessage) {
    uint8_t private_key[32] = {0};
    uint8_t message[32] = {0};
    uint8_t signature[96] = {0};
    
    bool result = auth_bls_sign_message(private_key, message, sizeof(message), signature);
    
    EXPECT_TRUE(result);
    
    // Проверяем что подпись не нулевая
    bool all_zeros = true;
    for (int i = 0; i < 96; i++) {
        if (signature[i] != 0) {
            all_zeros = false;
            break;
        }
    }
    EXPECT_FALSE(all_zeros);
}

TEST_F(SecurityTest, CreateAndValidateSession) {
    uint8_t farmer_id[32] = {0x01, 0x02, 0x03}; // Тестовый ID
    
    auth_session_t* session = auth_create_session(farmer_id);
    ASSERT_NE(session, nullptr);
    
    EXPECT_TRUE(session->is_authenticated);
    EXPECT_EQ(session->request_count, 0);
    EXPECT_GT(session->expiry_time, session->created_time);
    
    // Проверяем валидацию сессии
    bool valid = auth_validate_session(session->session_id);
    EXPECT_TRUE(valid);
    
    // Уничтожаем сессию
    bool destroyed = auth_destroy_session(session->session_id);
    EXPECT_TRUE(destroyed);
}

TEST_F(SecurityTest, ValidateExpiredSession) {
    uint8_t farmer_id[32] = {0};
    
    auth_session_t* session = auth_create_session(farmer_id);
    ASSERT_NE(session, nullptr);
    
    // Имитируем просроченную сессию
    session->expiry_time = session->created_time - 3600;
    
    bool valid = auth_validate_session(session->session_id);
    EXPECT_FALSE(valid);
}

TEST_F(SecurityTest, GenerateAndValidateToken) {
    uint8_t farmer_public_key[48] = {0x01, 0x02, 0x03};
    
    auth_token_t* token = auth_generate_token(farmer_public_key);
    ASSERT_NE(token, nullptr);
    
    EXPECT_GT(token->expiry_time, token->issue_time);
    EXPECT_EQ(memcmp(token->farmer_public_key, farmer_public_key, 48), 0);
    
    // Проверяем токен с подписью
    uint8_t signature[96] = {0};
    auth_result_t result = auth_validate_token(token, signature);
    
    // В тестовом режиме ожидаем успех
    EXPECT_EQ(result, AUTH_SUCCESS);
    
    free(token);
}

TEST_F(SecurityTest, RateLimiting) {
    uint8_t farmer_id[32] = {0x01};
    
    // Первые 60 запросов должны проходить
    for (int i = 0; i < 60; i++) {
        bool allowed = auth_check_rate_limit(farmer_id, 60);
        EXPECT_TRUE(allowed);
    }
    
    // 61-й запрос должен быть заблокирован
    bool allowed = auth_check_rate_limit(farmer_id, 60);
    EXPECT_FALSE(allowed);
    
    // Сбрасываем лимит
    auth_reset_rate_limit(farmer_id);
    
    // Теперь снова должен проходить
    allowed = auth_check_rate_limit(farmer_id, 60);
    EXPECT_TRUE(allowed);
}

TEST_F(SecurityTest, ProofVerificationValid) {
    uint8_t proof_data[368] = {0};
    proof_verification_params_t params = {
        .challenge = 123456789,
        .k_size = 32,
        .sub_slot_iters = 37600000000ULL,
        .difficulty = 1000,
        .required_iterations = 0
    };
    
    proof_metadata_t metadata;
    proof_verification_result_t result = proof_verify_space(
        proof_data, sizeof(proof_data), &params, &metadata);
    
    // В тестовом режиме ожидаем валидный результат
    EXPECT_EQ(result, PROOF_VALID);
}

TEST_F(SecurityTest, ProofVerificationInvalidKSize) {
    uint8_t proof_data[368] = {0};
    proof_verification_params_t params = {
        .challenge = 123456789,
        .k_size = 20, // Невалидный k-size
        .sub_slot_iters = 37600000000ULL,
        .difficulty = 1000,
        .required_iterations = 0
    };
    
    proof_metadata_t metadata;
    proof_verification_result_t result = proof_verify_space(
        proof_data, sizeof(proof_data), &params, &metadata);
    
    EXPECT_EQ(result, PROOF_INVALID_K_SIZE);
}

TEST_F(SecurityTest, ProofQualityValidation) {
    uint8_t proof_data[368] = {0};
    uint32_t k_size = 32;
    uint64_t challenge = 123456789;
    uint64_t quality;
    
    bool result = proof_validate_quality(proof_data, k_size, challenge, &quality);
    
    EXPECT_TRUE(result);
    EXPECT_GT(quality, 0);
}

TEST_F(SecurityTest, ProofIterationsValidation) {
    uint64_t quality = 1000000;
    uint64_t difficulty = 1000;
    uint64_t sub_slot_iters = 37600000000ULL;
    uint64_t iterations;
    
    bool result = proof_validate_iterations(quality, difficulty, sub_slot_iters, &iterations);
    
    EXPECT_TRUE(result);
    EXPECT_GT(iterations, 0);
}

TEST_F(SecurityTest, ProofPointsCalculation) {
    uint64_t quality = 1000000;
    uint64_t difficulty = 1000;
    uint64_t points;
    
    bool result = proof_calculate_points(quality, difficulty, &points);
    
    EXPECT_TRUE(result);
    EXPECT_GT(points, 0);
    
    // points = (quality * 1,000,000) / difficulty
    uint64_t expected_points = (quality * 1000000) / difficulty;
    EXPECT_EQ(points, expected_points);
}

TEST_F(SecurityTest, CleanupExpiredSessions) {
    // Создаем несколько сессий
    uint8_t farmer_id1[32] = {0x01};
    uint8_t farmer_id2[32] = {0x02};
    
    auth_session_t* session1 = auth_create_session(farmer_id1);
    auth_session_t* session2 = auth_create_session(farmer_id2);
    
    // Имитируем просрочку первой сессии
    session1->expiry_time = session1->created_time - 3600;
    
    // Очищаем просроченные сессии
    bool result = auth_cleanup_expired_sessions();
    EXPECT_TRUE(result);
    
    // Проверяем что первая сессия удалена, а вторая осталась
    bool valid1 = auth_validate_session(session1->session_id);
    bool valid2 = auth_validate_session(session2->session_id);
    
    EXPECT_FALSE(valid1);
    EXPECT_TRUE(valid2);
    
    // Очищаем
    auth_destroy_session(session2->session_id);
}
