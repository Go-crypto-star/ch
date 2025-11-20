package main

import (
    "chia-pool/api"
    "chia-pool/core"
    "fmt"
    "log"
    "os"
    "os/signal"
    "syscall"
)

func main() {
    // Инициализация логгера
    log.SetFlags(log.LstdFlags | log.Lshortfile)

    // Загрузка конфигурации
    config, err := LoadConfig("config/pool_config.json")
    if err != nil {
        log.Printf("Failed to load config, using defaults: %v", err)
        config = DefaultConfig()
    }

    // Валидация конфигурации
    if err := ValidateConfig(config); err != nil {
        log.Fatalf("Invalid config: %v", err)
    }

    // Печать конфигурации
    PrintConfig(config)

    // Инициализация C bridge
    bridge := GetPoolBridge()
    if err := bridge.Initialize(); err != nil {
        log.Fatalf("Failed to initialize C bridge: %v", err)
    }
    defer bridge.Cleanup()

    // Инициализация менеджеров
    farmerManager := core.NewFarmerManager(bridge)
    paymentProcessor := core.NewPaymentProcessor(bridge, core.PPLNS, config.MinPayout, config.PoolFee)
    difficultyManager := core.NewDifficultyManager(bridge, uint64(config.DifficultyTarget), 1, 1000000)

    // Создание и запуск API сервера
    server := api.NewServer(config.Port, farmerManager, paymentProcessor, difficultyManager)
    
    // Запуск сервера в горутине
    go func() {
        if err := server.Start(); err != nil {
            log.Fatalf("Failed to start API server: %v", err)
        }
    }()

    log.Printf("Chia Pool started successfully on port %d", config.Port)
    log.Printf("Pool URL: %s", config.PoolURL)
    log.Printf("Pool Fee: %.2f%%", config.PoolFee)

    // Ожидание сигнала завершения
    waitForShutdown()

    log.Println("Shutting down Chia Pool...")
}

// waitForShutdown ожидает сигнал завершения
func waitForShutdown() {
    sigChan := make(chan os.Signal, 1)
    signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
    <-sigChan
}

// Пример регистрации фермера
func exampleRegisterFarmer(farmerManager *core.FarmerManager) {
    farmer := Farmer{
        LauncherID: "a1b2c3d4e5f6789012345678901234567890123456789012345678901234567890",
        PoolURL:    "https://pool.example.com",
        Difficulty: 1000,
        Points:     0,
        Partials:   0,
        Timestamp:  time.Now(),
        IsActive:   true,
        Balance:    0,
    }

    if err := farmerManager.AddFarmer(farmer); err != nil {
        log.Printf("Failed to register farmer: %v", err)
    } else {
        log.Printf("Farmer registered successfully: %s", farmer.LauncherID)
    }
}

// Пример обработки partial
func exampleProcessPartial(farmerManager *core.FarmerManager, paymentProcessor *core.PaymentProcessor) {
    partial := PartialRequest{
        Challenge:  "c1d2e3f4a5b6789012345678901234567890123456789012345678901234567890",
        LauncherID: "a1b2c3d4e5f6789012345678901234567890123456789012345678901234567890",
        Signature:  "s1g2n3a4t5u6r7e890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123",
        Timestamp:  time.Now().Unix(),
        Difficulty: 1000,
        PlotSize:   32,
    }

    success, err := bridge.ProcessPartial(partial)
    if err != nil {
        log.Printf("Failed to process partial: %v", err)
    } else if success {
        log.Printf("Partial processed successfully for farmer: %s", partial.LauncherID)
        
        // Обновляем статистику фермера
        if err := farmerManager.IncrementFarmerPartials(partial.LauncherID); err != nil {
            log.Printf("Failed to update farmer partials: %v", err)
        }
    } else {
        log.Printf("Invalid partial from farmer: %s", partial.LauncherID)
    }
}

// Пример обработки выплат
func exampleProcessPayout(farmerManager *core.FarmerManager, paymentProcessor *core.PaymentProcessor) {
    farmer, err := farmerManager.GetFarmer("a1b2c3d4e5f6789012345678901234567890123456789012345678901234567890")
    if err != nil {
        log.Printf("Failed to get farmer: %v", err)
        return
    }

    // Предположим, что у фермера есть очки и мы рассчитываем выплату
    totalPoints := farmerManager.GetTotalPoints()
    blockReward := uint64(1750000000000) // 1.75 XCH в mojos

    payoutAmount := paymentProcessor.CalculatePayout(farmer, totalPoints, blockReward)
    if payoutAmount > 0 {
        payout, err := paymentProcessor.ProcessPayout(farmer, payoutAmount)
        if err != nil {
            log.Printf("Failed to process payout: %v", err)
        } else {
            log.Printf("Payout processed: %s - %d mojos", payout.LauncherID, payout.Amount)
        }
    }
}
