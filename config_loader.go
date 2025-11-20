package pool

import (
    "encoding/json"
    "io/ioutil"
    "log"
    "math/big"
)

// PoolConfig представляет конфигурацию пула
type PoolConfig struct {
    PoolName string `json:"pool_name"`
    Version  string `json:"version"`
    
    TokenSettings struct {
        TokenA TokenConfig `json:"token_a"`
        TokenB TokenConfig `json:"token_b"`
    } `json:"token_settings"`
    
    PoolParameters struct {
        FeeRate           int    `json:"fee_rate"`
        ProtocolFeeRate   int    `json:"protocol_fee_rate"`
        MaxSwapAmount     string `json:"max_swap_amount"`
        MinLiquidity      string `json:"min_liquidity"`
        PricePrecision    int    `json:"price_precision"`
        MaxRatioDeviation int    `json:"max_ratio_deviation"`
    } `json:"pool_parameters"`
    
    SecuritySettings struct {
        MaxSwapSlippage int    `json:"max_swap_slippage"`
        WithdrawTimeout int    `json:"withdraw_timeout"`
        EmergencyPause  bool   `json:"emergency_pause"`
        AdminAddress    string `json:"admin_address"`
    } `json:"security_settings"`
    
    PerformanceSettings struct {
        UseAssemblyMath bool   `json:"use_assembly_math"`
        HashAlgorithm   string `json:"hash_algorithm"`
        CacheSize       int    `json:"cache_size"`
        BatchOperations bool   `json:"batch_operations"`
    } `json:"performance_settings"`
}

type TokenConfig struct {
    Name     string `json:"name"`
    Symbol   string `json:"symbol"`
    Decimals int    `json:"decimals"`
    Address  string `json:"address"`
}

// LoadConfig загружает конфигурацию из JSON файла
func LoadConfig(filename string) (*PoolConfig, error) {
    data, err := ioutil.ReadFile(filename)
    if err != nil {
        return nil, err
    }
    
    var config PoolConfig
    if err := json.Unmarshal(data, &config); err != nil {
        return nil, err
    }
    
    return &config, nil
}

// ConvertToBigInt конвертирует строку в big.Int
func (pc *PoolConfig) ConvertToBigInt(value string) *big.Int {
    result := new(big.Int)
    result, ok := result.SetString(value, 10)
    if !ok {
        log.Printf("Warning: Failed to convert %s to big.Int", value)
        return big.NewInt(0)
    }
    return result
}

// ValidateConfig проверяет валидность конфигурации
func (pc *PoolConfig) ValidateConfig() error {
    // Проверка параметров пула
    if pc.PoolParameters.FeeRate > 10000 { // 100%
        return fmt.Errorf("fee rate too high: %d", pc.PoolParameters.FeeRate)
    }
    
    if pc.PoolParameters.ProtocolFeeRate > 5000 { // 50%
        return fmt.Errorf("protocol fee rate too high: %d", pc.PoolParameters.ProtocolFeeRate)
    }
    
    // Проверка адресов
    if !isValidAddress(pc.SecuritySettings.AdminAddress) {
        return fmt.Errorf("invalid admin address: %s", pc.SecuritySettings.AdminAddress)
    }
    
    return nil
}

// isValidAddress проверяет валидность Ethereum адреса
func isValidAddress(address string) bool {
    return len(address) == 42 && address[:2] == "0x"
}
