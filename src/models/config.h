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
 * O que deve ser definido aqui:
 *   - Número mínimo de processos por execução (>= 1000 nos
 *     experimentos principais, ver docs/PROJETO.md, seção 5)
 *   - Número mínimo de seeds por cenário (>= 100 nos experimentos
 *     principais)
 *   - Quantum padrão do Round Robin
 *   - Custo de troca de contexto padrão (> 0 nos experimentos
 *     principais; a equipe pode rodar experimentos complementares
 *     com custo = 0, ver docs/PROJETO.md, seção 4)
 *   - Convenção de prioridade (menor ou maior valor = maior
 *     prioridade), conforme decidido em docs/MODELAGEM.md
 *
 * TODO: implementar. Este arquivo é apenas um esqueleto inicial.
 * ===================================================================== */

#endif /* CONFIG_H */
