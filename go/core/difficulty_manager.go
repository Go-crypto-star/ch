package core

import (
    "chia-pool"
    "sync"
    "time"
)

// DifficultyManager управляет сложностью для фермеров
type DifficultyManager struct {
    mu                    sync.RWMutex
    bridge                *pool.PoolBridge
    targetPartialsPerDay  uint64
    minDifficulty         uint64
    maxDifficulty         uint64
    adjustmentInterval    time.Duration
    lastAdjustment        time.Time
}

// NewDifficultyManager создает новый менеджер сложности
func NewDifficultyManager(bridge *pool.PoolBridge, targetPartialsPerDay, minDifficulty, maxDifficulty uint64) *DifficultyManager {
    return &DifficultyManager{
        bridge:               bridge,
        targetPartialsPerDay: targetPartialsPerDay,
        minDifficulty:        minDifficulty,
        maxDifficulty:        maxDifficulty,
        adjustmentInterval:   time.Hour, // Настраиваемый интервал регулировки
        lastAdjustment:       time.Now(),
    }
}

// CalculateNewDifficulty рассчитывает новую сложность для фермера
func (dm *DifficultyManager) CalculateNewDifficulty(farmer *pool.Farmer, currentDifficulty uint64, farmerPoints24h uint64, timeSinceLastPartial uint64) uint64 {
    dm.mu.RLock()
    defer dm.mu.RUnlock()

    // Целевое количество очков в день (примерно targetPartialsPerDay * 1000)
    targetPointsPerDay := dm.targetPartialsPerDay * 1000

    // Если у фермера недостаточно очков за последние 24 часа, уменьшаем сложность
    if farmerPoints24h < targetPointsPerDay {
        newDifficulty := currentDifficulty * 8 / 10 // Уменьшаем на 20%
        if newDifficulty < dm.minDifficulty {
            newDifficulty = dm.minDifficulty
        }
        return newDifficulty
    }

    // Если у фермера слишком много очков, увеличиваем сложность
    if farmerPoints24h > targetPointsPerDay*2 {
        newDifficulty := currentDifficulty * 12 / 10 // Увеличиваем на 20%
        if newDifficulty > dm.maxDifficulty {
            newDifficulty = dm.maxDifficulty
        }
        return newDifficulty
    }

    // Если все в пределах нормы, оставляем текущую сложность
    return currentDifficulty
}

// AdjustDifficulty регулирует сложность для фермера
func (dm *DifficultyManager) AdjustDifficulty(farmer *pool.Farmer) error {
    dm.mu.Lock()
    defer dm.mu.Unlock()

    // Получаем текущую сложность фермера
    currentDifficulty := farmer.Difficulty

    // Получаем количество очков фермера за последние 24 часа
    farmerPoints24h := dm.getFarmerPoints24h(farmer.LauncherID)

    // Время с последнего partial
    timeSinceLastPartial := uint64(time.Since(farmer.Timestamp).Seconds())

    // Рассчитываем новую сложность
    newDifficulty := dm.CalculateNewDifficulty(farmer, currentDifficulty, farmerPoints24h, timeSinceLastPartial)

    // Обновляем сложность фермера
    farmer.Difficulty = newDifficulty
    farmer.Timestamp = time.Now()

    // Обновляем в C библиотеке
    return dm.bridge.UpdateFarmer(*farmer)
}

// getFarmerPoints24h возвращает количество очков фермера за последние 24 часа
func (dm *DifficultyManager) getFarmerPoints24h(launcherID string) uint64 {
    // В реальной реализации здесь будет запрос к базе данных или кешу
    // Для примера возвращаем фиктивное значение
    return 0
}

// AutoAdjustAll автоматически регулирует сложность для всех фермеров
func (dm *DifficultyManager) AutoAdjustAll(farmers []*pool.Farmer) error {
    dm.mu.Lock()
    defer dm.mu.Unlock()

    // Проверяем, прошло ли достаточно времени с последней регулировки
    if time.Since(dm.lastAdjustment) < dm.adjustmentInterval {
        return nil
    }

    for _, farmer := range farmers {
        if err := dm.AdjustDifficulty(farmer); err != nil {
            return err
        }
    }

    dm.lastAdjustment = time.Now()
    return nil
}

// SetTargetPartialsPerDay устанавливает целевое количество partials в день
func (dm *DifficultyManager) SetTargetPartialsPerDay(target uint64) {
    dm.mu.Lock()
    defer dm.mu.Unlock()

    dm.targetPartialsPerDay = target
}

// SetDifficultyRange устанавливает минимальную и максимальную сложность
func (dm *DifficultyManager) SetDifficultyRange(min, max uint64) {
    dm.mu.Lock()
    defer dm.mu.Unlock()

    dm.minDifficulty = min
    dm.maxDifficulty = max
}

// GetDifficultyStats возвращает статистику по сложности
func (dm *DifficultyManager) GetDifficultyStats(farmers []*pool.Farmer) map[string]interface{} {
    dm.mu.RLock()
    defer dm.mu.RUnlock()

    stats := make(map[string]interface{})
    var totalDifficulty uint64
    var minDifficulty uint64 = ^uint64(0)
    var maxDifficulty uint64

    for _, farmer := range farmers {
        diff := farmer.Difficulty
        totalDifficulty += diff

        if diff < minDifficulty {
            minDifficulty = diff
        }

        if diff > maxDifficulty {
            maxDifficulty = diff
        }
    }

    count := uint64(len(farmers))
    if count > 0 {
        stats["average_difficulty"] = totalDifficulty / count
    } else {
        stats["average_difficulty"] = 0
    }

    stats["min_difficulty"] = minDifficulty
    stats["max_difficulty"] = maxDifficulty
    stats["total_farmers"] = count
    stats["target_partials_per_day"] = dm.targetPartialsPerDay

    return stats
}
