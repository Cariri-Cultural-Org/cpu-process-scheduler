/* =====================================================================
 * algorithms/predictive_sjf.c
 * =====================================================================
 *
 * Frente responsável: Diogo Gomes (algoritmo próprio).
 *
 * SJF Preditivo com Aging — ver docs/MODELAGEM.md, seção 6, para a
 * motivação, as informações usadas e as limitações do algoritmo.
 *
 * Estado interno:
 *   O contrato de scheduler.h só passa a fila de prontos e o instante
 *   atual a cada chamada, sem estado persistente entre chamadas. Este
 *   arquivo mantém, por conta própria (sem alterar process.h nem
 *   simulator.c, que são de outras frentes):
 *     - um acumulador global (soma/contagem de rajadas de CPU já
 *       concluídas por qualquer processo), usado como fallback de
 *       τ(0) quando um processo ainda não tem rajadas concluídas;
 *     - por processo (indexado por pid), o instante em que ele entrou
 *       na fila de prontos na passagem atual, para o termo de aging.
 *   A estimativa τ em si NÃO precisa de estado persistente: Process
 *   já carrega o array completo de rajadas (bursts) e o índice da
 *   rajada atual (current_burst_index), então o histórico de rajadas
 *   de CPU já concluídas de um processo (bursts[0..current_burst_index),
 *   filtrando CPU) é recalculado a cada chamada a partir do próprio
 *   Process recebido — nunca se olha bursts[current_burst_index] (a
 *   rajada ainda não executada), respeitando a restrição de não usar
 *   informação futura.
 *
 * Limitação: a última rajada de CPU de um processo (a que o finaliza)
 * nunca é revisitada pela fila de prontos, então seu valor real nunca
 * é incorporado ao acumulador global nem à própria série do processo.
 * É uma limitação inerente a manter o algoritmo sem hooks no núcleo da
 * simulação; converge à medida que mais processos multi-rajada passam
 * pela fila.
 *
 * α, β e o fallback inicial de τ são centralizados em config.h (ver
 * docs/MODELAGEM.md, seção 6).
 * ===================================================================== */

#include "../models/scheduler.h"
#include "../models/config.h"

#include <stdlib.h>

typedef struct {
    int pid;
    int counted_index;    /* até onde as rajadas deste pid já entraram no acumulador global */
    int last_burst_index; /* current_burst_index observado na última chamada */
    int ready_since;
    int has_ready_since;
} PidState;

static PidState *g_states = NULL;
static size_t g_states_count = 0;
static size_t g_states_capacity = 0;

static double g_completed_sum = 0.0;
static long g_completed_count = 0;

void scheduler_predictive_sjf_reset(void) {
    free(g_states);
    g_states = NULL;
    g_states_count = 0;
    g_states_capacity = 0;
    g_completed_sum = 0.0;
    g_completed_count = 0;
}

static PidState *find_state(int pid) {
    for (size_t i = 0; i < g_states_count; i++) {
        if (g_states[i].pid == pid) {
            return &g_states[i];
        }
    }
    return NULL;
}

static PidState *get_or_create_state(int pid) {
    PidState *existing = find_state(pid);
    if (existing != NULL) {
        return existing;
    }

    if (g_states_count == g_states_capacity) {
        const size_t capacity = g_states_capacity == 0 ? 16 : g_states_capacity * 2;
        PidState *resized = realloc(g_states, capacity * sizeof(PidState));
        if (resized == NULL) {
            exit(EXIT_FAILURE);
        }
        g_states = resized;
        g_states_capacity = capacity;
    }

    PidState *created = &g_states[g_states_count++];
    created->pid = pid;
    created->counted_index = 0;
    created->last_burst_index = -1;
    created->ready_since = 0;
    created->has_ready_since = 0;
    return created;
}

/* Alimenta o acumulador global com as rajadas de CPU concluídas do
 * processo desde a última vez que ele foi visto, avançando
 * state->counted_index. */
static void accumulate_completed_bursts(PidState *state, const Process *process) {
    for (int i = state->counted_index; i < process->current_burst_index; i++) {
        if (process->bursts[i].type == CPU) {
            g_completed_sum += (double)process->bursts[i].duration;
            g_completed_count++;
        }
    }
    state->counted_index = process->current_burst_index;
}

/* τ estimado para a próxima rajada de CPU (ainda não executada) do
 * processo, a partir do histórico de rajadas de CPU já concluídas
 * (bursts[0..current_burst_index), nunca a atual). */
static double estimate_tau(const Process *process) {
    double tau = g_completed_count > 0
        ? g_completed_sum / (double)g_completed_count
        : PREDICTIVE_SJF_DEFAULT_TAU_FALLBACK;

    for (int i = 0; i < process->current_burst_index; i++) {
        if (process->bursts[i].type == CPU) {
            tau = PREDICTIVE_SJF_ALPHA * (double)process->bursts[i].duration + (1.0 - PREDICTIVE_SJF_ALPHA) * tau;
        }
    }

    return tau;
}

static double waiting_time(PidState *state, const Process *process, int current_time) {
    if (!state->has_ready_since || state->last_burst_index != process->current_burst_index) {
        state->ready_since = current_time;
        state->has_ready_since = 1;
        state->last_burst_index = process->current_burst_index;
    }

    return (double)(current_time - state->ready_since);
}

int scheduler_select_predictive_sjf(
    Process *ready_queue,
    size_t ready_count,
    int current_time,
    const SchedulerConfig *config
) {
    (void)config;

    int selected = SCHEDULER_NO_PROCESS;
    double selected_priority = 0.0;

    for (size_t i = 0; i < ready_count; i++) {
        Process *candidate = &ready_queue[i];
        if (candidate->state != READY) {
            continue;
        }

        PidState *state = get_or_create_state(candidate->pid);
        accumulate_completed_bursts(state, candidate);

        const double tau = estimate_tau(candidate);
        const double wait = waiting_time(state, candidate, current_time);
        const double effective_priority = tau - PREDICTIVE_SJF_BETA * wait;

        if (selected == SCHEDULER_NO_PROCESS) {
            selected = (int)i;
            selected_priority = effective_priority;
            continue;
        }

        Process *best = &ready_queue[selected];
        if (effective_priority < selected_priority ||
            (effective_priority == selected_priority &&
             candidate->arrival_time < best->arrival_time) ||
            (effective_priority == selected_priority &&
             candidate->arrival_time == best->arrival_time &&
             candidate->pid < best->pid)) {
            selected = (int)i;
            selected_priority = effective_priority;
        }
    }

    return selected;
}
