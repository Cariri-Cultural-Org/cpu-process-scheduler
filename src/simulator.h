#ifndef SIMULATOR_H
#define SIMULATOR_H

/* =====================================================================
 * simulator.h
 * =====================================================================
 *
 * Frente responsável: Alan Mendes (núcleo da simulação).
 *
 * Objetivo:
 *   Declarar a interface do núcleo da simulação: o laço principal que
 *   avança o tempo (discreto ou por eventos), gerencia a fila de
 *   prontos, os bloqueios por E/S e a troca de contexto, delegando ao
 *   algoritmo de escalonamento ativo a decisão de qual processo
 *   executar.
 *
 * O que deve ser definido aqui:
 *   - Assinatura da função que roda uma simulação completa, recebendo
 *     a carga de trabalho gerada, o algoritmo escolhido e os
 *     parâmetros de configuração (quantum, custo de troca de
 *     contexto etc.)
 *   - Estrutura(s) para representar a fila de prontos e a(s) fila(s)
 *     de E/S
 *   - Estrutura para armazenar os resultados brutos da simulação, a
 *     serem consumidos por src/metrics/
 *
 * Requisitos do enunciado relacionados:
 *   - Seção 4 (Simulação) — ver docs/PROJETO.md e docs/MODELAGEM.md
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

#endif /* SIMULATOR_H */
