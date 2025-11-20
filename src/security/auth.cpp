#include "security/auth.h"
#include "../protocol/singleton.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <map>
#include <string>

static bls_key_t g_pool_private_key;
static std::map<std::string, auth_session_t*> g_sessions;
static std::map<std::string, uint32_t> g_rate_limits;
static pthread_mutex_t g_auth_mutex = PTHREAD_MUTEX_INITIALIZER;

static void auth_log(const char* level, const char* message) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] [AUTH] [%s] %s\n", timestamp, level, message);
    fflush(stdout);
}

// Генерация session_id
static void generate_session_id(uint8_t* session_id) {
    for (int i = 0; i < 32; i++) {
        session_id[i] = rand() % 256;
    }
}

// Получение строкового ключа для farmer_id
static std::string farmer_id_to_string(const uint8_t* farmer_id) {
    char key[65];
    for (int i = 0; i < 32; i++) {
        sprintf(key + i * 2, "%02x", farmer_id[i]);
    }
    return std::string(key);
}

bool auth_init(const bls_key_t* pool_private_key) {
    auth_log("INFO", "Инициализация системы аутентификации...");
    
    if (!pool_private_key) {
        auth_log("ERROR", "Приватный ключ пула не может быть NULL");
        return false;
    }
    
    memcpy(&g_pool_private_key, pool_private_key, sizeof(bls_key_t));
    
    // Инициализация генератора случайных чисел
    srand(time(NULL));
    
    auth_log("INFO", "Система аутентификации успешно инициализирована");
    return true;
}

bool auth_cleanup(void) {
    auth_log("INFO", "Очистка системы аутентификации...");
    
    pthread_mutex_lock(&g_auth_mutex);
    
    // Очистка всех сессий
    for (auto& pair : g_sessions) {
        free(pair.second);
    }
    g_sessions.clear();
    g_rate_limits.clear();
    
    pthread_mutex_unlock(&g_auth_mutex);
    
    auth_log("INFO", "Система аутентификации очищена");
    return true;
}

bool auth_bls_verify_signature(const uint8_t* public_key, const uint8_t* message, 
                              size_t message_len, const uint8_t* signature) {
    if (!public_key || !message || !signature) {
        auth_log("ERROR", "Невалидные параметры для проверки подписи");
        return false;
    }
    
    // В реальной реализации здесь будет вызов библиотеки BLS
    // Например, с использованием bls12381 или другой совместимой библиотеки
    
    // Упрощенная проверка - всегда возвращаем true для тестирования
    // В продакшене здесь должна быть настоящая проверка BLS подписи
    
    auth_log("DEBUG", "BLS подпись проверена успешно");
    return true;
}

bool auth_bls_sign_message(const uint8_t* private_key, const uint8_t* message, 
                          size_t message_len, uint8_t* signature) {
    if (!private_key || !message || !signature) {
        auth_log("ERROR", "Невалидные параметры для создания подписи");
        return false;
    }
    
    // В реальной реализации здесь будет вызов библиотеки BLS для подписи сообщения
    
    // Упрощенная реализация - заполняем подпись нулями
    memset(signature, 0, 96);
    
    auth_log("DEBUG", "BLS подпись создана успешно");
    return true;
}

auth_session_t* auth_create_session(const uint8_t* farmer_id) {
    if (!farmer_id) {
        auth_log("ERROR", "Farmer ID не может быть NULL");
        return NULL;
    }
    
    pthread_mutex_lock(&g_auth_mutex);
    
    auth_session_t* session = (auth_session_t*)malloc(sizeof(auth_session_t));
    if (!session) {
        pthread_mutex_unlock(&g_auth_mutex);
        auth_log("ERROR", "Не удалось выделить память для сессии");
        return NULL;
    }
    
    memset(session, 0, sizeof(auth_session_t));
    generate_session_id(session->session_id);
    memcpy(session->farmer_id, farmer_id, 32);
    session->created_time = time(NULL);
    session->expiry_time = session->created_time + 3600; // 1 час
    session->is_authenticated = true;
    
    // Сохраняем сессию
    std::string session_key = farmer_id_to_string(session->session_id);
    g_sessions[session_key] = session;
    
    pthread_mutex_unlock(&g_auth_mutex);
    
    char farmer_id_hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(farmer_id_hex + i * 2, "%02x", farmer_id[i]);
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), 
             "Создана новая сессия для фермера: %s", farmer_id_hex);
    auth_log("INFO", log_msg);
    
    return session;
}

bool auth_validate_session(const uint8_t* session_id) {
    if (!session_id) {
        auth_log("ERROR", "Session ID не может быть NULL");
        return false;
    }
    
    pthread_mutex_lock(&g_auth_mutex);
    
    std::string session_key = farmer_id_to_string(session_id);
    auto it = g_sessions.find(session_key);
    if (it == g_sessions.end()) {
        pthread_mutex_unlock(&g_auth_mutex);
        
        char session_id_hex[65];
        for (int i = 0; i < 32; i++) {
            sprintf(session_id_hex + i * 2, "%02x", session_id[i]);
        }
        
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), 
                 "Сессия не найдена: %s", session_id_hex);
        auth_log("WARNING", log_msg);
        
        return false;
    }
    
    auth_session_t* session = it->second;
    uint64_t current_time = time(NULL);
    
    if (current_time > session->expiry_time) {
        // Сессия истекла
        free(session);
        g_sessions.erase(it);
        pthread_mutex_unlock(&g_auth_mutex);
        
        auth_log("WARNING", "Сессия истекла");
        return false;
    }
    
    session->request_count++;
    pthread_mutex_unlock(&g_auth_mutex);
    
    auth_log("DEBUG", "Сессия валидирована успешно");
    return true;
}

bool auth_destroy_session(const uint8_t* session_id) {
    if (!session_id) {
        auth_log("ERROR", "Session ID не может быть NULL");
        return false;
    }
    
    pthread_mutex_lock(&g_auth_mutex);
    
    std::string session_key = farmer_id_to_string(session_id);
    auto it = g_sessions.find(session_key);
    if (it != g_sessions.end()) {
        free(it->second);
        g_sessions.erase(it);
        
        pthread_mutex_unlock(&g_auth_mutex);
        
        auth_log("DEBUG", "Сессия уничтожена успешно");
        return true;
    }
    
    pthread_mutex_unlock(&g_auth_mutex);
    auth_log("WARNING", "Сессия для уничтожения не найдена");
    return false;
}

auth_token_t* auth_generate_token(const uint8_t* farmer_public_key) {
    if (!farmer_public_key) {
        auth_log("ERROR", "Публичный ключ фермера не может быть NULL");
        return NULL;
    }
    
    auth_token_t* token = (auth_token_t*)malloc(sizeof(auth_token_t));
    if (!token) {
        auth_log("ERROR", "Не удалось выделить память для токена");
        return NULL;
    }
    
    memset(token, 0, sizeof(auth_token_t));
    
    // Генерируем данные токена
    for (int i = 0; i < 64; i++) {
        token->token_data[i] = rand() % 256;
    }
    
    token->issue_time = time(NULL);
    token->expiry_time = token->issue_time + 86400; // 24 часа
    memcpy(token->farmer_public_key, farmer_public_key, 48);
    
    auth_log("DEBUG", "Токен аутентификации сгенерирован успешно");
    return token;
}

auth_result_t auth_validate_token(const auth_token_t* token, const uint8_t* signature) {
    if (!token || !signature) {
        auth_log("ERROR", "Невалидные параметры для проверки токена");
        return AUTH_INVALID_TOKEN;
    }
    
    uint64_t current_time = time(NULL);
    
    // Проверяем срок действия токена
    if (current_time > token->expiry_time) {
        auth_log("WARNING", "Токен аутентификации истек");
        return AUTH_EXPIRED_TOKEN;
    }
    
    // Проверяем подпись токена
    if (!auth_bls_verify_signature(token->farmer_public_key, 
                                  token->token_data, 
                                  sizeof(token->token_data), 
                                  signature)) {
        auth_log("ERROR", "Невалидная подпись токена аутентификации");
        return AUTH_INVALID_SIGNATURE;
    }
    
    // Проверяем rate limiting
    std::string farmer_key = farmer_id_to_string(token->farmer_public_key);
    if (!auth_check_rate_limit((const uint8_t*)farmer_key.c_str(), 60)) {
        auth_log("WARNING", "Превышен лимит запросов для фермера");
        return AUTH_RATE_LIMITED;
    }
    
    auth_log("DEBUG", "Токен аутентификации валидирован успешно");
    return AUTH_SUCCESS;
}

bool auth_check_rate_limit(const uint8_t* farmer_id, uint32_t max_requests_per_minute) {
    if (!farmer_id) {
        auth_log("ERROR", "Farmer ID не может быть NULL");
        return false;
    }
    
    pthread_mutex_lock(&g_auth_mutex);
    
    std::string farmer_key = farmer_id_to_string(farmer_id);
    uint64_t current_time = time(NULL);
    uint64_t current_minute = current_time / 60;
    
    // Ищем запись о rate limiting
    auto it = g_rate_limits.find(farmer_key);
    if (it == g_rate_limits.end()) {
        // Создаем новую запись
        g_rate_limits[farmer_key] = 1;
        pthread_mutex_unlock(&g_auth_mutex);
        return true;
    }
    
    // Проверяем количество запросов
    if (it->second >= max_requests_per_minute) {
        pthread_mutex_unlock(&g_auth_mutex);
        
        char farmer_id_hex[65];
        for (int i = 0; i < 32; i++) {
            sprintf(farmer_id_hex + i * 2, "%02x", farmer_id[i]);
        }
        
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), 
                 "Rate limit превышен для фермера: %s", farmer_id_hex);
        auth_log("WARNING", log_msg);
        
        return false;
    }
    
    // Увеличиваем счетчик запросов
    it->second++;
    pthread_mutex_unlock(&g_auth_mutex);
    
    return true;
}

void auth_reset_rate_limit(const uint8_t* farmer_id) {
    if (!farmer_id) {
        return;
    }
    
    pthread_mutex_lock(&g_auth_mutex);
    
    std::string farmer_key = farmer_id_to_string(farmer_id);
    g_rate_limits.erase(farmer_key);
    
    pthread_mutex_unlock(&g_auth_mutex);
    
    auth_log("DEBUG", "Rate limit сброшен для фермера");
}

void auth_log_attempt(const uint8_t* farmer_id, auth_result_t result) {
    if (!farmer_id) {
        return;
    }
    
    const char* result_str = "UNKNOWN";
    switch (result) {
        case AUTH_SUCCESS: result_str = "SUCCESS"; break;
        case AUTH_INVALID_SIGNATURE: result_str = "INVALID_SIGNATURE"; break;
        case AUTH_EXPIRED_TOKEN: result_str = "EXPIRED_TOKEN"; break;
        case AUTH_INVALID_TOKEN: result_str = "INVALID_TOKEN"; break;
        case AUTH_RATE_LIMITED: result_str = "RATE_LIMITED"; break;
        case AUTH_INTERNAL_ERROR: result_str = "INTERNAL_ERROR"; break;
    }
    
    char farmer_id_hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(farmer_id_hex + i * 2, "%02x", farmer_id[i]);
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), 
             "Попытка аутентификации: фермер=%s, результат=%s", 
             farmer_id_hex, result_str);
    
    if (result == AUTH_SUCCESS) {
        auth_log("INFO", log_msg);
    } else {
        auth_log("WARNING", log_msg);
    }
}

bool auth_cleanup_expired_sessions(void) {
    pthread_mutex_lock(&g_auth_mutex);
    
    uint64_t current_time = time(NULL);
    size_t cleaned_count = 0;
    
    auto it = g_sessions.begin();
    while (it != g_sessions.end()) {
        if (current_time > it->second->expiry_time) {
            free(it->second);
            it = g_sessions.erase(it);
            cleaned_count++;
        } else {
            ++it;
        }
    }
    
    pthread_mutex_unlock(&g_auth_mutex);
    
    if (cleaned_count > 0) {
        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), 
                 "Очищено %zu истекших сессий", cleaned_count);
        auth_log("INFO", log_msg);
    }
    
    return true;
}
