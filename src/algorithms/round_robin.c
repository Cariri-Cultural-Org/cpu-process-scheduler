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

/* #include "../scheduler.h" */
