/* =====================================================================
 * main.c
 * =====================================================================
 *
 * Frente responsável: Alan Mendes (núcleo da simulação), com
 * integração das partes desenvolvidas pelos demais integrantes.
 *
 * Objetivo:
 *   Ponto de entrada do simulador. Deve orquestrar: leitura dos
 *   parâmetros de execução, geração da carga de trabalho, execução da
 *   simulação com o algoritmo escolhido e gravação dos resultados.
 *
 * O que deve ser implementado aqui:
 *   - Leitura de parâmetros via linha de comando (ou arquivo de
 *     configuração), por exemplo:
 *       - cenário a simular (1 a 4, ver docs/PROJETO.md, seção 6)
 *       - seed
 *       - algoritmo de escalonamento (fcfs, round_robin, prioridade,
 *         algoritmo próprio)
 *       - quantum (Round Robin) e custo de troca de contexto
 *       - número de processos
 *   - Chamada ao gerador de carga de trabalho (src/workload/)
 *   - Chamada ao núcleo da simulação (src/simulator.c) com o
 *     algoritmo escolhido (src/algorithms/ + src/scheduler.h)
 *   - Chamada ao cálculo de métricas (src/metrics/) e gravação da
 *     saída (ex.: CSV em results/raw/), para uso pelos scripts de
 *     consolidação e geração de gráficos (scripts/)
 *
 * Este arquivo deve permanecer relativamente simples: a lógica de
 * cada etapa deve viver nos módulos correspondentes.
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

/* #include "process.h" */
/* #include "simulator.h" */
/* #include "scheduler.h" */
/* #include "workload/workload_generator.h" */
/* #include "metrics/metrics.h" */

int main(void) {
    /* TODO: implementar. */
    return 0;
}
