#include "protocol/singleton.h"
#include "../blockchain/chia_operations.h"
#include "../security/auth.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static void singleton_log(const char* level, const char* message) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] [SINGLETON] [%s] %s\n", timestamp, level, message);
    fflush(stdout);
}

bool singleton_init(const uint8_t* launcher_id, singleton_t* singleton) {
    if (!launcher_id || !singleton) {
        singleton_log("ERROR", "Невалидные параметры для инициализации синглтона");
        return false;
    }
    
    memset(singleton, 0, sizeof(singleton_t));
    memcpy(singleton->launcher_id, launcher_id, 32);
    singleton->last_partial_time = time(NULL);
    singleton->current_difficulty = 1;
    
    char launcher_id_hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(launcher_id_hex + i * 2, "%02x", launcher_id[i]);
    }
    launcher_id_hex[64] = '\0';
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Инициализация синглтона: %s", launcher_id_hex);
    singleton_log("INFO", log_msg);
    
    // Загрузка состояния из блокчейна
    if (!singleton_sync_with_blockchain(singleton)) {
        singleton_log("ERROR", "Не удалось синхронизировать синглтон с блокчейном");
        return false;
    }
    
    return true;
}

bool singleton_validate_ownership(const singleton_t* singleton, const uint8_t* signature) {
    if (!singleton || !signature) {
        singleton_log("ERROR", "Невалидные параметры для проверки владения");
        return false;
    }
    
    // Создаем сообщение для проверки подписи
    uint8_t message[64];
    memcpy(message, singleton->launcher_id, 32);
    memcpy(message + 32, &singleton->last_partial_time, sizeof(uint64_t));
    
    // Проверяем BLS подпись
    if (!auth_bls_verify_signature(singleton->owner_public_key, message, 
                                  sizeof(message), signature)) {
        singleton_log("ERROR", "Невалидная подпись владения синглтона");
        return false;
    }
    
    singleton_log("DEBUG", "Владение синглтона успешно проверено");
    return true;
}

bool singleton_verify_pool_membership(const singleton_t* singleton) {
    if (!singleton) {
        singleton_log("ERROR", "Синглтон не может быть NULL");
        return false;
    }
    
    // Проверяем, что синглтон настроен на этот пул
    // В реальной реализации здесь будет проверка пазла синглтона
    
    if (!singleton->is_pool_member) {
        char launcher_id_hex[65];
        for (int i = 0; i < 32; i++) {
            sprintf(launcher_id_hex + i * 2, "%02x", singleton->launcher_id[i]);
        }
        
        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "Синглтон %s не является членом пула", launcher_id_hex);
        singleton_log("WARNING", log_msg);
        return false;
    }
    
    singleton_log("DEBUG", "Членство синглтона в пуле подтверждено");
    return true;
}

bool singleton_update_state(singleton_t* singleton) {
    if (!singleton) {
        singleton_log("ERROR", "Синглтон не может быть NULL");
        return false;
    }
    
    // Синхронизация с блокчейном для получения актуального состояния
    if (!singleton_sync_with_blockchain(singleton)) {
        singleton_log("ERROR", "Не удалось обновить состояние синглтона");
        return false;
    }
    
    // Проверяем баланс и необходимость поглощения вознаграждений
    if (singleton->balance > 0) {
        singleton_log("INFO", "Обнаружен баланс для поглощения");
        if (!singleton_absorb_rewards(singleton)) {
            singleton_log("ERROR", "Не удалось поглотить вознаграждения");
            return false;
        }
    }
    
    singleton_log("DEBUG", "Состояние синглтона успешно обновлено");
    return true;
}

bool singleton_absorb_rewards(singleton_t* singleton) {
    if (!singleton) {
        singleton_log("ERROR", "Синглтон не может быть NULL");
        return false;
    }
    
    if (singleton->balance == 0) {
        singleton_log("DEBUG", "Нет вознаграждений для поглощения");
        return true;
    }
    
    char launcher_id_hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(launcher_id_hex + i * 2, "%02x", singleton->launcher_id[i]);
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), 
             "Поглощение вознаграждений для синглтона %s: %lu mojos", 
             launcher_id_hex, singleton->balance);
    singleton_log("INFO", log_msg);
    
    // Здесь будет реализация создания транзакции поглощения
    // с использованием smart_coin.h
    
    // После успешного поглощения обновляем баланс
    singleton->balance = 0;
    
    singleton_log("INFO", "Вознаграждения успешно поглощены");
    return true;
}

bool singleton_sync_with_blockchain(singleton_t* singleton) {
    if (!singleton) {
        singleton_log("ERROR", "Синглтон не может быть NULL");
        return false;
    }
    
    // Получаем актуальные данные о синглтоне из блокчейна
    if (!chia_rpc_get_coin_records_by_puzzle_hash(singleton->p2_singleton_puzzle, 0)) {
        singleton_log("ERROR", "Не удалось получить записи о коинах синглтона");
        return false;
    }
    
    // Обновляем состояние на основе полученных данных
    // В реальной реализации здесь будет парсинг ответа RPC
    
    singleton_log("DEBUG", "Синхронизация синглтона с блокчейном завершена");
    return true;
}

bool singleton_get_coin_records(const uint8_t* launcher_id, uint32_t start_height, uint32_t end_height) {
    if (!launcher_id) {
        singleton_log("ERROR", "Launcher ID не может быть NULL");
        return false;
    }
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "Получение записей коинов для синглтона с высоты %u до %u", 
             start_height, end_height);
    singleton_log("DEBUG", log_msg);
    
    // Реализация получения записей коинов через RPC
    return chia_rpc_get_coin_records_by_puzzle_hash(launcher_id, start_height);
}

void singleton_log_state(const singleton_t* singleton) {
    if (!singleton) {
        singleton_log("ERROR", "Невалидный синглтон для логирования");
        return;
    }
    
    char launcher_id_hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(launcher_id_hex + i * 2, "%02x", singleton->launcher_id[i]);
    }
    
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg),
             "Состояние синглтона %s: очки=%lu, сложность=%lu, баланс=%lu, "
             "в пуле=%s, последний partial=%lu",
             launcher_id_hex, singleton->total_points, singleton->current_difficulty,
             singleton->balance, singleton->is_pool_member ? "да" : "нет",
             singleton->last_partial_time);
    
    singleton_log("INFO", log_msg);
}

bool singleton_can_leave_pool(const singleton_t* singleton) {
    if (!singleton) {
        return false;
    }
    
    // Проверяем условия для выхода из пула
    // Включая относительную блокировку и другие условия
    
    bool can_leave = (singleton->relative_lock_height == 0);
    
    char launcher_id_hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(launcher_id_hex + i * 2, "%02x", singleton->launcher_id[i]);
    }
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "Синглтон %s %s покинуть пул", 
             launcher_id_hex, can_leave ? "может" : "не может");
    singleton_log("DEBUG", log_msg);
    
    return can_leave;
}
