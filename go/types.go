package main

/*
#include "../include/go_bridge.h"
*/
import "C"

import (
    "encoding/hex"
    "time"
    "unsafe"
)

// Farmer представляет фермера в пуле
type Farmer struct {
    LauncherID  string    `json:"launcher_id"`
    PoolURL     string    `json:"pool_url"`
    Difficulty  uint64    `json:"difficulty"`
    Points      uint64    `json:"points"`
    Partials    uint64    `json:"partials"`
    Timestamp   time.Time `json:"timestamp"`
    IsActive    bool      `json:"is_active"`
    Balance     uint64    `json:"balance"`
    LastPayout  time.Time `json:"last_payout"`
}

// PartialRequest представляет запрос на валидацию partial решения
type PartialRequest struct {
    Challenge   string    `json:"challenge"`
    LauncherID  string    `json:"launcher_id"`
    Signature   string    `json:"signature"`
    Timestamp   int64     `json:"timestamp"`
    Difficulty  uint64    `json:"difficulty"`
    PlotSize    uint32    `json:"plot_size"`
}

// PoolInfo представляет информацию о пуле
type PoolInfo struct {
    PoolName          string  `json:"pool_name"`
    PoolURL           string  `json:"pool_url"`
    TotalFarmers      uint64  `json:"total_farmers"`
    TotalNetspace     uint64  `json:"total_netspace"`
    CurrentDifficulty uint64  `json:"current_difficulty"`
    PoolFee           float64 `json:"pool_fee"`
    MinPayout         uint64  `json:"min_payout"`
    Version           string  `json:"version"`
}

// Payout представляет выплату фермеру
type Payout struct {
    LauncherID string    `json:"launcher_id"`
    Amount     uint64    `json:"amount"`
    Timestamp  time.Time `json:"timestamp"`
    TxID       string    `json:"tx_id,omitempty"`
    Status     string    `json:"status"` // pending, processed, failed
}

// DifficultyParams параметры для расчета сложности
type DifficultyParams struct {
    TargetPartialsPerDay uint64 `json:"target_partials_per_day"`
    CurrentDifficulty    uint64 `json:"current_difficulty"`
    FarmerPoints24h      uint64 `json:"farmer_points_24h"`
    TimeSinceLastPartial uint64 `json:"time_since_last_partial"`
    MinDifficulty        uint32 `json:"min_difficulty"`
    MaxDifficulty        uint32 `json:"max_difficulty"`
}

// PoolConfig конфигурация пула
type PoolConfig struct {
    PoolName          string  `json:"pool_name"`
    PoolURL           string  `json:"pool_url"`
    Port              uint16  `json:"port"`
    PoolFee           float64 `json:"pool_fee"`
    MinPayout         uint64  `json:"min_payout"`
    PartialDeadline   uint32  `json:"partial_deadline"`
    DifficultyTarget  uint32  `json:"difficulty_target"`
    NodeRPCHost       string  `json:"node_rpc_host"`
    NodeRPCPort       uint16  `json:"node_rpc_port"`
    NodeRPCCertPath   string  `json:"node_rpc_cert_path"`
    NodeRPCKeyPath    string  `json:"node_rpc_key_path"`
}

// Convert C FarmerInfo to Go Farmer
func farmerFromC(cFarmer *C.FarmerInfo) Farmer {
    return Farmer{
        LauncherID: C.GoString(&cFarmer.launcher_id[0]),
        PoolURL:    C.GoString(&cFarmer.pool_url[0]),
        Difficulty: uint64(cFarmer.difficulty),
        Points:     uint64(cFarmer.points),
        Partials:   uint64(cFarmer.partials),
        Timestamp:  time.Unix(int64(cFarmer.timestamp), 0),
    }
}

// Convert Go Farmer to C FarmerInfo
func farmerToC(farmer Farmer) C.FarmerInfo {
    var cFarmer C.FarmerInfo
    C.strcpy(&cFarmer.launcher_id[0], C.CString(farmer.LauncherID))
    C.strcpy(&cFarmer.pool_url[0], C.CString(farmer.PoolURL))
    cFarmer.difficulty = C.uint64_t(farmer.Difficulty)
    cFarmer.points = C.uint64_t(farmer.Points)
    cFarmer.partials = C.uint64_t(farmer.Partials)
    cFarmer.timestamp = C.uint64_t(farmer.Timestamp.Unix())
    return cFarmer
}

// Convert C PoolInfo to Go PoolInfo
func poolInfoFromC(cPoolInfo *C.PoolInfo) PoolInfo {
    return PoolInfo{
        PoolName:          C.GoString(&cPoolInfo.pool_name[0]),
        PoolURL:           C.GoString(&cPoolInfo.pool_url[0]),
        TotalFarmers:      uint64(cPoolInfo.total_farmers),
        TotalNetspace:     uint64(cPoolInfo.total_netspace),
        CurrentDifficulty: uint64(cPoolInfo.current_difficulty),
        PoolFee:           float64(cPoolInfo.pool_fee),
        MinPayout:         uint64(cPoolInfo.min_payout),
    }
}

// Convert Go PartialRequest to C PartialRequest
func partialRequestToC(partial PartialRequest) C.PartialRequest {
    var cPartial C.PartialRequest
    C.strcpy(&cPartial.challenge[0], C.CString(partial.Challenge))
    C.strcpy(&cPartial.launcher_id[0], C.CString(partial.LauncherID))
    C.strcpy(&cPartial.signature[0], C.CString(partial.Signature))
    cPartial.timestamp = C.uint64_t(partial.Timestamp)
    cPartial.difficulty = C.uint64_t(partial.Difficulty)
    return cPartial
}

// HexToBytes конвертирует hex строку в байты
func HexToBytes(hexStr string) ([]byte, error) {
    return hex.DecodeString(hexStr)
}

// BytesToHex конвертирует байты в hex строку
func BytesToHex(data []byte) string {
    return hex.EncodeToString(data)
}

// ValidateLauncherID проверяет валидность Launcher ID
func ValidateLauncherID(launcherID string) bool {
    if len(launcherID) != 64 {
        return false
    }
    _, err := HexToBytes(launcherID)
    return err == nil
}

// ValidateSignature проверяет валидность подписи
func ValidateSignature(signature string) bool {
    if len(signature) != 192 {
        return false
    }
    _, err := HexToBytes(signature)
    return err == nil
}

// ValidateChallenge проверяет валидность challenge
func ValidateChallenge(challenge string) bool {
    if len(challenge) != 64 {
        return false
    }
    _, err := HexToBytes(challenge)
    return err == nil
}
