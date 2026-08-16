#include "workload_generator.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    uint32_t state;
} Rng;

typedef struct {
    int bursts_min, bursts_max;
    int cpu_min, cpu_max;
    int io_min, io_max;
    int prio_min, prio_max;
    double arrival_rate;
} ScenarioParams;

static const ScenarioParams SCENARIO_PARAMS[SCENARIO_COUNT] = {
    [SCENARIO_BALANCED]        = { 1, 5,  1, 20,  5, 15, 1, 10, 0.5 },
    [SCENARIO_IO_BOUND]        = { 3, 7,  1,  5, 10, 30, 1, 10, 0.5 },
    [SCENARIO_CPU_BOUND]       = { 1, 2, 20, 50,  5, 15, 1, 10, 0.3 },
    [SCENARIO_PRIORITY_SKEWED] = { 1, 5,  1, 20,  5, 15, 1, 10, 0.5 },
};

/* xorshift32: PRNG próprio (não depende de rand_r/libc) para garantir a
 * mesma sequência para a mesma seed em qualquer plataforma. */
static Rng rng_init(unsigned long seed) {
    Rng rng;
    rng.state = (uint32_t)(seed ^ 0x9E3779B9u);
    if (rng.state == 0) rng.state = 1;
    return rng;
}

static uint32_t rng_next(Rng *rng) {
    uint32_t x = rng->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng->state = x;
    return x;
}

static int rng_range(Rng *rng, int min, int max) {
    return min + (int)(rng_next(rng) % (uint32_t)(max - min + 1));
}

static double rng_exponential(Rng *rng, double rate) {
    double u = (double)rng_next(rng) / (double)UINT32_MAX;
    if (u <= 0.0) u = 1e-9;
    return -log(u) / rate;
}

static Burst *build_bursts(Rng *rng, const ScenarioParams *params, int *out_num_bursts) {
    int num_cpu = rng_range(rng, params->bursts_min, params->bursts_max);
    int total = 2 * num_cpu - 1;

    Burst *bursts = malloc(sizeof(Burst) * (size_t)total);
    if (bursts == NULL) {
        fprintf(stderr, "Erro: não foi possível alocar memória para rajadas.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < total; i++) {
        if (i % 2 == 0) {
            bursts[i].type = CPU;
            bursts[i].duration = rng_range(rng, params->cpu_min, params->cpu_max);
        } else {
            bursts[i].type = IO;
            bursts[i].duration = rng_range(rng, params->io_min, params->io_max);
        }
    }

    *out_num_bursts = total;
    return bursts;
}

/* Cenário 4 (docs/MODELAGEM.md, seção 7): 80% prioridade baixa [1,3], 20% alta [8,10]. */
static int generate_priority(Rng *rng, Scenario scenario, const ScenarioParams *params) {
    if (scenario == SCENARIO_PRIORITY_SKEWED) {
        return ((double)rng_next(rng) / (double)UINT32_MAX) < 0.8
            ? rng_range(rng, 1, 3)
            : rng_range(rng, 8, 10);
    }
    return rng_range(rng, params->prio_min, params->prio_max);
}

Workload generate_workload(Scenario scenario, unsigned long seed, size_t n_processes) {
    Workload workload;
    workload.processes = malloc(sizeof(Process) * n_processes);
    if (workload.processes == NULL) {
        fprintf(stderr, "Erro: não foi possível alocar memória para os processos.\n");
        exit(EXIT_FAILURE);
    }
    workload.count = n_processes;

    Rng rng = rng_init(seed ^ (unsigned long)scenario);
    const ScenarioParams *params = &SCENARIO_PARAMS[scenario];
    double clock = 0.0;

    for (size_t i = 0; i < n_processes; i++) {
        clock += rng_exponential(&rng, params->arrival_rate);

        int num_bursts;
        Burst *bursts = build_bursts(&rng, params, &num_bursts);

        Process *p = &workload.processes[i];
        p->pid = (int)i + 1;
        p->arrival_time = (int)clock;
        p->priority = generate_priority(&rng, scenario, params);
        p->state = NEW;
        p->bursts = bursts;
        p->num_bursts = num_bursts;
        p->current_burst_index = 0;
    }

    return workload;
}

void free_workload(Workload *workload) {
    for (size_t i = 0; i < workload->count; i++) {
        free(workload->processes[i].bursts);
    }
    free(workload->processes);
    workload->processes = NULL;
    workload->count = 0;
}

const char *scenario_name(Scenario scenario) {
    switch (scenario) {
        case SCENARIO_BALANCED: return "balanced";
        case SCENARIO_IO_BOUND: return "io_bound";
        case SCENARIO_CPU_BOUND: return "cpu_bound";
        case SCENARIO_PRIORITY_SKEWED: return "priority_skewed";
        default: return "unknown";
    }
}
