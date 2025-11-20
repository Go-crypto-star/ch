#ifndef AUTH_H
#define AUTH_H

#include <cstddef>
#include <stdint.h>
#include <stdbool.h>

// BLS ключи
typedef struct {
    uint8_t private_key[32];
    uint8_t public_key[48];
    uint8_t chain_code[32];
} bls_key_t;

// Сессия аутентификации
typedef struct {
    uint8_t session_id[32];
    uint8_t farmer_id[32];
    uint64_t created_time;
    uint64_t expiry_time;
    uint32_t request_count;
    bool is_authenticated;
} auth_session_t;

// Токен аутентификации
typedef struct {
    uint8_t token_data[64];
    uint64_t issue_time;
    uint64_t expiry_time;
    uint8_t farmer_public_key[48];
} auth_token_t;

// Результат аутентификации
typedef enum {
    AUTH_SUCCESS,
    AUTH_INVALID_SIGNATURE,
    AUTH_EXPIRED_TOKEN,
    AUTH_INVALID_TOKEN,
    AUTH_RATE_LIMITED,
    AUTH_INTERNAL_ERROR
} auth_result_t;

// Инициализация системы аутентификации
bool auth_init(const bls_key_t* pool_private_key);
bool auth_cleanup(void);

// BLS операции
bool auth_bls_verify_signature(const uint8_t* public_key, const uint8_t* message, 
                              size_t message_len, const uint8_t* signature);
bool auth_bls_sign_message(const uint8_t* private_key, const uint8_t* message, 
                          size_t message_len, uint8_t* signature);

// Управление сессиями
auth_session_t* auth_create_session(const uint8_t* farmer_id);
bool auth_validate_session(const uint8_t* session_id);
bool auth_destroy_session(const uint8_t* session_id);

// Работа с токенами
auth_token_t* auth_generate_token(const uint8_t* farmer_public_key);
auth_result_t auth_validate_token(const auth_token_t* token, const uint8_t* signature);

// Rate limiting
bool auth_check_rate_limit(const uint8_t* farmer_id, uint32_t max_requests_per_minute);
void auth_reset_rate_limit(const uint8_t* farmer_id);

// Утилиты
void auth_log_attempt(const uint8_t* farmer_id, auth_result_t result);
bool auth_cleanup_expired_sessions(void);

#endif // AUTH_H
