#!/bin/bash

# Запуск всех тестов
echo "Running C++ tests..."
./build/math_tests
./build/pool_tests  
./build/security_tests

echo "Running Go tests..."
cd go && go test -v ./...

echo "Running integration tests..."
./build/integration_tests

echo "All tests passed!"
