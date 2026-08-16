/* =====================================================================
 * algorithms/round_robin.c
 * =====================================================================
 *
 * Frente responsável: Antônio Neto (algoritmos clássicos).
 *
 * Objetivo:
 *   Implementar o algoritmo Round Robin, com quantum configurável.
 *
 * O que deve ser implementado aqui:
 *   - Função que segue o contrato definido em scheduler.h
 *   - Lógica de preempção por tempo (quantum): quando o quantum
 *     expira, o processo volta ao final da fila de prontos
 *   - Parâmetro de quantum configurável (documentar o valor usado nos
 *     experimentos principais em docs/MODELAGEM.md)
 *   - Cuidado: cada troca de processo causada pela expiração do
 *     quantum conta como uma troca de contexto (ver modelagem do
 *     custo de troca de contexto em docs/MODELAGEM.md)
 *
 * Requisitos do enunciado relacionados:
 *   - Seção 8 (Algoritmos de escalonamento) — ver docs/PROJETO.md
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

#include "../models/scheduler.h"

int scheduler_select_round_robin(
    Process *ready_queue,
    size_t ready_count,
    int current_time,
    const SchedulerConfig *config
) {
    (void)current_time;
    (void)config;

    for (size_t i = 0; i < ready_count; i++) {
        if (ready_queue[i].state == READY) {
            return (int)i;
        }
    }

    return SCHEDULER_NO_PROCESS;
}

int scheduler_round_robin_should_preempt(
    int elapsed_in_current_quantum,
    const SchedulerConfig *config
) {
    if (config == NULL || config->quantum <= 0) {
        return 0;
    }

    return elapsed_in_current_quantum >= config->quantum;
}
