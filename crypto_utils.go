package main

import (
	"crypto/sha256"
	"encoding/hex"
	"golang.org/x/crypto/sha3"
)

// CryptoUtils предоставляет криптографические функции
type CryptoUtils struct{}

// Keccak256 вычисляет хеш Keccak-256
func (cu *CryptoUtils) Keccak256(data []byte) []byte {
	hash := sha3.NewLegacyKeccak256()
	hash.Write(data)
	return hash.Sum(nil)
}

// Sha256 вычисляет хеш SHA-256
func (cu *CryptoUtils) Sha256(data []byte) []byte {
	hash := sha256.Sum256(data)
	return hash[:]
}

// ValidateAddress проверяет валидность адреса
func (cu *CryptoUtils) ValidateAddress(address string) bool {
	if len(address) != 42 || address[:2] != "0x" {
		return false
	}
	
	_, err := hex.DecodeString(address[2:])
	return err == nil
}

// ComputePoolAddress вычисляет адрес пула
func (cu *CryptoUtils) ComputePoolAddress(factory, token0, token1 string, salt uint64) string {
	// Реализация аналогичная Solidity CREATE2
	factoryBytes, _ := hex.DecodeString(factory[2:])
	token0Bytes, _ := hex.DecodeString(token0[2:])
	token1Bytes, _ := hex.DecodeString(token1[2:])
	
	// Формирование данных для хеширования
	data := append([]byte{0xff})
	data = append(data, factoryBytes...)
	// ... полная реализация CREATE2
	
	hash := cu.Keccak256(data)
	return "0x" + hex.EncodeToString(hash[12:32])
}
