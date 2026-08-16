#ifndef WORKLOAD_GENERATOR_H
#define WORKLOAD_GENERATOR_H

#include <stddef.h>

/* =====================================================================
 * workload/workload_generator.h
 * =====================================================================
 *
 * Frente responsável: Diogo Gomes (algoritmo próprio e geração de
 * carga de trabalho).
 *
 * Gera cargas de trabalho controladas por seed: a mesma seed, no mesmo
 * cenário, sempre produz exatamente a mesma carga (processos, rajadas
 * de CPU, requisições de E/S, prioridades e tempos de chegada). Ver
 * docs/MODELAGEM.md, seções 4 (chegada) e 7 (parâmetros dos cenários),
 * para as decisões de modelagem por trás dos parâmetros usados aqui.
 *
 * DEPENDÊNCIA EM ABERTO: este header assume um tipo `Process` definido
 * em "../process.h" (frente do Alan), hoje ainda esqueleto. Os campos
 * abaixo são a PROPOSTA do Diogo para o que o gerador precisa popular
 * em cada processo — a validar/ajustar em conjunto com quem for
 * implementar process.h:
 *   - id (identificador único do processo)
 *   - tempo de chegada (gerado por processo de Poisson, seção 4 de
 *     MODELAGEM.md)
 *   - prioridade (convenção a definir na seção 1 de MODELAGEM.md)
 *   - sequência de rajadas alternando CPU e E/S (ex.: array de structs
 *     {tipo: CPU|IO, duração}, terminando sempre em rajada de CPU)
 *   - estado inicial = NOVO
 *
 * TODO: incluir "../process.h" assim que a struct Process existir, e
 * usar o tipo Process (em vez de void*) na struct Workload abaixo.
 * ===================================================================== */

/* Os 4 cenários obrigatórios (docs/PROJETO.md, seção 6; parâmetros
 * concretos de cada um em docs/MODELAGEM.md, seção 7). */
typedef enum {
    SCENARIO_BALANCED = 0,     /* 1. Aleatório equilibrado */
    SCENARIO_IO_BOUND,         /* 2. I/O-bound */
    SCENARIO_CPU_BOUND,        /* 3. CPU-bound / processos longos */
    SCENARIO_PRIORITY_SKEWED,  /* 4. Prioridades desbalanceadas */
    SCENARIO_COUNT
} Scenario;

/* Resultado de uma geração: n_processes processos, já ordenados por
 * tempo de chegada, prontos para alimentar o núcleo da simulação
 * (src/simulator.c). `processes` é alocado internamente por
 * generate_workload() e deve ser liberado com free_workload(). */
typedef struct {
    void *processes;   /* TODO: trocar para Process * quando process.h existir */
    size_t count;
} Workload;

/* Gera `n_processes` processos para `scenario`, usando `seed` para
 * inicializar um gerador pseudoaleatório com estado próprio (não usa
 * rand()/srand() globais, para não interferir com geração concorrente
 * de outras seeds/cenários na mesma execução). Determinístico: a mesma
 * tripla (scenario, seed, n_processes) sempre produz a mesma saída. */
Workload generate_workload(Scenario scenario, unsigned long seed, size_t n_processes);

/* Libera a memória alocada por generate_workload(). */
void free_workload(Workload *workload);

/* Nome legível do cenário (para logs e nomes de arquivo em results/raw/). */
const char *scenario_name(Scenario scenario);

#endif /* WORKLOAD_GENERATOR_H */
