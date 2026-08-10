#ifndef METRICS_H
#define METRICS_H

/* =====================================================================
 * metrics/metrics.h
 * =====================================================================
 *
 * Frente responsável: Cícero Jesus (métricas, estatística e
 * visualização).
 *
 * Objetivo:
 *   Declarar a interface de cálculo das métricas obrigatórias a
 *   partir dos dados brutos produzidos por uma execução da simulação.
 *
 * O que deve ser definido aqui:
 *   - Assinatura das funções de cálculo de:
 *       - Turnaround médio
 *       - Número de trocas de contexto
 *       - Slowdown por processo (turnaround / tempo mínimo ideal)
 *       - Índice de Jain aplicado ao slowdown (métrica de justiça)
 *   - Formato de saída dos resultados de uma execução (ex.: uma linha
 *     por execução, identificando cenário, seed, algoritmo e valores
 *     das métricas), para ser consumido pelos scripts em scripts/
 *
 * Requisitos do enunciado relacionados:
 *   - Seção 9 (Resultados esperados) — ver docs/PROJETO.md, inclui as
 *     fórmulas de slowdown e do índice de Jain
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

#endif /* METRICS_H */
