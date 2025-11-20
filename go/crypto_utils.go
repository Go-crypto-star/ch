package main

import (
    "crypto/sha256"
    "encoding/hex"
    "fmt"
    "hash"
)

// CryptoUtils предоставляет криптографические утилиты
type CryptoUtils struct {
    sha256Hash hash.Hash
}

// NewCryptoUtils создает новый экземпляр CryptoUtils
func NewCryptoUtils() *CryptoUtils {
    return &CryptoUtils{
        sha256Hash: sha256.New(),
    }
}

// ComputeSHA256 вычисляет SHA256 хеш от данных
func (cu *CryptoUtils) ComputeSHA256(data []byte) ([]byte, error) {
    cu.sha256Hash.Reset()
    _, err := cu.sha256Hash.Write(data)
    if err != nil {
        return nil, fmt.Errorf("failed to compute SHA256: %v", err)
    }
    return cu.sha256Hash.Sum(nil), nil
}

// ComputeSHA256Hex вычисляет SHA256 хеш и возвращает его в hex строке
func (cu *CryptoUtils) ComputeSHA256Hex(data []byte) (string, error) {
    hash, err := cu.ComputeSHA256(data)
    if err != nil {
        return "", err
    }
    return hex.EncodeToString(hash), nil
}

// ValidateProofHash проверяет хеш доказательства (упрощенная версия)
func (cu *CryptoUtils) ValidateProofHash(proofHash string, challenge string, difficulty uint64) bool {
    // В реальной реализации здесь будет сложная логика проверки proof of space
    // Для примера делаем простую проверку длины
    return len(proofHash) == 64 && len(challenge) == 64
}

// VerifySignature проверяет BLS подпись (заглушка)
func (cu *CryptoUtils) VerifySignature(publicKey []byte, message []byte, signature []byte) bool {
    // В реальной реализации здесь будет проверка BLS подписи
    // Для примера возвращаем true
    return true
}

// GenerateChallenge генерирует challenge для фермера
func (cu *CryptoUtils) GenerateChallenge() (string, error) {
    // Генерируем случайный challenge
    randomData := []byte(fmt.Sprintf("challenge-%d", time.Now().UnixNano()))
    return cu.ComputeSHA256Hex(randomData)
}

// ValidateLauncherID проверяет валидность Launcher ID
func (cu *CryptoUtils) ValidateLauncherID(launcherID string) bool {
    if len(launcherID) != 64 {
        return false
    }
    _, err := hex.DecodeString(launcherID)
    return err == nil
}

// ValidatePlotID проверяет валидность Plot ID
func (cu *CryptoUtils) ValidatePlotID(plotID string) bool {
    if len(plotID) != 64 {
        return false
    }
    _, err := hex.DecodeString(plotID)
    return err == nil
}

// HashData хеширует данные с использованием SHA256
func (cu *CryptoUtils) HashData(data []byte) []byte {
    hash, _ := cu.ComputeSHA256(data)
    return hash
}

// HashDataHex хеширует данные и возвращает hex строку
func (cu *CryptoUtils) HashDataHex(data []byte) string {
    hash, _ := cu.ComputeSHA256Hex(data)
    return hash
}

// DoubleSHA256 вычисляет двойной SHA256 хеш
func (cu *CryptoUtils) DoubleSHA256(data []byte) ([]byte, error) {
    firstHash, err := cu.ComputeSHA256(data)
    if err != nil {
        return nil, err
    }
    return cu.ComputeSHA256(firstHash)
}

// DoubleSHA256Hex вычисляет двойной SHA256 и возвращает hex строку
func (cu *CryptoUtils) DoubleSHA256Hex(data []byte) (string, error) {
    hash, err := cu.DoubleSHA256(data)
    if err != nil {
        return "", err
    }
    return hex.EncodeToString(hash), nil
}
