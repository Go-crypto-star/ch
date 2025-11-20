CC = gcc
CXX = g++
GO = go
CFLAGS = -O3 -march=native -fPIC -Wall -Wextra
CXXFLAGS = -O3 -march=native -fPIC -std=c++17 -Wall -Wextra
LDFLAGS = -ljsoncpp -lm -lpthread

# Цели сборки
TARGET = libpool.so
GO_TARGET = pool.a

# Исходные файлы
C_SOURCES = pool_math.c hash_utils.c
CXX_SOURCES = pool_manager.cpp state_manager.cpp
ASM_SOURCES = math_ops.asm
GO_SOURCES = config_loader.go crypto_utils.go

# Объектные файлы
C_OBJS = $(C_SOURCES:.c=.o)
CXX_OBJS = $(CXX_SOURCES:.cpp=.o)
ASM_OBJS = $(ASM_SOURCES:.asm=.o)
GO_OBJS = $(GO_SOURCES:.go=.o)

all: $(TARGET) $(GO_TARGET)

# Сборка основной C++ библиотеки
$(TARGET): $(C_OBJS) $(CXX_OBJS) $(ASM_OBJS)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

# Сборка Go пакета
$(GO_TARGET): $(GO_SOURCES)
	$(GO) build -buildmode=c-archive -o $@ ./...

# Компиляция C файлов
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция C++ файлов  
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Компиляция ассемблера
%.o: %.asm
	nasm -f elf64 $< -o $@

# Очистка
clean:
	rm -f *.o $(TARGET) $(GO_TARGET)

.PHONY: all clean
