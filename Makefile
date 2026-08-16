# =====================================================================
# Makefile - Simulador de Escalonamento de Processos
# =====================================================================
# Ponto de partida; ajustar conforme os arquivos-fonte forem sendo
# criados em src/.
#
# Uso:
#   make            -> compila o simulador (binário em bin/simulador)
#   make clean      -> remove artefatos de build
#   make run        -> compila e executa o simulador
#   make test       -> compila e roda os testes (Unity, tests/)
#
# TODO: revisar flags de otimização/depuração conforme necessário.
# =====================================================================

CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O2 -Isrc
SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
TARGET := $(BIN_DIR)/simulador

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# ---- Testes (Unity, adicionado como submodule em tests/unity) ----
UNITY_DIR := tests/unity/src
TEST_DIR := tests

# Um binário por arquivo tests/test_*.c (cada um com seu próprio main
# via UNITY_BEGIN/END) para não colidir símbolos entre arquivos de teste.
TEST_SRCS := $(shell find $(TEST_DIR) -maxdepth 1 -name 'test_*.c')
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SRCS))
UNITY_SRC := $(UNITY_DIR)/unity.c

TESTED_SRCS := $(filter-out $(SRC_DIR)/main.c,$(SRCS))

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

run: all
	./$(TARGET)

test: $(TEST_BINS)
	@for t in $(TEST_BINS); do ./$$t || exit 1; done

$(BIN_DIR)/%: $(TEST_DIR)/%.c $(UNITY_SRC) $(TESTED_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(UNITY_DIR) -o $@ $< $(UNITY_SRC) $(TESTED_SRCS) -lm
