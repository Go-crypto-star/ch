#ifndef PARTIALS_H
#define PARTIALS_H

#include <cstddef>
#include <stdint.h>
#include <stdbool.h>

// Структура partial решения
typedef struct {
    uint8_t launcher_id[32];       // ID синглтона фермера
    uint8_t challenge[32];         // Вызов (challenge)
    uint8_t proof[368];            // Доказательство пространства (Proof of Space)
    uint8_t signature[96];         // BLS подпись
    uint64_t timestamp;            // Время получения
    uint64_t difficulty;           // Сложность на момент отправки
    uint32_t plot_size;            // Размер плота (k-size)
    uint8_t plot_id[32];           // ID плота
    uint64 points;                 // Начисленные очки
} partial_t;

// Результат валидации partial
typedef enum {
    PARTIAL_VALID,
    PARTIAL_INVALID_SIGNATURE,
    PARTIAL_INVALID_PROOF,
    PARTIAL_INVALID_CHALLENGE,
    PARTIAL_INVALID_SINGLETON,
    PARTIAL_TOO_LATE,
    PARTIAL_DUPLICATE,
    PARTIAL_INTERNAL_ERROR
} partial_validation_result_t;

// Очередь partial решений
typedef struct partial_queue_node {
    partial_t partial;
    struct partial_queue_node* next;
} partial_queue_node_t;

typedef struct {
    partial_queue_node_t* head;
    partial_queue_node_t* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint32_t size;
    uint32_t max_size;
} partial_queue_t;

// Инициализация очереди
bool partial_queue_init(partial_queue_t* queue, uint32_t max_size);
bool partial_queue_push(partial_queue_t* queue, const partial_t* partial);
bool partial_queue_pop(partial_queue_t* queue, partial_t* partial);
void partial_queue_cleanup(partial_queue_t* queue);

// Валидация partial
partial_validation_result_t partial_validate(const partial_t* partial);
bool partial_verify_proof(const partial_t* partial);
bool partial_verify_signature(const partial_t* partial);
bool partial_verify_challenge(const uint8_t* challenge);

// Обработка partial
bool partial_process(const partial_t* partial);
void partial_log_validation_result(partial_validation_result_t result, const uint8_t* launcher_id);

// Статистика
void partials_get_stats(uint64_t* valid, uint64_t* invalid, uint64_t* total);

#endif // PARTIALS_H
