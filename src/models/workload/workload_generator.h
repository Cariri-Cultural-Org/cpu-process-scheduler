#ifndef WORKLOAD_GENERATOR_H
#define WORKLOAD_GENERATOR_H

#include <stddef.h>
#include "../process/process.h"

/* Cenários obrigatórios (docs/PROJETO.md, seção 6; parâmetros em
 * docs/MODELAGEM.md, seção 7). */
typedef enum {
    SCENARIO_BALANCED = 0,
    SCENARIO_IO_BOUND,
    SCENARIO_CPU_BOUND,
    SCENARIO_PRIORITY_SKEWED,
    SCENARIO_COUNT
} Scenario;

typedef struct {
    Process *processes;
    size_t count;
} Workload;

/* Determinístico: mesma (scenario, seed, n_processes) sempre gera a
 * mesma carga. processes é alocado internamente; liberar com
 * free_workload(). */
Workload generate_workload(Scenario scenario, unsigned long seed, size_t n_processes);
void free_workload(Workload *workload);
const char *scenario_name(Scenario scenario);

#endif /* WORKLOAD_GENERATOR_H */
