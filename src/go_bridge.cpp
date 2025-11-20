#include "go_bridge.h"
#include "pool_core.h"
#include "protocol/partials.h"
#include "protocol/singleton.h"
#include "security/auth.h"
#include "math_operations.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Callback функции из Go
static GoLogCallback g_log_callback = NULL;
static GoPartialCallback g_partial_callback = NULL;
static GoPayoutCallback g_payout_callback = NULL;

static void go_bridge_log(const char* level, const char* message) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char log_message[1024];
    snprintf(log_message, sizeof(log_message), "[%s] [GO_BRIDGE] [%s] %s", 
             timestamp, level, message);
    
    printf("%s\n", log_message);
    fflush(stdout);
    
    // Вызываем callback в Go если зарегистрирован
    if (g_log_callback) {
        g_log_callback(log_message, 0); // level передается как int
    }
}

bool go_bridge_init(void) {
    go_bridge_log("INFO", "Инициализация Go бриджа...");
    
    // Инициализация любых необходимых ресурсов для работы с Go
    
    go_bridge_log("INFO", "Go бридж успешно инициализирован");
    return true;
}

bool go_bridge_cleanup(void) {
    go_bridge_log("INFO", "Очистка Go бриджа...");
    
    // Сброс callback функций
    g_log_callback = NULL;
    g_partial_callback = NULL;
    g_payout_callback = NULL;
    
    go_bridge_log("INFO", "Go бридж очищен");
    return true;
}

bool go_bridge_add_farmer(const FarmerInfo* farmer) {
    if (!farmer) {
        go_bridge_log("ERROR", "FarmerInfo не может быть NULL");
        return false;
    }
    
    go_bridge_log("INFO", "Добавление фермера через Go бридж...");
    
    // Конвертируем hex строки в бинарные данные
    uint8_t launcher_id[32];
    if (strlen(farmer->launcher_id) != 64) {
        go_bridge_log("ERROR", "Невалидная длина launcher_id");
        return false;
    }
    
    for (int i = 0; i < 32; i++) {
        sscanf(farmer->launcher_id + i * 2, "%02hhx", &launcher_id[i]);
    }
    
    // Создаем синглтон для фермера
    singleton_t farmer_singleton;
    if (!singleton_init(launcher_id, &farmer_singleton)) {
        go_bridge_log("ERROR", "Не удалось инициализировать синглтон фермера");
        return false;
    }
    
    // Обновляем статистику фермера
    farmer_singleton.total_points = farmer->points;
    farmer_singleton.current_difficulty = farmer->difficulty;
    
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg),
             "Фермер добавлен: launcher=%s, points=%lu, difficulty=%lu, partials=%lu",
             farmer->launcher_id, farmer->points, farmer->difficulty, farmer->partials);
    go_bridge_log("INFO", log_msg);
    
    return true;
}

bool go_bridge_update_farmer(const FarmerInfo* farmer) {
    if (!farmer) {
        go_bridge_log("ERROR", "FarmerInfo не может быть NULL");
        return false;
    }
    
    go_bridge_log("DEBUG", "Обновление информации о фермере через Go бридж...");
    
    // Аналогично add_farmer, но обновляем существующую запись
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Информация о фермере обновлена: %s", farmer->launcher_id);
    go_bridge_log("DEBUG", log_msg);
    
    return true;
}

bool go_bridge_remove_farmer(const char* launcher_id) {
    if (!launcher_id) {
        go_bridge_log("ERROR", "Launcher ID не может быть NULL");
        return false;
    }
    
    go_bridge_log("INFO", "Удаление фермера через Go бридж...");
    
    // В реальной реализации здесь будет удаление фермера из всех структур данных
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Фермер удален: %s", launcher_id);
    go_bridge_log("INFO", log_msg);
    
    return true;
}

bool go_bridge_process_partial(const PartialRequest* partial) {
    if (!partial) {
        go_bridge_log("ERROR", "PartialRequest не может быть NULL");
        return false;
    }
    
    go_bridge_log("DEBUG", "Обработка partial решения через Go бридж...");
    
    // Конвертируем hex строки в бинарные данные
    uint8_t challenge[32];
    uint8_t launcher_id[32];
    uint8_t signature[96];
    
    if (strlen(partial->challenge) != 64) {
        go_bridge_log("ERROR", "Невалидная длина challenge");
        return false;
    }
    if (strlen(partial->launcher_id) != 64) {
        go_bridge_log("ERROR", "Невалидная длина launcher_id");
        return false;
    }
    if (strlen(partial->signature) != 192) {
        go_bridge_log("ERROR", "Невалидная длина signature");
        return false;
    }
    
    for (int i = 0; i < 32; i++) {
        sscanf(partial->challenge + i * 2, "%02hhx", &challenge[i]);
        sscanf(partial->launcher_id + i * 2, "%02hhx", &launcher_id[i]);
    }
    for (int i = 0; i < 96; i++) {
        sscanf(partial->signature + i * 2, "%02hhx", &signature[i]);
    }
    
    // Создаем структуру partial для обработки
    partial_t partial_data;
    memset(&partial_data, 0, sizeof(partial_t));
    
    memcpy(partial_data.challenge, challenge, 32);
    memcpy(partial_data.launcher_id, launcher_id, 32);
    memcpy(partial_data.signature, signature, 96);
    partial_data.timestamp = partial->timestamp;
    partial_data.difficulty = partial->difficulty;
    
    // Обрабатываем partial решение
    if (!partial_process(&partial_data)) {
        go_bridge_log("ERROR", "Не удалось обработать partial решение");
        return false;
    }
    
    // Вызываем callback в Go если зарегистрирован
    if (g_partial_callback) {
        g_partial_callback(partial);
    }
    
    go_bridge_log("INFO", "Partial решение успешно обработано через Go бридж");
    return true;
}

bool go_bridge_validate_partial(const PartialRequest* partial) {
    if (!partial) {
        go_bridge_log("ERROR", "PartialRequest не может быть NULL");
        return false;
    }
    
    go_bridge_log("DEBUG", "Валидация partial решения через Go бридж...");
    
    // Конвертируем hex строки в бинарные данные (аналогично process_partial)
    uint8_t challenge[32];
    uint8_t launcher_id[32];
    uint8_t signature[96];
    
    for (int i = 0; i < 32; i++) {
        sscanf(partial->challenge + i * 2, "%02hhx", &challenge[i]);
        sscanf(partial->launcher_id + i * 2, "%02hhx", &launcher_id[i]);
    }
    for (int i = 0; i < 96; i++) {
        sscanf(partial->signature + i * 2, "%02hhx", &signature[i]);
    }
    
    // Создаем временную структуру partial для валидации
    partial_t partial_data;
    memset(&partial_data, 0, sizeof(partial_t));
    
    memcpy(partial_data.challenge, challenge, 32);
    memcpy(partial_data.launcher_id, launcher_id, 32);
    memcpy(partial_data.signature, signature, 96);
    partial_data.timestamp = partial->timestamp;
    
    // Валидируем partial решение
    partial_validation_result_t result = partial_validate(&partial_data);
    bool valid = (result == PARTIAL_VALID);
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Валидация partial: launcher=%s, результат=%s",
             partial->launcher_id, valid ? "VALID" : "INVALID");
    
    if (valid) {
        go_bridge_log("INFO", log_msg);
    } else {
        go_bridge_log("WARNING", log_msg);
    }
    
    return valid;
}

bool go_bridge_get_pool_info(PoolInfo* pool_info) {
    if (!pool_info) {
        go_bridge_log("ERROR", "PoolInfo не может быть NULL");
        return false;
    }
    
    go_bridge_log("DEBUG", "Получение информации о пуле через Go бридж...");
    
    // Получаем контекст пула
    pool_context_t* context = pool_get_context();
    if (!context) {
        go_bridge_log("ERROR", "Не удалось получить контекст пула");
        return false;
    }
    
    // Заполняем структуру информацией о пуле
    strncpy(pool_info->pool_name, context->config.pool_name, sizeof(pool_info->pool_name) - 1);
    strncpy(pool_info->pool_url, context->config.pool_url, sizeof(pool_info->pool_url) - 1);
    
    pool_info->total_farmers = context->stats.total_farmers;
    pool_info->total_netspace = context->stats.total_netspace;
    pool_info->current_difficulty = context->stats.current_difficulty;
    pool_info->pool_fee = context->config.pool_fee;
    pool_info->min_payout = context->config.min_payout;
    
    go_bridge_log("DEBUG", "Информация о пуле успешно получена");
    return true;
}

bool go_bridge_process_payout(const char* launcher_id, uint64_t amount) {
    if (!launcher_id) {
        go_bridge_log("ERROR", "Launcher ID не может быть NULL");
        return false;
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Обработка выплаты через Go бридж: launcher=%s, amount=%lu",
             launcher_id, amount);
    go_bridge_log("INFO", log_msg);
    
    // В реальной реализации здесь будет создание и отправка транзакции выплаты
    
    // Вызываем callback в Go если зарегистрирован
    if (g_payout_callback) {
        g_payout_callback(launcher_id, amount);
    }
    
    return true;
}

bool go_bridge_calculate_payouts(void) {
    go_bridge_log("INFO", "Расчет выплат через Go бридж...");
    
    // В реальной реализации здесь будет расчет всех выплат
    // на основе накопленных очков фермеров
    
    go_bridge_log("INFO", "Расчет выплат завершен");
    return true;
}

bool go_bridge_get_statistics(uint64_t* total_farmers, uint64_t* total_partials,
                             uint64_t* valid_partials, uint64_t* total_points) {
    if (!total_farmers || !total_partials || !valid_partials || !total_points) {
        go_bridge_log("ERROR", "Выходные параметры не могут быть NULL");
        return false;
    }
    
    go_bridge_log("DEBUG", "Получение статистики через Go бридж...");
    
    // Получаем статистику пула
    pool_context_t* context = pool_get_context();
    if (!context) {
        go_bridge_log("ERROR", "Не удалось получить контекст пула");
        return false;
    }
    
    *total_farmers = context->stats.total_farmers;
    *total_partials = context->stats.total_partials;
    *valid_partials = context->stats.valid_partials;
    *total_points = context->stats.total_points;
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg),
             "Статистика получена: farmers=%lu, partials=%lu, valid=%lu, points=%lu",
             *total_farmers, *total_partials, *valid_partials, *total_points);
    go_bridge_log("DEBUG", log_msg);
    
    return true;
}

void go_bridge_log_debug(const char* message) {
    go_bridge_log("DEBUG", message);
}

void go_bridge_log_info(const char* message) {
    go_bridge_log("INFO", message);
}

void go_bridge_log_error(const char* message) {
    go_bridge_log("ERROR", message);
}

void go_bridge_log_fatal(const char* message) {
    go_bridge_log("FATAL", message);
}

bool go_bridge_register_log_callback(GoLogCallback callback) {
    g_log_callback = callback;
    
    go_bridge_log("INFO", "Callback для логирования зарегистрирован");
    return true;
}

bool go_bridge_register_partial_callback(GoPartialCallback callback) {
    g_partial_callback = callback;
    
    go_bridge_log("INFO", "Callback для partial решений зарегистрирован");
    return true;
}

bool go_bridge_register_payout_callback(GoPayoutCallback callback) {
    g_payout_callback = callback;
    
    go_bridge_log("INFO", "Callback для выплат зарегистрирован");
    return true;
}
