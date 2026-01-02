#ifndef PARTIALS_H
#define PARTIALS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct partial_t partial_t;
typedef struct partial_queue_node_t partial_queue_node_t;

// Структура частичного решения
struct partial_t {
    uint8_t proof[264];           // Доказательство пространства
    uint8_t farmer_id[48];        // ID фермера (BLS публичный ключ)
    uint8_t launcher_id[32];      // Launcher ID синглтона
    uint64_t timestamp;           // Временная метка
    uint64_t difficulty;          // Сложность
    uint64_t points;              // Начисленные очки
    uint8_t signature[96];        // BLS подпись
    uint8_t challenge[32];        // Вызов
    uint8_t plot_size;            // Размер плота
};

// Структура узла очереди
struct partial_queue_node_t {
    partial_t partial;
    partial_queue_node_t* next;
};

// Результат валидации
typedef enum {
    VALIDATION_SUCCESS,
    VALIDATION_INVALID_SIGNATURE,
    VALIDATION_INVALID_PROOF,
    VALIDATION_EXPIRED,
    VALIDATION_INVALID_DIFFICULTY,
    VALIDATION_RATE_LIMITED,
    VALIDATION_INTERNAL_ERROR,
    VALIDATION_INVALID_SINGLETON,
    VALIDATION_INVALID_CHALLENGE,
    VALIDATION_DUPLICATE,
    VALIDATION_TOO_LATE
} partial_validation_result_t;

// Структура очереди
typedef struct {
    partial_queue_node_t* head;    // Голова очереди
    partial_queue_node_t* tail;    // Хвост очереди
    uint32_t max_size;             // Максимальный размер
    uint32_t size;                 // Текущий размер
    pthread_mutex_t mutex;         // Мьютекс для синхронизации
    pthread_cond_t cond;           // Условная переменная
} partial_queue_t;

// Совместимость со старым кодом
#define PARTIAL_VALID VALIDATION_SUCCESS
#define PARTIAL_INVALID_SIGNATURE VALIDATION_INVALID_SIGNATURE
#define PARTIAL_INVALID_PROOF VALIDATION_INVALID_PROOF
#define PARTIAL_TOO_LATE VALIDATION_TOO_LATE
#define PARTIAL_INVALID_SINGLETON VALIDATION_INVALID_SINGLETON
#define PARTIAL_INVALID_CHALLENGE VALIDATION_INVALID_CHALLENGE
#define PARTIAL_DUPLICATE VALIDATION_DUPLICATE
#define PARTIAL_INTERNAL_ERROR VALIDATION_INTERNAL_ERROR

// Функции очереди
bool partial_queue_init(partial_queue_t* queue, uint32_t capacity);
bool partial_queue_push(partial_queue_t* queue, const partial_t* partial);
bool partial_queue_pop(partial_queue_t* queue, partial_t* partial);
void partial_queue_cleanup(partial_queue_t* queue);

// Функции валидации
partial_validation_result_t partial_validate(const partial_t* partial);
bool partial_verify_proof(const partial_t* partial);
bool partial_verify_signature(const partial_t* partial);
bool partial_verify_challenge(const uint8_t* challenge);

// Основные функции
bool partials_init(void);
bool partials_add(const partial_t* partial);
bool partials_process(const partial_t* partial);

// Вспомогательные функции
void partial_log_validation_result(partial_validation_result_t result, 
                                   const uint8_t* launcher_id);
void partials_get_stats(uint64_t* valid, uint64_t* invalid, uint64_t* total);

#ifdef __cplusplus
}
#endif

#endif // PARTIALS_H
