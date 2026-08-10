#ifndef PROCESS_H
#define PROCESS_H

/* =====================================================================
 * process.h
 * =====================================================================
 *
 * Frente responsável: Alan Mendes (núcleo da simulação).
 *
 * Objetivo:
 *   Definir a estrutura de dados que representa um processo simulado
 *   e seus possíveis estados, conforme exigido pelo enunciado.
 *
 * O que deve ser implementado aqui:
 *   - enum com os estados do processo: novo, pronto, executando,
 *     bloqueado, finalizado
 *   - struct do processo, contendo pelo menos:
 *       - identificador do processo
 *       - tempo de chegada
 *       - prioridade (usar a convenção definida em
 *         docs/MODELAGEM.md: valores menores ou maiores = maior
 *         prioridade)
 *       - sequência de rajadas de CPU e de E/S (ex.: CPU -> E/S ->
 *         CPU -> E/S -> CPU)
 *       - número e duração das requisições de E/S
 *       - estado atual
 *       - campos auxiliares para métricas (ex.: tempo de término,
 *         tempo restante da rajada atual)
 *
 * Requisitos do enunciado relacionados:
 *   - Seção 3 (Modelo de processo) — ver docs/PROJETO.md
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

#endif /* PROCESS_H */
