/* =====================================================================
 * simulator.c
 * =====================================================================
 *
 * Frente responsável: Alan Mendes (núcleo da simulação).
 *
 * Objetivo:
 *   Implementar o laço principal da simulação descrito em
 *   simulator.h.
 *
 * O que deve ser implementado aqui:
 *   - Avanço do tempo (discreto ou por eventos) até que todos os
 *     processos da carga de trabalho tenham sido finalizados
 *   - Entrada de processos na fila de prontos ao chegarem (conforme
 *     modelo de chegada definido em docs/MODELAGEM.md)
 *   - Retirada de um processo da CPU e bloqueio quando solicita E/S;
 *     retorno à fila de prontos ao final da E/S (conforme modelagem
 *     definida em docs/MODELAGEM.md)
 *   - Aplicação do custo de troca de contexto (configurável, > 0 nos
 *     experimentos principais), igual para todos os algoritmos em um
 *     mesmo experimento
 *   - Chamada ao algoritmo de escalonamento ativo (ver scheduler.h)
 *     para decidir o próximo processo a executar
 *   - Registro dos dados brutos necessários para o cálculo das
 *     métricas em src/metrics/ (tempos de chegada/término, número de
 *     trocas de contexto etc.)
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

/* #include "simulator.h" */
