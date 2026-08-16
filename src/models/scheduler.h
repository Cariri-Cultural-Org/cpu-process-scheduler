#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>

#include "process/process.h"

/* =====================================================================
 * scheduler.h
 * =====================================================================
 *
 * Frente responsável: definido em conjunto (núcleo da simulação +
 * algoritmos de escalonamento).
 *
 * Objetivo:
 *   Definir o contrato comum que todos os algoritmos de escalonamento
 *   em src/algorithms/ devem seguir para poderem ser plugados no
 *   núcleo da simulação (src/simulator.c) de forma intercambiável.
 *
 * O que deve ser definido aqui:
 *   - Forma de identificar/selecionar o algoritmo a usar em uma
 *     execução (ex.: enum com FCFS, ROUND_ROBIN, PRIORIDADE, PROPRIO)
 *   - Assinatura da função que cada algoritmo implementa para decidir
 *     qual processo (da fila de prontos) executa em seguida
 *   - Como parâmetros específicos de cada algoritmo são passados
 *     (ex.: quantum do Round Robin)
 *
 * Requisitos do enunciado relacionados:
 *   - Seção 8 (Algoritmos de escalonamento) — ver docs/PROJETO.md
 *
 * Convenção adotada:
 *   As funções de escalonamento recebem a fila de prontos como um
 *   vetor de Process e retornam o índice do processo escolhido. Caso
 *   não exista processo pronto, retornam SCHEDULER_NO_PROCESS.
 * ===================================================================== */

#define SCHEDULER_NO_PROCESS (-1)

typedef enum {
    SCHEDULER_FCFS = 0,
    SCHEDULER_ROUND_ROBIN,
    SCHEDULER_PRIORITY,
    SCHEDULER_CUSTOM
} SchedulerAlgorithm;

typedef struct {
    int quantum;
    int context_switch_cost;
} SchedulerConfig;

typedef int (*SchedulerSelectFn)(
    Process *ready_queue,
    size_t ready_count,
    int current_time,
    const SchedulerConfig *config
);

int scheduler_select_fcfs(
    Process *ready_queue,
    size_t ready_count,
    int current_time,
    const SchedulerConfig *config
);

int scheduler_select_round_robin(
    Process *ready_queue,
    size_t ready_count,
    int current_time,
    const SchedulerConfig *config
);

int scheduler_round_robin_should_preempt(
    int elapsed_in_current_quantum,
    const SchedulerConfig *config
);

int scheduler_select_priority(
    Process *ready_queue,
    size_t ready_count,
    int current_time,
    const SchedulerConfig *config
);

/* Algoritmo próprio (SJF Preditivo com Aging) — ver docs/MODELAGEM.md,
 * seção 6, e src/algorithms/predictive_sjf.c. */
int scheduler_select_predictive_sjf(
    Process *ready_queue,
    size_t ready_count,
    int current_time,
    const SchedulerConfig *config
);

/* Zera o estado interno do SJF Preditivo (histórico global e por
 * processo). Necessário entre execuções que reutilizem o mesmo
 * processo do SO (ex.: suíte de testes); não é preciso em execuções
 * normais do binário (uma simulação por processo). */
void scheduler_predictive_sjf_reset(void);

#endif /* SCHEDULER_H */
