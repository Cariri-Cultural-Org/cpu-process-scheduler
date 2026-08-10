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

.PHONY: all clean run

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
