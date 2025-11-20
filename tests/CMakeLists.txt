cmake_minimum_required(VERSION 3.10)
project(ChiaPoolTests)

# Поиск Google Test
find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)

# Включаемые директории
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${GTEST_INCLUDE_DIRS})

# Тестовые исполняемые файлы
add_executable(math_test math_test.cpp)
target_link_libraries(math_test GTest::gtest GTest::gtest_main pool_static)

add_executable(security_test security_test.cpp)
target_link_libraries(security_test GTest::gtest GTest::gtest_main pool_static)

add_executable(pool_test pool_test.cpp)
target_link_libraries(pool_test GTest::gtest GTest::gtest_main pool_static)

add_executable(performance_benchmark performance_benchmark.cpp)
target_link_libraries(performance_benchmark benchmark::benchmark pool_static)

# Go интеграционные тесты
add_custom_target(go_integration_test
    COMMAND go test -v integration_test.go
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running Go integration tests"
)

# Добавляем тесты в CTest
include(GoogleTest)
gtest_discover_tests(math_test)
gtest_discover_tests(security_test)
gtest_discover_tests(pool_test)

# Бенчмарки
add_test(NAME performance_benchmark
    COMMAND performance_benchmark
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
