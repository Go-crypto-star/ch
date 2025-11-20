package core

import (
    "chia-pool"
    "fmt"
    "sync"
    "time"
)

// PaymentStrategy стратегия выплат
type PaymentStrategy string

const (
    PPS   PaymentStrategy = "pps"   // Pay Per Share
    PPLNS PaymentStrategy = "pplns" // Pay Per Last N Shares
)

// PaymentProcessor управляет выплатами фермерам
type PaymentProcessor struct {
    mu          sync.RWMutex
    bridge      *pool.PoolBridge
    strategy    PaymentStrategy
    minPayout   uint64
    feePercent  float64
    payoutHistory map[string][]pool.Payout // key: launcher_id
}

// NewPaymentProcessor создает новый процессор выплат
func NewPaymentProcessor(bridge *pool.PoolBridge, strategy PaymentStrategy, minPayout uint64, feePercent float64) *PaymentProcessor {
    return &PaymentProcessor{
        bridge:        bridge,
        strategy:      strategy,
        minPayout:     minPayout,
        feePercent:    feePercent,
        payoutHistory: make(map[string][]pool.Payout),
    }
}

// CalculatePayout рассчитывает выплату для фермера
func (pp *PaymentProcessor) CalculatePayout(farmer *pool.Farmer, totalPoints uint64, blockReward uint64) uint64 {
    pp.mu.RLock()
    defer pp.mu.RUnlock()

    if totalPoints == 0 {
        return 0
    }

    var payout uint64

    switch pp.strategy {
    case PPS:
        payout = pp.calculatePPSPayout(farmer, totalPoints, blockReward)
    case PPLNS:
        payout = pp.calculatePPLNSPayout(farmer, totalPoints, blockReward)
    default:
        payout = pp.calculatePPLNSPayout(farmer, totalPoints, blockReward)
    }

    // Apply pool fee
    payout = uint64(float64(payout) * (1 - pp.feePercent/100))

    // Ensure minimum payout
    if payout < pp.minPayout {
        return 0
    }

    return payout
}

// calculatePPSPayout рассчитывает выплату по схеме PPS
func (pp *PaymentProcessor) calculatePPSPayout(farmer *pool.Farmer, totalPoints uint64, blockReward uint64) uint64 {
    // PPS: выплата основана на доле фермера в общем количестве очков
    share := float64(farmer.Points) / float64(totalPoints)
    return uint64(float64(blockReward) * share)
}

// calculatePPLNSPayout рассчитывает выплату по схеме PPLNS
func (pp *PaymentProcessor) calculatePPLNSPayout(farmer *pool.Farmer, totalPoints uint64, blockReward uint64) uint64 {
    // PPLNS: выплата основана на доле фермера в последних N очках
    // В данном упрощенном примере используем общее количество очков
    // В реальной реализации нужно учитывать окно последних N очков
    share := float64(farmer.Points) / float64(totalPoints)
    return uint64(float64(blockReward) * share)
}

// ProcessPayout обрабатывает выплату для фермера
func (pp *PaymentProcessor) ProcessPayout(farmer *pool.Farmer, amount uint64) (*pool.Payout, error) {
    pp.mu.Lock()
    defer pp.mu.Unlock()

    if amount < pp.minPayout {
        return nil, fmt.Errorf("amount below minimum payout: %d < %d", amount, pp.minPayout)
    }

    payout := pool.Payout{
        LauncherID: farmer.LauncherID,
        Amount:     amount,
        Timestamp:  time.Now(),
        Status:     "processed",
    }

    // Сохраняем в историю выплат
    pp.payoutHistory[farmer.LauncherID] = append(pp.payoutHistory[farmer.LauncherID], payout)

    // Обновляем баланс фермера
    farmer.Balance -= amount
    farmer.LastPayout = time.Now()

    // Вызываем C библиотеку для обработки выплаты
    if err := pp.bridge.ProcessPayout(farmer.LauncherID, amount); err != nil {
        return nil, err
    }

    return &payout, nil
}

// GetPayoutHistory возвращает историю выплат для фермера
func (pp *PaymentProcessor) GetPayoutHistory(launcherID string) ([]pool.Payout, error) {
    pp.mu.RLock()
    defer pp.mu.RUnlock()

    history, exists := pp.payoutHistory[launcherID]
    if !exists {
        return nil, fmt.Errorf("no payout history for farmer: %s", launcherID)
    }

    return history, nil
}

// RequestPayout запрашивает выплату для фермера
func (pp *PaymentProcessor) RequestPayout(farmer *pool.Farmer, amount uint64) (*pool.Payout, error) {
    if amount == 0 {
        // Если сумма не указана, используем весь баланс
        amount = farmer.Balance
    }

    if amount > farmer.Balance {
        return nil, fmt.Errorf("insufficient balance: %d > %d", amount, farmer.Balance)
    }

    return pp.ProcessPayout(farmer, amount)
}

// CalculateTotalPayouts рассчитывает общую сумму выплат для всех фермеров
func (pp *PaymentProcessor) CalculateTotalPayouts(farmers []*pool.Farmer, totalPoints uint64, blockReward uint64) map[string]uint64 {
    pp.mu.RLock()
    defer pp.mu.RUnlock()

    payouts := make(map[string]uint64)
    for _, farmer := range farmers {
        payout := pp.CalculatePayout(farmer, totalPoints, blockReward)
        if payout > 0 {
            payouts[farmer.LauncherID] = payout
        }
    }

    return payouts
}

// ProcessBatchPayouts обрабатывает выплаты для нескольких фермеров
func (pp *PaymentProcessor) ProcessBatchPayouts(payouts map[string]uint64) ([]*pool.Payout, error) {
    pp.mu.Lock()
    defer pp.mu.Unlock()

    var processedPayouts []*pool.Payout

    for launcherID, amount := range payouts {
        farmer, err := pp.bridge.GetFarmer(launcherID) // Предполагаем, что есть метод для получения фермера
        if err != nil {
            return nil, err
        }

        payout, err := pp.ProcessPayout(farmer, amount)
        if err != nil {
            return nil, err
        }

        processedPayouts = append(processedPayouts, payout)
    }

    return processedPayouts, nil
}

// GetTotalPaid возвращает общую сумму выплат
func (pp *PaymentProcessor) GetTotalPaid() uint64 {
    pp.mu.RLock()
    defer pp.mu.RUnlock()

    var total uint64
    for _, history := range pp.payoutHistory {
        for _, payout := range history {
            total += payout.Amount
        }
    }

    return total
}
