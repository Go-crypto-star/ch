#ifndef GO_BRIDGE_H
#define GO_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Структуры для передачи данных в Go
typedef struct {
    char launcher_id[65];        // Hex строка
    char pool_url[512];
    uint64_t difficulty;
    uint64_t points;
    uint64_t partials;
    uint64_t timestamp;
} FarmerInfo;

typedef struct {
    char challenge[65];          // Hex строка
    char launcher_id[65];
    char signature[193];         // Hex строка BLS подписи
    uint64_t timestamp;
    uint64_t difficulty;
} PartialRequest;

typedef struct {
    char pool_name[256];
    char pool_url[512];
    uint64_t total_farmers;
    uint64_t total_netspace;
    uint64_t current_difficulty;
    double pool_fee;
    uint64_t min_payout;
} PoolInfo;

// Инициализация Go бриджа
bool go_bridge_init(void);
bool go_bridge_cleanup(void);

// Управление фермерами
bool go_bridge_add_farmer(const FarmerInfo* farmer);
bool go_bridge_update_farmer(const FarmerInfo* farmer);
bool go_bridge_remove_farmer(const char* launcher_id);

// Обработка partial решений
bool go_bridge_process_partial(const PartialRequest* partial);
bool go_bridge_validate_partial(const PartialRequest* partial);

// Получение информации о пуле
bool go_bridge_get_pool_info(PoolInfo* pool_info);

// Выплаты
bool go_bridge_process_payout(const char* launcher_id, uint64_t amount);
bool go_bridge_calculate_payouts(void);

// Статистика
bool go_bridge_get_statistics(uint64_t* total_farmers, uint64_t* total_partials,
                             uint64_t* valid_partials, uint64_t* total_points);

// Логирование в Go
void go_bridge_log_debug(const char* message);
void go_bridge_log_info(const char* message);
void go_bridge_log_error(const char* message);
void go_bridge_log_fatal(const char* message);

// Callback функции из Go
typedef void (*GoLogCallback)(const char* message, int level);
typedef void (*GoPartialCallback)(const PartialRequest* partial);
typedef void (*GoPayoutCallback)(const char* launcher_id, uint64_t amount);

// Регистрация callback функций
bool go_bridge_register_log_callback(GoLogCallback callback);
bool go_bridge_register_partial_callback(GoPartialCallback callback);
bool go_bridge_register_payout_callback(GoPayoutCallback callback);

#ifdef __cplusplus
}
#endif

#endif // GO_BRIDGE_H
