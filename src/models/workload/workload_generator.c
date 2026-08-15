/* =====================================================================
 * workload/workload_generator.c
 * =====================================================================
 *
 * Frente responsável: Diogo Gomes (algoritmo próprio e geração de
 * carga de trabalho).
 *
 * Objetivo:
 *   Implementar a geração de processos aleatórios, porém
 *   reprodutíveis via seed, para cada um dos 4 cenários obrigatórios.
 *
 * O que deve ser implementado aqui:
 *   - Inicialização do gerador de números pseudoaleatórios a partir
 *     da seed recebida
 *   - Geração de tempos de chegada conforme o modelo de chegada
 *     definido em docs/MODELAGEM.md (instante 0, intervalos fixos,
 *     aleatórios ou em lotes)
 *   - Geração de rajadas de CPU e de E/S de acordo com o perfil de
 *     cada cenário (equilibrado, I/O-bound, CPU-bound, prioridades
 *     desbalanceadas)
 *   - Geração de prioridades, respeitando a convenção definida em
 *     docs/MODELAGEM.md
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

/* #include "workload_generator.h" */
