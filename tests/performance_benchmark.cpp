#include <benchmark/benchmark.h>
#include "math_operations.h"
#include "security/auth.h"
#include "security/proof_verification.h"
#include "protocol/partials.h"
#include <random>

extern "C" {
    // Объявления ассемблерных функций
    void sha256_extended_x64(const uint8_t* input, size_t length, uint8_t* output);
    void bls_verify_batch_avx2(const uint8_t** public_keys, const uint8_t** messages, 
                              const uint8_t** signatures, size_t count, bool* results);
    void process_partials_batch_avx2(partial_t** partials, size_t count, uint64_t* points);
}

class PerformanceBenchmark : public benchmark::Fixture {
protected:
    void SetUp(const benchmark::State& state) override {
        math_operations_init();
        
        bls_key_t test_key;
        memset(&test_key, 0, sizeof(bls_key_t));
        auth_init(&test_key);
        proof_verification_init();
    }

    void TearDown(const benchmark::State& state) override {
        auth_cleanup();
        proof_verification_cleanup();
    }
    
    std::vector<uint8_t> GenerateRandomData(size_t size) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (size_t i = 0; i < size; i++) {
            data[i] = static_cast<uint8_t>(dis(gen));
        }
        return data;
    }
};

BENCHMARK_F(PerformanceBenchmark, SHA256Extended)(benchmark::State& state) {
    auto input_data = GenerateRandomData(1024); // 1KB данных
    uint8_t output[32];
    
    for (auto _ : state) {
        sha256_extended_x64(input_data.data(), input_data.size(), output);
        benchmark::DoNotOptimize(output);
    }
    
    state.SetBytesProcessed(state.iterations() * input_data.size());
}

BENCHMARK_F(PerformanceBenchmark, BLSVerifyBatch)(benchmark::State& state) {
    const size_t batch_size = state.range(0);
    
    // Подготавливаем тестовые данные
    std::vector<const uint8_t*> public_keys(batch_size);
    std::vector<const uint8_t*> messages(batch_size);
    std::vector<const uint8_t*> signatures(batch_size);
    std::vector<bool> results(batch_size);
    
    auto test_data = GenerateRandomData(48 + 32 + 96); // pubkey + message + signature
    
    for (size_t i = 0; i < batch_size; i++) {
        public_keys[i] = test_data.data();
        messages[i] = test_data.data() + 48;
        signatures[i] = test_data.data() + 48 + 32;
    }
    
    for (auto _ : state) {
        bls_verify_batch_avx2(public_keys.data(), messages.data(), 
                             signatures.data(), batch_size, results.data());
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}

BENCHMARK_F(PerformanceBenchmark, ProcessPartialsBatch)(benchmark::State& state) {
    const size_t batch_size = state.range(0);
    
    // Подготавливаем тестовые partials
    std::vector<partial_t*> partials(batch_size);
    std::vector<partial_t> partial_data(batch_size);
    std::vector<uint64_t> points(batch_size);
    
    for (size_t i = 0; i < batch_size; i++) {
        memset(&partial_data[i], 0, sizeof(partial_t));
        partial_data[i].timestamp = time(NULL);
        partial_data[i].difficulty = 1000 + i;
        partials[i] = &partial_data[i];
    }
    
    for (auto _ : state) {
        process_partials_batch_avx2(partials.data(), batch_size, points.data());
        benchmark::DoNotOptimize(points);
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}

BENCHMARK_F(PerformanceBenchmark, MathCalculatePoints)(benchmark::State& state) {
    const size_t iterations = state.range(0);
    
    for (auto _ : state) {
        uint64_t points = math_calculate_points(1000, iterations);
        benchmark::DoNotOptimize(points);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(PerformanceBenchmark, MathCalculateDifficulty)(benchmark::State& state) {
    difficulty_params_t params = {
        .target_partials_per_day = 300,
        .current_difficulty = 1000,
        .farmer_points_24h = 150000,
        .time_since_last_partial = 3600,
        .min_difficulty = 100,
        .max_difficulty = 10000
    };
    
    for (auto _ : state) {
        uint64_t difficulty = math_calculate_difficulty(&params);
        benchmark::DoNotOptimize(difficulty);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(PerformanceBenchmark, ProofVerification)(benchmark::State& state) {
    uint8_t proof_data[368];
    memset(proof_data, 0xAA, sizeof(proof_data)); // Заполняем тестовыми данными
    
    proof_verification_params_t params = {
        .challenge = 123456789,
        .k_size = 32,
        .sub_slot_iters = 37600000000ULL,
        .difficulty = 1000,
        .required_iterations = 0
    };
    
    proof_metadata_t metadata;
    
    for (auto _ : state) {
        proof_verification_result_t result = proof_verify_space(
            proof_data, sizeof(proof_data), &params, &metadata);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(PerformanceBenchmark, BLSSignatureVerification)(benchmark::State& state) {
    uint8_t public_key[48] = {0};
    uint8_t message[32] = {0};
    uint8_t signature[96] = {0};
    
    for (auto _ : state) {
        bool result = auth_bls_verify_signature(public_key, message, sizeof(message), signature);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(PerformanceBenchmark, PartialValidation)(benchmark::State& state) {
    partial_t partial;
    memset(&partial, 0, sizeof(partial_t));
    partial.timestamp = time(NULL);
    
    for (auto _ : state) {
        partial_validation_result_t result = partial_validate(&partial);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(PerformanceBenchmark, PayoutCalculation)(benchmark::State& state) {
    payout_calculation_params_t params = {
        .total_points = 1000000,
        .pool_points = 10000,
        .farmer_points = 50000,
        .pool_fee_percentage = 0.01,
        .block_rewards = 1750000000000,
        .total_netspace = 1000000000000000,
        .farmer_netspace = 1000000000000
    };
    
    for (auto _ : state) {
        payout_calculation_result_t result = math_calculate_payout(&params);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Регистрируем бенчмарки с параметрами
BENCHMARK_REGISTER_F(PerformanceBenchmark, BLSVerifyBatch)
    ->Arg(1)->Arg(4)->Arg(8)->Arg(16)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(PerformanceBenchmark, ProcessPartialsBatch)
    ->Arg(1)->Arg(8)->Arg(32)->Arg(64)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(PerformanceBenchmark, MathCalculatePoints)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)
    ->Unit(benchmark::kNanosecond);

// Основная функция
BENCHMARK_MAIN();
