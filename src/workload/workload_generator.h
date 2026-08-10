#ifndef WORKLOAD_GENERATOR_H
#define WORKLOAD_GENERATOR_H

/* =====================================================================
 * workload/workload_generator.h
 * =====================================================================
 *
 * Frente responsável: Diogo Gomes (algoritmo próprio e geração de
 * carga de trabalho).
 *
 * Objetivo:
 *   Declarar a interface de geração de cargas de trabalho controladas
 *   por seed: a mesma seed, no mesmo cenário, deve sempre gerar
 *   exatamente a mesma carga de trabalho (processos, rajadas de CPU,
 *   requisições de E/S, prioridades e tempos de chegada).
 *
 * O que deve ser definido aqui:
 *   - Assinatura da função que gera N processos a partir de uma seed
 *     e de um cenário
 *   - Identificação dos 4 cenários obrigatórios (ver docs/PROJETO.md,
 *     seção 6):
 *       1. Aleatório equilibrado
 *       2. I/O-bound
 *       3. CPU-bound / processos longos
 *       4. Prioridades desbalanceadas
 *   - Parâmetros mínimos: >= 1000 processos por execução, >= 100
 *     seeds por cenário (ver docs/PROJETO.md, seções 5 e 7)
 *
 * Requisitos do enunciado relacionados:
 *   - Seção 5 (Geração das cargas de trabalho) e seção 6 (Cenários
 *     obrigatórios) — ver docs/PROJETO.md
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

#endif /* WORKLOAD_GENERATOR_H */
