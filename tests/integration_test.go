package main

import (
	"testing"
	"time"
	"unsafe"
)

/*
#include "../include/go_bridge.h"
#include <stdlib.h>

// Обертки для C функций
bool test_pool_init() {
    PoolInfo pool_info;
    return go_bridge_get_pool_info(&pool_info);
}

bool test_process_partial() {
    PartialRequest partial = {
        .challenge = "a1b2c3d4e5f6789012345678901234567890123456789012345678901234567890",
        .launcher_id = "f1e2d3c4b5a6879065432167890543216789054321678905432167890543216",
        .signature = "1a2b3c4d5e6f7890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef",
        .timestamp = 1234567890,
        .difficulty = 1000
    };
    return go_bridge_process_partial(&partial);
}
*/
import "C"

func TestPoolInitialization(t *testing.T) {
	// Инициализация Go бриджа
	if !C.go_bridge_init() {
		t.Fatal("Failed to initialize Go bridge")
	}
	defer C.go_bridge_cleanup()

	// Тест получения информации о пуле
	if !C.test_pool_init() {
		t.Error("Failed to get pool info")
	}
}

func TestPartialProcessing(t *testing.T) {
	if !C.go_bridge_init() {
		t.Fatal("Failed to initialize Go bridge")
	}
	defer C.go_bridge_cleanup()

	// Тест обработки partial
	if !C.test_process_partial() {
		t.Error("Failed to process partial")
	}
}

func TestFarmerManagement(t *testing.T) {
	if !C.go_bridge_init() {
		t.Fatal("Failed to initialize Go bridge")
	}
	defer C.go_bridge_cleanup()

	// Тест добавления фермера
	farmer := C.FarmerInfo{
		launcher_id: C.CString("test_launcher_1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcd"),
		pool_url:    C.CString("https://test.pool.example.com"),
		difficulty:  1000,
		points:      50000,
		partials:    50,
		timestamp:   C.uint64_t(time.Now().Unix()),
	}
	defer C.free(unsafe.Pointer(farmer.launcher_id))
	defer C.free(unsafe.Pointer(farmer.pool_url))

	if !C.go_bridge_add_farmer(&farmer) {
		t.Error("Failed to add farmer")
	}

	// Тест обновления фермера
	farmer.points = 60000
	if !C.go_bridge_update_farmer(&farmer) {
		t.Error("Failed to update farmer")
	}

	// Тест удаления фермера
	if !C.go_bridge_remove_farmer(farmer.launcher_id) {
		t.Error("Failed to remove farmer")
	}
}

func TestStatistics(t *testing.T) {
	if !C.go_bridge_init() {
		t.Fatal("Failed to initialize Go bridge")
	}
	defer C.go_bridge_cleanup()

	var totalFarmers, totalPartials, validPartials, totalPoints C.uint64_t

	if !C.go_bridge_get_statistics(&totalFarmers, &totalPartials, &validPartials, &totalPoints) {
		t.Error("Failed to get statistics")
	}

	t.Logf("Statistics: Farmers=%d, Partials=%d, Valid=%d, Points=%d",
		totalFarmers, totalPartials, validPartials, totalPoints)
}

func TestPayoutProcessing(t *testing.T) {
	if !C.go_bridge_init() {
		t.Fatal("Failed to initialize Go bridge")
	}
	defer C.go_bridge_cleanup()

	launcherID := C.CString("test_launcher_1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcd")
	defer C.free(unsafe.Pointer(launcherID))

	// Тест обработки выплаты
	if !C.go_bridge_process_payout(launcherID, 1000000000) { // 0.001 XCH
		t.Error("Failed to process payout")
	}

	// Тест расчета всех выплат
	if !C.go_bridge_calculate_payouts() {
		t.Error("Failed to calculate payouts")
	}
}

func TestLogging(t *testing.T) {
	if !C.go_bridge_init() {
		t.Fatal("Failed to initialize Go bridge")
	}
	defer C.go_bridge_cleanup()

	// Тестируем различные уровни логирования
	debugMsg := C.CString("Debug message from integration test")
	infoMsg := C.CString("Info message from integration test")
	errorMsg := C.CString("Error message from integration test")

	defer C.free(unsafe.Pointer(debugMsg))
	defer C.free(unsafe.Pointer(infoMsg))
	defer C.free(unsafe.Pointer(errorMsg))

	C.go_bridge_log_debug(debugMsg)
	C.go_bridge_log_info(infoMsg)
	C.go_bridge_log_error(errorMsg)
}

// Callback функции для тестирования
//export testLogCallback
func testLogCallback(message *C.char, level C.int) {
	msg := C.GoString(message)
	println("Test Log Callback:", msg)
}

//export testPartialCallback
func testPartialCallback(partial *C.PartialRequest) {
	launcherID := C.GoString(partial.launcher_id)
	println("Test Partial Callback:", launcherID)
}

//export testPayoutCallback
func testPayoutCallback(launcherID *C.char, amount C.uint64_t) {
	id := C.GoString(launcherID)
	println("Test Payout Callback:", id, amount)
}

func TestCallbacks(t *testing.T) {
	if !C.go_bridge_init() {
		t.Fatal("Failed to initialize Go bridge")
	}
	defer C.go_bridge_cleanup()

	// Регистрируем callback функции
	if !C.go_bridge_register_log_callback(C.GoLogCallback(C.testLogCallback)) {
		t.Error("Failed to register log callback")
	}

	if !C.go_bridge_register_partial_callback(C.GoPartialCallback(C.testPartialCallback)) {
		t.Error("Failed to register partial callback")
	}

	if !C.go_bridge_register_payout_callback(C.GoPayoutCallback(C.testPayoutCallback)) {
		t.Error("Failed to register payout callback")
	}

	// Вызываем функции которые должны триггерить callback
	C.go_bridge_log_info(C.CString("Testing callback"))
}

func BenchmarkPartialProcessing(b *testing.B) {
	if !C.go_bridge_init() {
		b.Fatal("Failed to initialize Go bridge")
	}
	defer C.go_bridge_cleanup()

	partial := C.PartialRequest{
		challenge:   C.CString("benchmark_challenge_1234567890abcdef1234567890abcdef1234567890abcdef1234567890"),
		launcher_id: C.CString("benchmark_launcher_1234567890abcdef1234567890abcdef1234567890abcdef1234567890"),
		signature:   C.CString("benchmark_signature_1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890"),
		timestamp:   C.uint64_t(time.Now().Unix()),
		difficulty:  1000,
	}
	defer C.free(unsafe.Pointer(partial.challenge))
	defer C.free(unsafe.Pointer(partial.launcher_id))
	defer C.free(unsafe.Pointer(partial.signature))

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		if !C.go_bridge_process_partial(&partial) {
			b.Error("Failed to process partial in benchmark")
		}
	}
}

func BenchmarkStatistics(b *testing.B) {
	if !C.go_bridge_init() {
		b.Fatal("Failed to initialize Go bridge")
	}
	defer C.go_bridge_cleanup()

	var totalFarmers, totalPartials, validPartials, totalPoints C.uint64_t

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		if !C.go_bridge_get_statistics(&totalFarmers, &totalPartials, &validPartials, &totalPoints) {
			b.Error("Failed to get statistics in benchmark")
		}
	}
}

func main() {
	// Запускаем тесты если файл выполняется напрямую
	t := &testing.T{}
	TestPoolInitialization(t)
	TestPartialProcessing(t)
	TestFarmerManagement(t)
	TestStatistics(t)
	TestPayoutProcessing(t)
	TestLogging(t)
	TestCallbacks(t)
}
