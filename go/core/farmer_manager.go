package core

import (
    "chia-pool"
    "sync"
    "time"
)

// FarmerManager управляет фермерами и их данными
type FarmerManager struct {
    mu      sync.RWMutex
    farmers map[string]*pool.Farmer // key: launcher_id
    bridge  *pool.PoolBridge
}

// NewFarmerManager создает новый менеджер фермеров
func NewFarmerManager(bridge *pool.PoolBridge) *FarmerManager {
    return &FarmerManager{
        farmers: make(map[string]*pool.Farmer),
        bridge:  bridge,
    }
}

// AddFarmer добавляет фермера
func (fm *FarmerManager) AddFarmer(farmer pool.Farmer) error {
    fm.mu.Lock()
    defer fm.mu.Unlock()

    if _, exists := fm.farmers[farmer.LauncherID]; exists {
        return fmt.Errorf("farmer already exists: %s", farmer.LauncherID)
    }

    fm.farmers[farmer.LauncherID] = &farmer

    // Обновляем в C библиотеке
    if err := fm.bridge.AddFarmer(farmer); err != nil {
        delete(fm.farmers, farmer.LauncherID)
        return err
    }

    return nil
}

// GetFarmer возвращает фермера по launcher_id
func (fm *FarmerManager) GetFarmer(launcherID string) (*pool.Farmer, error) {
    fm.mu.RLock()
    defer fm.mu.RUnlock()

    farmer, exists := fm.farmers[launcherID]
    if !exists {
        return nil, fmt.Errorf("farmer not found: %s", launcherID)
    }

    return farmer, nil
}

// UpdateFarmer обновляет данные фермера
func (fm *FarmerManager) UpdateFarmer(farmer pool.Farmer) error {
    fm.mu.Lock()
    defer fm.mu.Unlock()

    if _, exists := fm.farmers[farmer.LauncherID]; !exists {
        return fmt.Errorf("farmer not found: %s", farmer.LauncherID)
    }

    fm.farmers[farmer.LauncherID] = &farmer

    // Обновляем в C библиотеке
    if err := fm.bridge.UpdateFarmer(farmer); err != nil {
        return err
    }

    return nil
}

// RemoveFarmer удаляет фермера
func (fm *FarmerManager) RemoveFarmer(launcherID string) error {
    fm.mu.Lock()
    defer fm.mu.Unlock()

    if _, exists := fm.farmers[launcherID]; !exists {
        return fmt.Errorf("farmer not found: %s", launcherID)
    }

    delete(fm.farmers, launcherID)

    // Удаляем из C библиотеки
    if err := fm.bridge.RemoveFarmer(launcherID); err != nil {
        // Восстанавливаем в мапе, если не удалось удалить из C?
        // Пока просто возвращаем ошибку, но фермер уже удален из мапы
        return err
    }

    return nil
}

// GetAllFarmers возвращает всех фермеров
func (fm *FarmerManager) GetAllFarmers() []pool.Farmer {
    fm.mu.RLock()
    defer fm.mu.RUnlock()

    farmers := make([]pool.Farmer, 0, len(fm.farmers))
    for _, farmer := range fm.farmers {
        farmers = append(farmers, *farmer)
    }

    return farmers
}

// UpdateFarmerPoints обновляет очки фермера
func (fm *FarmerManager) UpdateFarmerPoints(launcherID string, points uint64) error {
    fm.mu.Lock()
    defer fm.mu.Unlock()

    farmer, exists := fm.farmers[launcherID]
    if !exists {
        return fmt.Errorf("farmer not found: %s", launcherID)
    }

    farmer.Points += points
    farmer.Timestamp = time.Now()

    // Обновляем в C библиотеке
    return fm.bridge.UpdateFarmer(*farmer)
}

// UpdateFarmerDifficulty обновляет сложность фермера
func (fm *FarmerManager) UpdateFarmerDifficulty(launcherID string, difficulty uint64) error {
    fm.mu.Lock()
    defer fm.mu.Unlock()

    farmer, exists := fm.farmers[launcherID]
    if !exists {
        return fmt.Errorf("farmer not found: %s", launcherID)
    }

    farmer.Difficulty = difficulty
    farmer.Timestamp = time.Now()

    // Обновляем в C библиотеке
    return fm.bridge.UpdateFarmer(*farmer)
}

// IncrementFarmerPartials увеличивает счетчик partials фермера
func (fm *FarmerManager) IncrementFarmerPartials(launcherID string) error {
    fm.mu.Lock()
    defer fm.mu.Unlock()

    farmer, exists := fm.farmers[launcherID]
    if !exists {
        return fmt.Errorf("farmer not found: %s", launcherID)
    }

    farmer.Partials++
    farmer.Timestamp = time.Now()

    // Обновляем в C библиотеке
    return fm.bridge.UpdateFarmer(*farmer)
}

// GetFarmerCount возвращает количество фермеров
func (fm *FarmerManager) GetFarmerCount() int {
    fm.mu.RLock()
    defer fm.mu.RUnlock()

    return len(fm.farmers)
}

// GetTotalPoints возвращает общее количество очков всех фермеров
func (fm *FarmerManager) GetTotalPoints() uint64 {
    fm.mu.RLock()
    defer fm.mu.RUnlock()

    var total uint64
    for _, farmer := range fm.farmers {
        total += farmer.Points
    }

    return total
}

// GetActiveFarmers возвращает количество активных фермеров (за последние 24 часа)
func (fm *FarmerManager) GetActiveFarmers() int {
    fm.mu.RLock()
    defer fm.mu.RUnlock()

    activeThreshold := time.Now().Add(-24 * time.Hour)
    count := 0
    for _, farmer := range fm.farmers {
        if farmer.Timestamp.After(activeThreshold) {
            count++
        }
    }

    return count
}
