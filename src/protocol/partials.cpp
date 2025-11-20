#include "protocol/partials.h"
#include "../security/proof_verification.h"
#include "../security/auth.h"
#include "../blockchain/chia_operations.h"
#include "singleton.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static uint64_t g_valid_partials = 0;
static uint64_t g_invalid_partials = 0;
static uint64_t g_total_partials = 0;

static void partials_log(const char* level, const char* message) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] [PARTIALS] [%s] %s\n", timestamp, level, message);
    fflush(stdout);
}

bool partial_queue_init(partial_queue_t* queue, uint32_t max_size) {
    if (!queue) {
        partials_log("ERROR", "Очередь не может быть NULL");
        return false;
    }
    
    memset(queue, 0, sizeof(partial_queue_t));
    queue->max_size = max_size;
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        partials_log("ERROR", "Не удалось инициализировать мьютекс очереди");
        return false;
    }
    
    if (pthread_cond_init(&queue->cond, NULL) != 0) {
        partials_log("ERROR", "Не удалось инициализировать условную переменную");
        pthread_mutex_destroy(&queue->mutex);
        return false;
    }
    
    partials_log("INFO", "Очередь partial решений инициализирована");
    return true;
}

bool partial_queue_push(partial_queue_t* queue, const partial_t* partial) {
    if (!queue || !partial) {
        partials_log("ERROR", "Невалидные параметры для добавления в очередь");
        return false;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->size >= queue->max_size) {
        pthread_mutex_unlock(&queue->mutex);
        partials_log("WARNING", "Очередь partial решений переполнена");
        return false;
    }
    
    partial_queue_node_t* new_node = (partial_queue_node_t*)malloc(sizeof(partial_queue_node_t));
    if (!new_node) {
        pthread_mutex_unlock(&queue->mutex);
        partials_log("ERROR", "Не удалось выделить память для узла очереди");
        return false;
    }
    
    memcpy(&new_node->partial, partial, sizeof(partial_t));
    new_node->next = NULL;
    
    if (queue->tail) {
        queue->tail->next = new_node;
    } else {
        queue->head = new_node;
    }
    queue->tail = new_node;
    queue->size++;
    
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    
    partials_log("DEBUG", "Partial решение добавлено в очередь");
    return true;
}

bool partial_queue_pop(partial_queue_t* queue, partial_t* partial) {
    if (!queue || !partial) {
        partials_log("ERROR", "Невалидные параметры для извлечения из очереди");
        return false;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->size == 0) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    
    if (!queue->head) {
        pthread_mutex_unlock(&queue->mutex);
        partials_log("ERROR", "Очередь пуста, но размер не нулевой");
        return false;
    }
    
    partial_queue_node_t* node = queue->head;
    memcpy(partial, &node->partial, sizeof(partial_t));
    
    queue->head = node->next;
    if (!queue->head) {
        queue->tail = NULL;
    }
    
    free(node);
    queue->size--;
    
    pthread_mutex_unlock(&queue->mutex);
    
    partials_log("DEBUG", "Partial решение извлечено из очереди");
    return true;
}

void partial_queue_cleanup(partial_queue_t* queue) {
    if (!queue) {
        return;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    partial_queue_node_t* current = queue->head;
    while (current) {
        partial_queue_node_t* next = current->next;
        free(current);
        current = next;
    }
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    
    pthread_mutex_unlock(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
    pthread_mutex_destroy(&queue->mutex);
    
    partials_log("INFO", "Очередь partial решений очищена");
}

partial_validation_result_t partial_validate(const partial_t* partial) {
    if (!partial) {
        partials_log("ERROR", "Partial решение не может быть NULL");
        return PARTIAL_INTERNAL_ERROR;
    }
    
    g_total_partials++;
    
    // Проверка временной метки
    uint64_t current_time = time(NULL);
    if (current_time - partial->timestamp > 28) { // 28 секунд дедлайн
        partials_log("WARNING", "Partial решение получено слишком поздно");
        g_invalid_partials++;
        return PARTIAL_TOO_LATE;
    }
    
    // Проверка синглтона
    singleton_t farmer_singleton;
    if (!singleton_init(partial->launcher_id, &farmer_singleton)) {
        partials_log("ERROR", "Не удалось инициализировать синглтон фермера");
        g_invalid_partials++;
        return PARTIAL_INVALID_SINGLETON;
    }
    
    if (!singleton_verify_pool_membership(&farmer_singleton)) {
        partials_log("WARNING", "Синглтон не является членом пула");
        g_invalid_partials++;
        return PARTIAL_INVALID_SINGLETON;
    }
    
    // Проверка подписи
    if (!partial_verify_signature(partial)) {
        partials_log("ERROR", "Невалидная подпись partial решения");
        g_invalid_partials++;
        return PARTIAL_INVALID_SIGNATURE;
    }
    
    // Проверка доказательства пространства
    if (!partial_verify_proof(partial)) {
        partials_log("ERROR", "Невалидное доказательство пространства");
        g_invalid_partials++;
        return PARTIAL_INVALID_PROOF;
    }
    
    // Проверка вызова (challenge)
    if (!partial_verify_challenge(partial->challenge)) {
        partials_log("ERROR", "Невалидный вызов partial решения");
        g_invalid_partials++;
        return PARTIAL_INVALID_CHALLENGE;
    }
    
    g_valid_partials++;
    
    char launcher_id_hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(launcher_id_hex + i * 2, "%02x", partial->launcher_id[i]);
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), 
             "Partial решение валидно: фермер=%s, сложность=%lu, очки=%lu", 
             launcher_id_hex, partial->difficulty, partial->points);
    partials_log("INFO", log_msg);
    
    return PARTIAL_VALID;
}

bool partial_verify_proof(const partial_t* partial) {
    if (!partial) {
        partials_log("ERROR", "Partial решение не может быть NULL");
        return false;
    }
    
    proof_verification_params_t params = {
        .challenge = *(uint64_t*)partial->challenge,
        .k_size = partial->plot_size,
        .sub_slot_iters = 37600000000ULL, // 37.6 миллиардов
        .difficulty = (uint32_t)partial->difficulty,
        .required_iterations = 0
    };
    
    proof_metadata_t metadata;
    proof_verification_result_t result = proof_verify_space(partial->proof, 
                                                           sizeof(partial->proof),
                                                           &params, &metadata);
    
    if (result != PROOF_VALID) {
        proof_log_verification_result(result, metadata.plot_id);
        return false;
    }
    
    // Сохраняем вычисленные очки
    *(uint64_t*)&partial->points = metadata.iterations;
    
    partials_log("DEBUG", "Доказательство пространства верифицировано успешно");
    return true;
}

bool partial_verify_signature(const partial_t* partial) {
    if (!partial) {
        partials_log("ERROR", "Partial решение не может быть NULL");
        return false;
    }
    
    // Создаем сообщение для проверки подписи
    uint8_t message[128];
    memcpy(message, partial->launcher_id, 32);
    memcpy(message + 32, partial->challenge, 32);
    memcpy(message + 64, partial->proof, 32); // Первые 32 байта proof
    memcpy(message + 96, &partial->timestamp, sizeof(uint64_t));
    
    // Получаем публичный ключ фермера из синглтона
    singleton_t farmer_singleton;
    if (!singleton_init(partial->launcher_id, &farmer_singleton)) {
        partials_log("ERROR", "Не удалось получить синглтон для проверки подписи");
        return false;
    }
    
    // Проверяем BLS подпись
    if (!auth_bls_verify_signature(farmer_singleton.owner_public_key, message, 
                                  sizeof(message), partial->signature)) {
        partials_log("ERROR", "Невалидная BLS подпись partial решения");
        return false;
    }
    
    partials_log("DEBUG", "Подпись partial решения верифицирована успешно");
    return true;
}

bool partial_verify_challenge(const uint8_t* challenge) {
    if (!challenge) {
        partials_log("ERROR", "Challenge не может быть NULL");
        return false;
    }
    
    // Получаем текущую точку сигнейджа из блокчейна
    signage_point_t current_sp = chia_get_current_signage_point();
    
    // Сравниваем challenge с текущей точкой сигнейджа
    if (memcmp(challenge, current_sp.challenge_hash, 32) != 0) {
        partials_log("WARNING", "Challenge не соответствует текущей точке сигнейджа");
        return false;
    }
    
    partials_log("DEBUG", "Challenge верифицирован успешно");
    return true;
}

bool partial_process(const partial_t* partial) {
    if (!partial) {
        partials_log("ERROR", "Partial решение не может быть NULL");
        return false;
    }
    
    // Валидация partial решения
    partial_validation_result_t result = partial_validate(partial);
    
    // Логируем результат валидации
    partial_log_validation_result(result, partial->launcher_id);
    
    if (result != PARTIAL_VALID) {
        return false;
    }
    
    // Обновляем статистику фермера
    singleton_t farmer_singleton;
    if (!singleton_init(partial->launcher_id, &farmer_singleton)) {
        partials_log("ERROR", "Не удалось обновить статистику фермера");
        return false;
    }
    
    farmer_singleton.total_points += partial->points;
    farmer_singleton.last_partial_time = partial->timestamp;
    
    // Адаптируем сложность если необходимо
    // (реализация в difficulty_manager)
    
    partials_log("INFO", "Partial решение успешно обработано");
    return true;
}

void partial_log_validation_result(partial_validation_result_t result, const uint8_t* launcher_id) {
    const char* result_str = "UNKNOWN";
    
    switch (result) {
        case PARTIAL_VALID: result_str = "VALID"; break;
        case PARTIAL_INVALID_SIGNATURE: result_str = "INVALID_SIGNATURE"; break;
        case PARTIAL_INVALID_PROOF: result_str = "INVALID_PROOF"; break;
        case PARTIAL_INVALID_CHALLENGE: result_str = "INVALID_CHALLENGE"; break;
        case PARTIAL_INVALID_SINGLETON: result_str = "INVALID_SINGLETON"; break;
        case PARTIAL_TOO_LATE: result_str = "TOO_LATE"; break;
        case PARTIAL_DUPLICATE: result_str = "DUPLICATE"; break;
        case PARTIAL_INTERNAL_ERROR: result_str = "INTERNAL_ERROR"; break;
    }
    
    char launcher_id_hex[65] = {0};
    if (launcher_id) {
        for (int i = 0; i < 32; i++) {
            sprintf(launcher_id_hex + i * 2, "%02x", launcher_id[i]);
        }
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), 
             "Результат валидации partial: %s, фермер=%s", 
             result_str, launcher_id_hex);
    
    if (result == PARTIAL_VALID) {
        partials_log("INFO", log_msg);
    } else {
        partials_log("WARNING", log_msg);
    }
}

void partials_get_stats(uint64_t* valid, uint64_t* invalid, uint64_t* total) {
    if (valid) *valid = g_valid_partials;
    if (invalid) *invalid = g_invalid_partials;
    if (total) *total = g_total_partials;
    
    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), 
             "Статистика partials: valid=%lu, invalid=%lu, total=%lu", 
             g_valid_partials, g_invalid_partials, g_total_partials);
    partials_log("DEBUG", log_msg);
}
