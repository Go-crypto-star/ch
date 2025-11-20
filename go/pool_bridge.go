package main

/*
#cgo CFLAGS: -I../include
#cgo LDFLAGS: -L../build -lpool

#include "go_bridge.h"
#include <stdlib.h>
#include <string.h>

// Вспомогательные функции для конвертации между Go и C
*/
import "C"

import (
    "fmt"
    "log"
    "unsafe"
    "sync"
)

// PoolBridge управляет взаимодействием с C библиотекой
type PoolBridge struct {
    mu sync.RWMutex
    initialized bool
}

var (
    bridge *PoolBridge
    bridgeOnce sync.Once
)

// GetPoolBridge возвращает синглтон PoolBridge
func GetPoolBridge() *PoolBridge {
    bridgeOnce.Do(func() {
        bridge = &PoolBridge{
            initialized: false,
        }
    })
    return bridge
}

// Initialize инициализирует C библиотеку
func (pb *PoolBridge) Initialize() error {
    pb.mu.Lock()
    defer pb.mu.Unlock()

    if pb.initialized {
        return nil
    }

    success := C.go_bridge_init()
    if !success {
        return fmt.Errorf("failed to initialize C bridge")
    }

    // Регистрируем callback функции
    C.go_bridge_register_log_callback(C.GoLogCallback(C.logCallback))
    C.go_bridge_register_partial_callback(C.GoPartialCallback(C.partialCallback))
    C.go_bridge_register_payout_callback(C.GoPayoutCallback(C.payoutCallback))

    pb.initialized = true
    log.Println("C bridge initialized successfully")
    return nil
}

// Cleanup очищает ресурсы C библиотеки
func (pb *PoolBridge) Cleanup() error {
    pb.mu.Lock()
    defer pb.mu.Unlock()

    if !pb.initialized {
        return nil
    }

    success := C.go_bridge_cleanup()
    if !success {
        return fmt.Errorf("failed to cleanup C bridge")
    }

    pb.initialized = false
    log.Println("C bridge cleaned up successfully")
    return nil
}

// AddFarmer добавляет фермера в пул
func (pb *PoolBridge) AddFarmer(farmer Farmer) error {
    pb.mu.RLock()
    defer pb.mu.RUnlock()

    if !pb.initialized {
        return fmt.Errorf("bridge not initialized")
    }

    cFarmer := farmerToC(farmer)
    success := C.go_bridge_add_farmer(&cFarmer)
    
    // Освобождаем C строки
    defer func() {
        C.free(unsafe.Pointer(C.strdup(&cFarmer.launcher_id[0])))
        C.free(unsafe.Pointer(C.strdup(&cFarmer.pool_url[0])))
    }()

    if !success {
        return fmt.Errorf("failed to add farmer: %s", farmer.LauncherID)
    }

    return nil
}

// UpdateFarmer обновляет информацию о фермере
func (pb *PoolBridge) UpdateFarmer(farmer Farmer) error {
    pb.mu.RLock()
    defer pb.mu.RUnlock()

    if !pb.initialized {
        return fmt.Errorf("bridge not initialized")
    }

    cFarmer := farmerToC(farmer)
    success := C.go_bridge_update_farmer(&cFarmer)
    
    defer func() {
        C.free(unsafe.Pointer(C.strdup(&cFarmer.launcher_id[0])))
        C.free(unsafe.Pointer(C.strdup(&cFarmer.pool_url[0])))
    }()

    if !success {
        return fmt.Errorf("failed to update farmer: %s", farmer.LauncherID)
    }

    return nil
}

// RemoveFarmer удаляет фермера из пула
func (pb *PoolBridge) RemoveFarmer(launcherID string) error {
    pb.mu.RLock()
    defer pb.mu.RUnlock()

    if !pb.initialized {
        return fmt.Errorf("bridge not initialized")
    }

    cLauncherID := C.CString(launcherID)
    defer C.free(unsafe.Pointer(cLauncherID))

    success := C.go_bridge_remove_farmer(cLauncherID)
    if !success {
        return fmt.Errorf("failed to remove farmer: %s", launcherID)
    }

    return nil
}

// ProcessPartial обрабатывает partial решение
func (pb *PoolBridge) ProcessPartial(partial PartialRequest) (bool, error) {
    pb.mu.RLock()
    defer pb.mu.RUnlock()

    if !pb.initialized {
        return false, fmt.Errorf("bridge not initialized")
    }

    cPartial := partialRequestToC(partial)
    success := C.go_bridge_process_partial(&cPartial)
    
    defer func() {
        C.free(unsafe.Pointer(C.strdup(&cPartial.challenge[0])))
        C.free(unsafe.Pointer(C.strdup(&cPartial.launcher_id[0])))
        C.free(unsafe.Pointer(C.strdup(&cPartial.signature[0])))
    }()

    return bool(success), nil
}

// ValidatePartial валидирует partial решение
func (pb *PoolBridge) ValidatePartial(partial PartialRequest) (bool, error) {
    pb.mu.RLock()
    defer pb.mu.RUnlock()

    if !pb.initialized {
        return false, fmt.Errorf("bridge not initialized")
    }

    cPartial := partialRequestToC(partial)
    success := C.go_bridge_validate_partial(&cPartial)
    
    defer func() {
        C.free(unsafe.Pointer(C.strdup(&cPartial.challenge[0])))
        C.free(unsafe.Pointer(C.strdup(&cPartial.launcher_id[0])))
        C.free(unsafe.Pointer(C.strdup(&cPartial.signature[0])))
    }()

    return bool(success), nil
}

// GetPoolInfo возвращает информацию о пуле
func (pb *PoolBridge) GetPoolInfo() (PoolInfo, error) {
    pb.mu.RLock()
    defer pb.mu.RUnlock()

    if !pb.initialized {
        return PoolInfo{}, fmt.Errorf("bridge not initialized")
    }

    var cPoolInfo C.PoolInfo
    success := C.go_bridge_get_pool_info(&cPoolInfo)
    if !success {
        return PoolInfo{}, fmt.Errorf("failed to get pool info")
    }

    return poolInfoFromC(&cPoolInfo), nil
}

// ProcessPayout обрабатывает выплату фермеру
func (pb *PoolBridge) ProcessPayout(launcherID string, amount uint64) error {
    pb.mu.RLock()
    defer pb.mu.RUnlock()

    if !pb.initialized {
        return fmt.Errorf("bridge not initialized")
    }

    cLauncherID := C.CString(launcherID)
    defer C.free(unsafe.Pointer(cLauncherID))

    success := C.go_bridge_process_payout(cLauncherID, C.uint64_t(amount))
    if !success {
        return fmt.Errorf("failed to process payout for farmer: %s", launcherID)
    }

    return nil
}

// CalculatePayouts рассчитывает все выплаты
func (pb *PoolBridge) CalculatePayouts() error {
    pb.mu.RLock()
    defer pb.mu.RUnlock()

    if !pb.initialized {
        return fmt.Errorf("bridge not initialized")
    }

    success := C.go_bridge_calculate_payouts()
    if !success {
        return fmt.Errorf("failed to calculate payouts")
    }

    return nil
}

// GetStatistics возвращает статистику пула
func (pb *PoolBridge) GetStatistics() (uint64, uint64, uint64, uint64, error) {
    pb.mu.RLock()
    defer pb.mu.RUnlock()

    if !pb.initialized {
        return 0, 0, 0, 0, fmt.Errorf("bridge not initialized")
    }

    var totalFarmers, totalPartials, validPartials, totalPoints C.uint64_t
    success := C.go_bridge_get_statistics(&totalFarmers, &totalPartials, &validPartials, &totalPoints)
    if !success {
        return 0, 0, 0, 0, fmt.Errorf("failed to get statistics")
    }

    return uint64(totalFarmers), uint64(totalPartials), uint64(validPartials), uint64(totalPoints), nil
}

// LogDebug отправляет debug сообщение в C библиотеку
func (pb *PoolBridge) LogDebug(message string) {
    cMessage := C.CString(message)
    defer C.free(unsafe.Pointer(cMessage))
    C.go_bridge_log_debug(cMessage)
}

// LogInfo отправляет info сообщение в C библиотеку
func (pb *PoolBridge) LogInfo(message string) {
    cMessage := C.CString(message)
    defer C.free(unsafe.Pointer(cMessage))
    C.go_bridge_log_info(cMessage)
}

// LogError отправляет error сообщение в C библиотеку
func (pb *PoolBridge) LogError(message string) {
    cMessage := C.CString(message)
    defer C.free(unsafe.Pointer(cMessage))
    C.go_bridge_log_error(cMessage)
}

// LogFatal отправляет fatal сообщение в C библиотеку
func (pb *PoolBridge) LogFatal(message string) {
    cMessage := C.CString(message)
    defer C.free(unsafe.Pointer(cMessage))
    C.go_bridge_log_fatal(cMessage)
}

// IsInitialized возвращает статус инициализации
func (pb *PoolBridge) IsInitialized() bool {
    pb.mu.RLock()
    defer pb.mu.RUnlock()
    return pb.initialized
}

//export logCallback
func logCallback(message *C.char, level C.int) {
    goMessage := C.GoString(message)
    switch level {
    case 0:
        log.Printf("[DEBUG] %s", goMessage)
    case 1:
        log.Printf("[INFO] %s", goMessage)
    case 2:
        log.Printf("[WARN] %s", goMessage)
    case 3:
        log.Printf("[ERROR] %s", goMessage)
    case 4:
        log.Printf("[FATAL] %s", goMessage)
    default:
        log.Printf("[UNKNOWN] %s", goMessage)
    }
}

//export partialCallback
func partialCallback(partial *C.PartialRequest) {
    // Обработка partial callback
    launcherID := C.GoString(&partial.launcher_id[0])
    challenge := C.GoString(&partial.challenge[0])
    
    log.Printf("Partial callback received: launcher_id=%s, challenge=%s", 
        launcherID, challenge)
}

//export payoutCallback
func payoutCallback(launcherID *C.char, amount C.uint64_t) {
    // Обработка payout callback
    goLauncherID := C.GoString(launcherID)
    goAmount := uint64(amount)
    
    log.Printf("Payout callback received: launcher_id=%s, amount=%d", 
        goLauncherID, goAmount)
}
