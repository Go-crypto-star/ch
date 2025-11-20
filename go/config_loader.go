package main

import (
    "encoding/json"
    "fmt"
    "io/ioutil"
    "os"
)

// Config представляет конфигурацию пула
type Config struct {
    PoolName         string  `json:"pool_name"`
    PoolURL          string  `json:"pool_url"`
    Port             int     `json:"port"`
    PoolFee          float64 `json:"pool_fee"`
    MinPayout        uint64  `json:"min_payout"`
    PartialDeadline  uint32  `json:"partial_deadline"`
    DifficultyTarget uint32  `json:"difficulty_target"`
    NodeRPCHost      string  `json:"node_rpc_host"`
    NodeRPCPort      int     `json:"node_rpc_port"`
    NodeRPCCertPath  string  `json:"node_rpc_cert_path"`
    NodeRPCKeyPath   string  `json:"node_rpc_key_path"`
    DatabasePath     string  `json:"database_path"`
    LogLevel         string  `json:"log_level"`
    LogPath          string  `json:"log_path"`
}

// LoadConfig загружает конфигурацию из файла
func LoadConfig(filename string) (*Config, error) {
    data, err := ioutil.ReadFile(filename)
    if err != nil {
        return nil, fmt.Errorf("failed to read config file: %v", err)
    }

    var config Config
    if err := json.Unmarshal(data, &config); err != nil {
        return nil, fmt.Errorf("failed to parse config file: %v", err)
    }

    return &config, nil
}

// SaveConfig сохраняет конфигурацию в файл
func SaveConfig(filename string, config *Config) error {
    data, err := json.MarshalIndent(config, "", "  ")
    if err != nil {
        return fmt.Errorf("failed to marshal config: %v", err)
    }

    if err := ioutil.WriteFile(filename, data, 0644); err != nil {
        return fmt.Errorf("failed to write config file: %v", err)
    }

    return nil
}

// DefaultConfig возвращает конфигурацию по умолчанию
func DefaultConfig() *Config {
    return &Config{
        PoolName:         "Chia Reference Pool",
        PoolURL:          "https://pool.example.com",
        Port:             8444,
        PoolFee:          1.0,
        MinPayout:        1000000000, // 0.001 XCH
        PartialDeadline:  28,
        DifficultyTarget: 300,
        NodeRPCHost:      "localhost",
        NodeRPCPort:      8555,
        NodeRPCCertPath:  "/root/.chia/mainnet/config/ssl/full_node/private_full_node.crt",
        NodeRPCKeyPath:   "/root/.chia/mainnet/config/ssl/full_node/private_full_node.key",
        DatabasePath:     "/var/lib/chiapool/pool.db",
        LogLevel:         "info",
        LogPath:          "/var/log/chiapool/pool.log",
    }
}

// ValidateConfig проверяет валидность конфигурации
func ValidateConfig(config *Config) error {
    if config.PoolName == "" {
        return fmt.Errorf("pool_name cannot be empty")
    }
    if config.PoolURL == "" {
        return fmt.Errorf("pool_url cannot be empty")
    }
    if config.Port < 1 || config.Port > 65535 {
        return fmt.Errorf("invalid port: %d", config.Port)
    }
    if config.PoolFee < 0 || config.PoolFee > 100 {
        return fmt.Errorf("invalid pool fee: %.2f", config.PoolFee)
    }
    if config.NodeRPCHost == "" {
        return fmt.Errorf("node_rpc_host cannot be empty")
    }
    if config.NodeRPCPort < 1 || config.NodeRPCPort > 65535 {
        return fmt.Errorf("invalid node_rpc_port: %d", config.NodeRPCPort)
    }
    return nil
}

// PrintConfig печатает конфигурацию
func PrintConfig(config *Config) {
    fmt.Printf("Pool Configuration:\n")
    fmt.Printf("  Pool Name: %s\n", config.PoolName)
    fmt.Printf("  Pool URL: %s\n", config.PoolURL)
    fmt.Printf("  Port: %d\n", config.Port)
    fmt.Printf("  Pool Fee: %.2f%%\n", config.PoolFee)
    fmt.Printf("  Min Payout: %d mojos\n", config.MinPayout)
    fmt.Printf("  Partial Deadline: %d seconds\n", config.PartialDeadline)
    fmt.Printf("  Difficulty Target: %d\n", config.DifficultyTarget)
    fmt.Printf("  Node RPC Host: %s\n", config.NodeRPCHost)
    fmt.Printf("  Node RPC Port: %d\n", config.NodeRPCPort)
    fmt.Printf("  Node RPC Cert Path: %s\n", config.NodeRPCCertPath)
    fmt.Printf("  Node RPC Key Path: %s\n", config.NodeRPCKeyPath)
    fmt.Printf("  Database Path: %s\n", config.DatabasePath)
    fmt.Printf("  Log Level: %s\n", config.LogLevel)
    fmt.Printf("  Log Path: %s\n", config.LogPath)
}
