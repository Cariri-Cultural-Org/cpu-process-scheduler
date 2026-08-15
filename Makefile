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
TEST_BIN := $(BIN_DIR)/test_runner

TEST_SRCS := $(shell find $(TEST_DIR) -maxdepth 1 -name '*.c')
UNITY_SRC := $(UNITY_DIR)/unity.c

# Módulos do simulador exercitados pelos testes. main.c fica de fora
# porque já define sua própria função main() (conflitaria com a do
# test runner do Unity).
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

test: | $(BIN_DIR)
	$(CC) $(CFLAGS) -I$(UNITY_DIR) -o $(TEST_BIN) $(TEST_SRCS) $(UNITY_SRC) $(TESTED_SRCS) -lm
	./$(TEST_BIN)
