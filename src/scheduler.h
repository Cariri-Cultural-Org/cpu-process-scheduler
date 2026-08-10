#ifndef SCHEDULER_H
#define SCHEDULER_H

/* =====================================================================
 * scheduler.h
 * =====================================================================
 *
 * Frente responsável: definido em conjunto (núcleo da simulação +
 * algoritmos de escalonamento).
 *
 * Objetivo:
 *   Definir o contrato comum que todos os algoritmos de escalonamento
 *   (src/algorithms/*.c) devem seguir para poderem ser plugados no
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
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

#endif /* SCHEDULER_H */
