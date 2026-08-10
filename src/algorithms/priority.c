/* =====================================================================
 * algorithms/priority.c
 * =====================================================================
 *
 * Frente responsável: Antônio Neto (algoritmos clássicos).
 *
 * Objetivo:
 *   Implementar o escalonamento por prioridade, não preemptivo: entre
 *   os processos prontos, executa o de maior prioridade até o fim da
 *   rajada de CPU atual.
 *
 * O que deve ser implementado aqui:
 *   - Função que segue o contrato definido em scheduler.h
 *   - Uso da convenção de prioridade definida em docs/MODELAGEM.md
 *     (valores menores ou maiores = maior prioridade)
 *   - Critério de desempate entre processos de mesma prioridade
 *     (documentar em docs/MODELAGEM.md)
 *   - Atenção: por ser não preemptivo, um processo em execução não é
 *     interrompido por outro de prioridade mais alta que chegue
 *     enquanto ele executa
 *
 * Requisitos do enunciado relacionados:
 *   - Seção 8 (Algoritmos de escalonamento) — ver docs/PROJETO.md
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

/* #include "../scheduler.h" */
