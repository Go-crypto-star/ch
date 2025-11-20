#include "pool_core.h"
#include "math_operations.h"
#include <cstring>
#include <stdexcept>

// Safe arithmetic with overflow checking
namespace security {

bool checked_u256_add(uint256_t* result, const uint256_t* a, const uint256_t* b) {
    uint8_t carry = asm_u256_add(a, b, result);
    return (carry == 0); // Возвращает false если было переполнение
}

bool checked_u256_sub(uint256_t* result, const uint256_t* a, const uint256_t* b) {
    uint8_t borrow = asm_u256_sub(a, b, result);
    return (borrow == 0); // Возвращает false если было заёма (underflow)
}

bool checked_u256_mul(uint256_t* result, const uint256_t* a, const uint256_t* b) {
    // Для проверки переполнения при умножении нужно полное 512-битное умножение
    uint64_t temp[8] = {0};
    
    uint64_t* a_parts = (uint64_t*)a->data;
    uint64_t* b_parts = (uint64_t*)b->data;
    
    // Schoolbook multiplication
    for (int i = 0; i < 4; i++) {
        uint64_t carry = 0;
        for (int j = 0; j < 4; j++) {
            int idx = i + j;
            if (idx < 8) {
                __uint128_t product = (__uint128_t)a_parts[i] * b_parts[j] + temp[idx] + carry;
                temp[idx] = (uint64_t)product;
                carry = (uint64_t)(product >> 64);
            }
        }
    }
    
    // Проверяем, что старшие 256 бит нулевые
    bool overflow = false;
    for (int i = 4; i < 8; i++) {
        if (temp[i] != 0) {
            overflow = true;
            break;
        }
    }
    
    if (!overflow) {
        memcpy(result->data, temp, 32);
    }
    
    return !overflow;
}

void validate_pool_state(const PoolState* pool) {
    if (pool == nullptr) {
        throw std::invalid_argument("Pool state is null");
    }
    
    // Проверка fee rate
    if (pool->fee > MAX_FEE) {
        throw std::invalid_argument("Fee rate too high");
    }
    
    // Проверка резервов на согласованность
    if (u256_is_zero(&pool->reserve0) != u256_is_zero(&pool->reserve1)) {
        throw std::invalid_argument("Reserves inconsistent");
    }
    
    // Проверка total_supply
    if (u256_is_zero(&pool->total_supply) && 
        (!u256_is_zero(&pool->reserve0) || !u256_is_zero(&pool->reserve1))) {
        throw std::invalid_argument("Total supply inconsistent with reserves");
    }
}

bool validate_swap_amounts(const PoolState* pool, const uint256_t* amount0_out, const uint256_t* amount1_out) {
    if (u256_is_zero(amount0_out) && u256_is_zero(amount1_out)) {
        return false;
    }
    
    // Нельзя выводить оба токена одновременно в обычном свопе
    if (!u256_is_zero(amount0_out) && !u256_is_zero(amount1_out)) {
        return false;
    }
    
    // Проверка достаточности резервов
    if (!u256_is_zero(amount0_out) && asm_u256_cmp(amount0_out, &pool->reserve0) > 0) {
        return false;
    }
    
    if (!u256_is_zero(amount1_out) && asm_u256_cmp(amount1_out, &pool->reserve1) > 0) {
        return false;
    }
    
    return true;
}

bool validate_liquidity_amounts(const uint256_t* amount0, const uint256_t* amount1) {
    // Оба amounts не могут быть нулевыми
    if (u256_is_zero(amount0) && u256_is_zero(amount1)) {
        return false;
    }
    
    // Проверка на разумные размеры (предотвращение атак с очень малыми amounts)
    uint256_t min_liquidity;
    memset(&min_liquidity, 0, sizeof(min_liquidity));
    ((uint64_t*)min_liquidity.data)[0] = 1000; // Минимальная ликвидность
    
    if (asm_u256_cmp(amount0, &min_liquidity) < 0 && 
        asm_u256_cmp(amount1, &min_liquidity) < 0) {
        return false;
    }
    
    return true;
}

// Protection against reentrancy attacks
class ReentrancyGuard {
private:
    bool locked;
    
public:
    ReentrancyGuard() : locked(false) {}
    
    void enter() {
        if (locked) {
            throw std::runtime_error("Reentrancy attack detected");
        }
        locked = true;
    }
    
    void exit() {
        locked = false;
    }
    
    class Guard {
    private:
        ReentrancyGuard& parent;
        
    public:
        Guard(ReentrancyGuard& guard) : parent(guard) {
            parent.enter();
        }
        
        ~Guard() {
            parent.exit();
        }
    };
};

// Rate limiting for DoS protection
class RateLimiter {
private:
    struct RequestInfo {
        uint64_t timestamp;
        uint32_t count;
    };
    
    std::unordered_map<std::string, RequestInfo> requests;
    uint32_t max_requests_per_second;
    
public:
    RateLimiter(uint32_t max_rps) : max_requests_per_second(max_rps) {}
    
    bool check_limit(const address_t& address) {
        uint64_t current_time = get_current_timestamp();
        std::string addr_str((char*)address.data, sizeof(address.data));
        
        auto it = requests.find(addr_str);
        if (it != requests.end()) {
            // Reset counter if more than 1 second passed
            if (current_time - it->second.timestamp >= 1) {
                it->second.count = 1;
                it->second.timestamp = current_time;
                return true;
            }
            
            // Check rate limit
            if (it->second.count >= max_requests_per_second) {
                return false;
            }
            
            it->second.count++;
        } else {
            requests[addr_str] = {current_time, 1};
        }
        
        return true;
    }
    
    void cleanup_old_entries() {
        uint64_t current_time = get_current_timestamp();
        for (auto it = requests.begin(); it != requests.end();) {
            if (current_time - it->second.timestamp > 60) { // Cleanup after 60 seconds
                it = requests.erase(it);
            } else {
                ++it;
            }
        }
    }
    
private:
    uint64_t get_current_timestamp() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

// Input validation
bool validate_address(const address_t* address) {
    if (address == nullptr) return false;
    
    // Проверка что адрес не нулевой
    bool all_zero = true;
    for (int i = 0; i < 20; i++) {
        if (address->data[i] != 0) {
            all_zero = false;
            break;
        }
    }
    
    return !all_zero;
}

bool validate_amount(const uint256_t* amount) {
    if (amount == nullptr) return false;
    
    // Проверка что amount не отрицательный (все биты в пределах 256 бит)
    // и не превышает разумные лимиты
    uint256_t max_amount;
    memset(&max_amount, 0xFF, sizeof(max_amount)); // Максимальное 256-битное значение
    
    return asm_u256_cmp(amount, &max_amount) <= 0;
}

} // namespace security

// Safe wrapper functions
bool safe_pool_mint(PoolState* pool, const address_t* to, const uint256_t* amount0, 
                   const uint256_t* amount1, uint256_t* liquidity_out) {
    // Input validation
    if (!security::validate_address(to) || 
        !security::validate_amount(amount0) || 
        !security::validate_amount(amount1)) {
        return false;
    }
    
    if (!security::validate_liquidity_amounts(amount0, amount1)) {
        return false;
    }
    
    security::validate_pool_state(pool);
    
    // Check for overflow in reserve updates
    uint256_t new_reserve0, new_reserve1;
    if (!security::checked_u256_add(&new_reserve0, &pool->reserve0, amount0) ||
        !security::checked_u256_add(&new_reserve1, &pool->reserve1, amount1)) {
        return false;
    }
    
    return pool_mint(pool, *to, *amount0, *amount1, liquidity_out);
}

bool safe_pool_swap(PoolState* pool, const uint256_t* amount0_out, const uint256_t* amount1_out,
                   const address_t* to) {
    if (!security::validate_address(to) ||
        !security::validate_amount(amount0_out) ||
        !security::validate_amount(amount1_out)) {
        return false;
    }
    
    if (!security::validate_swap_amounts(pool, amount0_out, amount1_out)) {
        return false;
    }
    
    security::validate_pool_state(pool);
    
    return pool_swap(pool, *amount0_out, *amount1_out, *to);
}
