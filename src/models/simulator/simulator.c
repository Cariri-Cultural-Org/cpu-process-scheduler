/* =====================================================================
 * simulator.c
 * =====================================================================
 *
 * Frente responsável: Alan Mendes (núcleo da simulação).
 *
 * Implementa o laço principal descrito em simulator.h: avanço do tempo
 * por eventos, chegada dos processos, bloqueio e retorno da E/S,
 * preempção por quantum, custo de troca de contexto e registro dos
 * dados brutos consumidos por src/models/metrics/.
 * ===================================================================== */

#include "simulator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16

static void *checked_alloc(size_t size) {
    void *block = malloc(size);
    if (block == NULL) {
        fprintf(stderr, "Erro: não foi possível alocar memória para a simulação.\n");
        exit(EXIT_FAILURE);
    }
    return block;
}

static void *checked_realloc(void *block, size_t size) {
    void *resized = realloc(block, size);
    if (resized == NULL) {
        fprintf(stderr, "Erro: não foi possível realocar memória para a simulação.\n");
        exit(EXIT_FAILURE);
    }
    return resized;
}

/* ---------------------------------------------------------------------
 * Fila de prontos
 * ------------------------------------------------------------------- */
void ready_queue_init(ReadyQueue *queue) {
    if (queue == NULL) {
        return;
    }

    queue->items = NULL;
    queue->ids = NULL;
    queue->count = 0;
    queue->capacity = 0;
}

void ready_queue_push(ReadyQueue *queue, const Process *process, size_t id) {
    if (queue == NULL || process == NULL) {
        return;
    }

    if (queue->count == queue->capacity) {
        const size_t capacity = queue->capacity == 0
            ? INITIAL_CAPACITY
            : queue->capacity * 2;
        queue->items = checked_realloc(queue->items, capacity * sizeof(Process));
        queue->ids = checked_realloc(queue->ids, capacity * sizeof(size_t));
        queue->capacity = capacity;
    }

    queue->items[queue->count] = *process;
    queue->ids[queue->count] = id;
    queue->count++;
}

bool ready_queue_remove_at(
    ReadyQueue *queue,
    size_t index,
    Process *out_process,
    size_t *out_id
) {
    if (queue == NULL || index >= queue->count) {
        return false;
    }

    if (out_process != NULL) {
        *out_process = queue->items[index];
    }
    if (out_id != NULL) {
        *out_id = queue->ids[index];
    }

    /* mantém a ordem de entrada dos demais processos */
    const size_t tail = queue->count - index - 1;
    if (tail > 0) {
        memmove(&queue->items[index], &queue->items[index + 1], tail * sizeof(Process));
        memmove(&queue->ids[index], &queue->ids[index + 1], tail * sizeof(size_t));
    }
    queue->count--;

    return true;
}

void ready_queue_free(ReadyQueue *queue) {
    if (queue == NULL) {
        return;
    }

    free(queue->items);
    free(queue->ids);
    ready_queue_init(queue);
}

/* ---------------------------------------------------------------------
 * Fila de E/S
 * ------------------------------------------------------------------- */
void io_queue_init(IoQueue *queue) {
    if (queue == NULL) {
        return;
    }

    queue->items = NULL;
    queue->count = 0;
    queue->capacity = 0;
}

void io_queue_push(IoQueue *queue, size_t id, int release_time) {
    if (queue == NULL) {
        return;
    }

    if (queue->count == queue->capacity) {
        const size_t capacity = queue->capacity == 0
            ? INITIAL_CAPACITY
            : queue->capacity * 2;
        queue->items = checked_realloc(queue->items, capacity * sizeof(BlockedProcess));
        queue->capacity = capacity;
    }

    /* inserção ordenada por (release_time, id) */
    size_t position = queue->count;
    while (position > 0) {
        const BlockedProcess *previous = &queue->items[position - 1];
        if (previous->release_time < release_time ||
            (previous->release_time == release_time && previous->id <= id)) {
            break;
        }
        queue->items[position] = queue->items[position - 1];
        position--;
    }

    queue->items[position].id = id;
    queue->items[position].release_time = release_time;
    queue->count++;
}

bool io_queue_pop_released(IoQueue *queue, int now, BlockedProcess *out) {
    if (queue == NULL || queue->count == 0 || queue->items[0].release_time > now) {
        return false;
    }

    if (out != NULL) {
        *out = queue->items[0];
    }

    memmove(&queue->items[0], &queue->items[1], (queue->count - 1) * sizeof(BlockedProcess));
    queue->count--;

    return true;
}

void io_queue_free(IoQueue *queue) {
    if (queue == NULL) {
        return;
    }

    free(queue->items);
    io_queue_init(queue);
}

/* ---------------------------------------------------------------------
 * Estado interno da simulação
 * ------------------------------------------------------------------- */
typedef struct {
    const SimulationConfig *config;
    SchedulerSelectFn select;

    Process *pool;            /* estado vivo de cada processo da carga */
    int *burst_remaining;     /* tempo restante da rajada de CPU atual */
    size_t *arrival_order;    /* ids ordenados por (chegada, pid) */
    ProcessMetricsSample *samples;
    size_t count;

    ReadyQueue ready;
    IoQueue blocked;

    size_t next_arrival;
    size_t finished;
    int now;
    int last_pid;
    bool cpu_used;

    size_t context_switches;
    int total_time;
    int cpu_busy_time;
    int context_switch_time;
    int idle_time;
} Simulation;

typedef struct {
    int arrival_time;
    int pid;
    size_t id;
} ArrivalEntry;

static int compare_arrivals(const void *left, const void *right) {
    const ArrivalEntry *a = (const ArrivalEntry *)left;
    const ArrivalEntry *b = (const ArrivalEntry *)right;

    if (a->arrival_time != b->arrival_time) {
        return a->arrival_time < b->arrival_time ? -1 : 1;
    }
    if (a->pid != b->pid) {
        return a->pid < b->pid ? -1 : 1;
    }
    return a->id < b->id ? -1 : (a->id > b->id ? 1 : 0);
}

static SchedulerSelectFn select_function(SchedulerAlgorithm algorithm) {
    switch (algorithm) {
        case SCHEDULER_FCFS:
            return scheduler_select_fcfs;
        case SCHEDULER_ROUND_ROBIN:
            return scheduler_select_round_robin;
        case SCHEDULER_PRIORITY:
            return scheduler_select_priority;
        case SCHEDULER_CUSTOM: /* ainda não implementado (custom_algorithm.c) */
        default:
            return NULL;
    }
}

/* Tempos que dependem apenas da carga: chegada e tempo mínimo ideal
 * (docs/PROJETO.md, seção 9). O término é preenchido ao final. */
static void init_samples(Simulation *sim, const Workload *workload) {
    for (size_t id = 0; id < sim->count; id++) {
        const Process *process = &workload->processes[id];

        double total_cpu_time = 0.0;
        double total_io_time = 0.0;
        for (int index = 0; index < process->num_bursts; index++) {
            if (process->bursts[index].type == CPU) {
                total_cpu_time += process->bursts[index].duration;
            } else {
                total_io_time += process->bursts[index].duration;
            }
        }

        sim->samples[id].arrival_time = (double)process->arrival_time;
        sim->samples[id].completion_time = 0.0;
        sim->samples[id].total_cpu_time = total_cpu_time;
        sim->samples[id].total_io_time = total_io_time;
    }
}

static void init_arrival_order(Simulation *sim) {
    ArrivalEntry *entries = checked_alloc(sim->count * sizeof(ArrivalEntry));

    for (size_t id = 0; id < sim->count; id++) {
        entries[id].arrival_time = sim->pool[id].arrival_time;
        entries[id].pid = sim->pool[id].pid;
        entries[id].id = id;
    }

    qsort(entries, sim->count, sizeof(ArrivalEntry), compare_arrivals);

    for (size_t position = 0; position < sim->count; position++) {
        sim->arrival_order[position] = entries[position].id;
    }

    free(entries);
}

static void simulation_init(
    Simulation *sim,
    const Workload *workload,
    const SimulationConfig *config
) {
    sim->config = config;
    sim->select = select_function(config->algorithm);
    sim->count = workload->count;

    sim->pool = checked_alloc(sim->count * sizeof(Process));
    sim->burst_remaining = checked_alloc(sim->count * sizeof(int));
    sim->arrival_order = checked_alloc(sim->count * sizeof(size_t));
    sim->samples = checked_alloc(sim->count * sizeof(ProcessMetricsSample));

    for (size_t id = 0; id < sim->count; id++) {
        sim->pool[id] = workload->processes[id];
        /* a carga não é modificada: o simulador trabalha sobre a cópia e
         * sempre parte do estado inicial, para que a mesma carga possa
         * ser reexecutada com outros algoritmos. */
        sim->pool[id].state = NEW;
        sim->pool[id].current_burst_index = 0;
        sim->burst_remaining[id] = 0;
    }

    init_samples(sim, workload);
    init_arrival_order(sim);

    ready_queue_init(&sim->ready);
    io_queue_init(&sim->blocked);

    sim->next_arrival = 0;
    sim->finished = 0;
    sim->now = 0;
    sim->last_pid = 0;
    sim->cpu_used = false;

    sim->context_switches = 0;
    sim->total_time = 0;
    sim->cpu_busy_time = 0;
    sim->context_switch_time = 0;
    sim->idle_time = 0;
}

static void simulation_free(Simulation *sim) {
    free(sim->pool);
    free(sim->burst_remaining);
    free(sim->arrival_order);
    ready_queue_free(&sim->ready);
    io_queue_free(&sim->blocked);
}

static void finish_process(Simulation *sim, size_t id) {
    set_state(&sim->pool[id], FINISHED);
    sim->samples[id].completion_time = (double)sim->now;
    sim->finished++;
    sim->total_time = sim->now;
}

/* Encaminha o processo conforme a rajada atual: sem rajadas pendentes
 * finaliza; rajada de E/S bloqueia; rajada de CPU vai para a fila de
 * prontos. Usada na chegada, ao fim de cada rajada de CPU e ao fim de
 * cada E/S. */
static void start_current_burst(Simulation *sim, size_t id) {
    Process *process = &sim->pool[id];

    if (process->current_burst_index >= process->num_bursts) {
        finish_process(sim, id);
        return;
    }

    const Burst *burst = &process->bursts[process->current_burst_index];
    if (burst->type == IO) {
        set_state(process, BLOCKED);
        io_queue_push(&sim->blocked, id, sim->now + burst->duration);
        return;
    }

    sim->burst_remaining[id] = burst->duration;
    set_state(process, READY);
    ready_queue_push(&sim->ready, process, id);
}

static void release_finished_io(Simulation *sim) {
    BlockedProcess released;
    while (io_queue_pop_released(&sim->blocked, sim->now, &released)) {
        sim->pool[released.id].current_burst_index++;
        start_current_burst(sim, released.id);
    }
}

static void admit_arrivals(Simulation *sim) {
    while (sim->next_arrival < sim->count) {
        const size_t id = sim->arrival_order[sim->next_arrival];
        if (sim->pool[id].arrival_time > sim->now) {
            break;
        }
        sim->next_arrival++;
        start_current_burst(sim, id);
    }
}

/* Ordem de entrada na fila de prontos em um mesmo instante: primeiro os
 * retornos de E/S (já estavam no sistema), depois as novas chegadas. */
static void advance_pending_events(Simulation *sim) {
    release_finished_io(sim);
    admit_arrivals(sim);
}

/* Próximo instante em que algo pode acontecer com a CPU ociosa; -1 se
 * não há mais nada previsto. */
static int next_event_time(const Simulation *sim) {
    int next = -1;

    if (sim->next_arrival < sim->count) {
        next = sim->pool[sim->arrival_order[sim->next_arrival]].arrival_time;
    }
    if (sim->blocked.count > 0) {
        const int release_time = sim->blocked.items[0].release_time;
        if (next < 0 || release_time < next) {
            next = release_time;
        }
    }

    return next;
}

static void apply_context_switch(Simulation *sim, const Process *process) {
    if (sim->cpu_used && sim->last_pid == process->pid) {
        return;
    }

    sim->context_switches++;
    sim->last_pid = process->pid;
    sim->cpu_used = true;

    const int cost = sim->config->scheduler.context_switch_cost;
    if (cost > 0) {
        sim->now += cost;
        sim->context_switch_time += cost;
    }
}

/* Fatia de CPU concedida ao processo: a rajada inteira, limitada ao
 * quantum quando o algoritmo é preemptivo por tempo. */
static int cpu_slice(const Simulation *sim, size_t id) {
    const int remaining = sim->burst_remaining[id];
    const int quantum = sim->config->scheduler.quantum;

    if (sim->config->algorithm != SCHEDULER_ROUND_ROBIN || quantum <= 0) {
        return remaining;
    }

    return remaining < quantum ? remaining : quantum;
}

static void run_simulation(Simulation *sim) {
    while (sim->finished < sim->count) {
        advance_pending_events(sim);

        if (sim->ready.count == 0) {
            const int next = next_event_time(sim);
            if (next < 0) {
                break; /* defensivo: nada mais a executar */
            }
            if (next > sim->now) {
                sim->idle_time += next - sim->now;
                sim->now = next;
            }
            continue;
        }

        const int index = sim->select(
            sim->ready.items,
            sim->ready.count,
            sim->now,
            &sim->config->scheduler
        );
        if (index < 0 || (size_t)index >= sim->ready.count) {
            break; /* SCHEDULER_NO_PROCESS ou índice inválido */
        }

        Process dispatched;
        size_t id = 0;
        ready_queue_remove_at(&sim->ready, (size_t)index, &dispatched, &id);

        apply_context_switch(sim, &dispatched);
        set_state(&sim->pool[id], EXECUTING);

        const int slice = cpu_slice(sim, id);
        sim->now += slice;
        sim->cpu_busy_time += slice;
        sim->burst_remaining[id] -= slice;

        if (sim->burst_remaining[id] <= 0) {
            sim->pool[id].current_burst_index++;
            advance_pending_events(sim);
            start_current_burst(sim, id);
            continue;
        }

        /* rajada não terminou: só sai da CPU por fim de quantum */
        if (!scheduler_round_robin_should_preempt(slice, &sim->config->scheduler)) {
            break; /* defensivo: não deveria ocorrer com as fatias acima */
        }

        /* o preemptado volta ao fim da fila, atrás de quem ficou pronto
         * neste mesmo instante */
        advance_pending_events(sim);
        set_state(&sim->pool[id], READY);
        ready_queue_push(&sim->ready, &sim->pool[id], id);
    }
}

bool simulator_run(
    const Workload *workload,
    const SimulationConfig *config,
    SimulationResult *result
) {
    if (workload == NULL || workload->processes == NULL || workload->count == 0) {
        return false;
    }
    if (config == NULL || result == NULL) {
        return false;
    }
    if (config->scheduler.context_switch_cost < 0) {
        return false;
    }
    if (config->algorithm == SCHEDULER_ROUND_ROBIN && config->scheduler.quantum <= 0) {
        return false;
    }
    if (select_function(config->algorithm) == NULL) {
        return false;
    }

    Simulation sim;
    simulation_init(&sim, workload, config);
    run_simulation(&sim);

    const SimulationResult simulated = {
        .samples = sim.samples, /* posse transferida ao chamador */
        .count = sim.count,
        .context_switches = sim.context_switches,
        .total_time = sim.total_time,
        .cpu_busy_time = sim.cpu_busy_time,
        .context_switch_time = sim.context_switch_time,
        .idle_time = sim.idle_time,
    };
    *result = simulated;

    simulation_free(&sim);

    return true;
}

void simulator_free_result(SimulationResult *result) {
    if (result == NULL) {
        return;
    }

    free(result->samples);
    result->samples = NULL;
    result->count = 0;
}

const char *simulator_algorithm_name(SchedulerAlgorithm algorithm) {
    switch (algorithm) {
        case SCHEDULER_FCFS: return "fcfs";
        case SCHEDULER_ROUND_ROBIN: return "round_robin";
        case SCHEDULER_PRIORITY: return "priority";
        case SCHEDULER_CUSTOM: return "custom";
        default: return "unknown";
    }
}
