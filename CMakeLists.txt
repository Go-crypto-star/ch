cmake_minimum_required(VERSION 3.10)
project(ChiaPool C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# Настройки компиляции
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fPIC")

# Поиск зависимостей
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(OPENSSL REQUIRED openssl)
pkg_check_modules(GMP REQUIRED gmp)

# Включаемые директории
include_directories(include)
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIRS})
include_directories(${GMP_INCLUDE_DIRS})

# Источники
file(GLOB C_SOURCES 
    "src/*.c"
    "src/protocol/*.c" 
    "src/blockchain/*.c"
    "src/security/*.c"
)

file(GLOB CPP_SOURCES
    "src/*.cpp"
    "src/protocol/*.cpp"
    "src/blockchain/*.cpp" 
    "src/security/*.cpp"
)

# Библиотеки
add_library(pool SHARED ${C_SOURCES} ${CPP_SOURCES})
add_library(pool_static STATIC ${C_SOURCES} ${CPP_SOURCES})

# Зависимости
target_link_libraries(pool ${CURL_LIBRARIES} ${OPENSSL_LIBRARIES} ${GMP_LIBRARIES} pthread m)
target_link_libraries(pool_static ${CURL_LIBRARIES} ${OPENSSL_LIBRARIES} ${GMP_LIBRARIES} pthread m)

# Установка
install(TARGETS pool pool_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Тесты
if(EXISTS "${CMAKE_SOURCE_DIR}/tests")
    enable_testing()
    add_subdirectory(tests)
endif()

# Go компоненты
if(EXISTS "${CMAKE_SOURCE_DIR}/go")
    add_custom_target(go_build
        COMMAND go build -buildmode=c-shared -o ${CMAKE_BINARY_DIR}/libpool.go ./go
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Building Go components"
    )
endif()
