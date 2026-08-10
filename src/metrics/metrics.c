/* =====================================================================
 * metrics/metrics.c
 * =====================================================================
 *
 * Frente responsável: Cícero Jesus (métricas, estatística e
 * visualização).
 *
 * Objetivo:
 *   Implementar o cálculo das métricas declaradas em metrics.h, a
 *   partir dos dados brutos de uma execução da simulação.
 *
 * O que deve ser implementado aqui:
 *   - turnaround_i = término_i - chegada_i, e a média sobre todos os
 *     processos
 *   - contagem de trocas de contexto ocorridas durante a simulação
 *   - tempo_mínimo_ideal_i = soma das rajadas de CPU + soma das
 *     rajadas de E/S do processo i
 *   - slowdown_i = turnaround_i / tempo_mínimo_ideal_i
 *   - índice de Jain do slowdown:
 *       Jain(%) = (soma(slowdown_i))^2 / (n * soma(slowdown_i^2)) * 100
 *   - gravação dos resultados em um formato consumível pelos scripts
 *     de consolidação/gráficos (ex.: CSV)
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

/* #include "metrics.h" */
