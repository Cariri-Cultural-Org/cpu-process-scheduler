#ifndef CONFIG_H
#define CONFIG_H

/* =====================================================================
 * config.h
 * =====================================================================
 *
 * Frente responsável: definido em conjunto (parâmetros usados por
 * todos os módulos).
 *
 * Objetivo:
 *   Centralizar os parâmetros de configuração dos experimentos, para
 *   que fiquem fáceis de ajustar e para garantir que os mesmos
 *   valores sejam usados por todos os algoritmos em um mesmo
 *   experimento (exigência do enunciado).
 *
 * Parâmetros de experimento:
 *   Ver docs/PROJETO.md, seção 5, e docs/MODELAGEM.md.
 * ===================================================================== */

/* Número mínimo de processos por execução nos experimentos principais
 * (docs/PROJETO.md, seção 5). */
#define CONFIG_MIN_PROCESSES 1000

/* Número mínimo de seeds por cenário nos experimentos principais
 * (docs/PROJETO.md, seção 5). */
#define CONFIG_MIN_SEEDS 100

/* Quantum padrão do Round Robin (unidades de tempo)
 * (docs/MODELAGEM.md, seção 5). */
#define CONFIG_RR_QUANTUM 4

/* Custo padrão de troca de contexto (unidades de tempo)
 * (docs/MODELAGEM.md, seção 3). */
#define CONFIG_CONTEXT_SWITCH_COST 1

/* Convenção de prioridade:
 * Valores MAIORES representam MAIOR prioridade
 * (docs/MODELAGEM.md, seção 1).
 * Macro para comparação explícita: IS_HIGHER_PRIORITY(p1, p2) retorna
 * verdadeiro se p1 tem maior prioridade que p2. */
#define IS_HIGHER_PRIORITY(p1, p2) ((p1) > (p2))

/* Parâmetros do algoritmo próprio (SJF Preditivo com Aging)
 * Fonte: docs/MODELAGEM.md, seção 6. */

/* Fator da média exponencial para estimativa de rajada (ALPHA)
 * τ(n+1) = α · t(n) + (1 − α) · τ(n) */
#define PREDICTIVE_SJF_ALPHA 0.5

/* Fator de aging para suavização de starvation (BETA)
 * Prioridade efetiva = τ estimado − β · tempo_de_espera */
#define PREDICTIVE_SJF_BETA 0.1

/* Fallback inicial para estimativa de rajada quando nenhuma observação
 * foi ainda realizada (unidades de tempo) */
#define PREDICTIVE_SJF_DEFAULT_TAU_FALLBACK 10.0

#endif /* CONFIG_H */
