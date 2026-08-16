/* =====================================================================
 * algorithms/fcfs.c
 * =====================================================================
 *
 * Frente responsável: Antônio Neto (algoritmos clássicos).
 *
 * Objetivo:
 *   Implementar o algoritmo First-Come, First-Served (FCFS): o
 *   processo que chega primeiro à fila de prontos é o próximo a ser
 *   executado, sem preempção.
 *
 * O que deve ser implementado aqui:
 *   - Função que segue o contrato definido em scheduler.h
 *   - Critério de desempate quando dois processos chegam no mesmo
 *     instante (definir e documentar em docs/MODELAGEM.md)
 *
 * Requisitos do enunciado relacionados:
 *   - Seção 8 (Algoritmos de escalonamento) — ver docs/PROJETO.md
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

#include "../models/scheduler.h"

int scheduler_select_fcfs(
    Process *ready_queue,
    size_t ready_count,
    int current_time,
    const SchedulerConfig *config
) {
    (void)current_time;
    (void)config;

    int selected = SCHEDULER_NO_PROCESS;

    for (size_t i = 0; i < ready_count; i++) {
        Process *candidate = &ready_queue[i];
        if (candidate->state != READY) {
            continue;
        }

        if (selected == SCHEDULER_NO_PROCESS) {
            selected = (int)i;
            continue;
        }

        Process *best = &ready_queue[selected];
        if (candidate->arrival_time < best->arrival_time ||
            (candidate->arrival_time == best->arrival_time &&
             candidate->pid < best->pid)) {
            selected = (int)i;
        }
    }

    return selected;
}
