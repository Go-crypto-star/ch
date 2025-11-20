#ifndef MATH_OPERATIONS_H
#define MATH_OPERATIONS_H

#include "pool_core.h"

// Ассемблерные математические операции
void asm_u256_add(const uint256_t* a, const uint256_t* b, uint256_t* result);
void asm_u256_sub(const uint256_t* a, const uint256_t* b, uint256_t* result);
void asm_u256_mul(const uint256_t* a, const uint256_t* b, uint256_t* result);
void asm_u256_div(const uint256_t* a, const uint256_t* b, uint256_t* result);
void asm_u256_mod(const uint256_t* a, const uint256_t* b, uint256_t* result);

// Верификация ассемблерных операций
bool verify_asm_operations();

// Безопасные математические операции
bool safe_u256_add(uint256_t* result, const uint256_t* a, const uint256_t* b);
bool safe_u256_sub(uint256_t* result, const uint256_t* a, const uint256_t* b);
bool safe_u256_mul(uint256_t* result, const uint256_t* a, const uint256_t* b);

#endif
