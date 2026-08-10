#!/usr/bin/env python3
# =====================================================================
# consolidate_results.py
# =====================================================================
#
# Frente responsável: Cícero Jesus (métricas, estatística e
# visualização).
#
# Objetivo:
#   Ler os resultados brutos gerados pelo simulador (results/raw/),
#   uma execução por combinação de cenário x seed x algoritmo, e
#   consolidá-los em tabelas (ex.: CSV) prontas para análise
#   estatística e para os gráficos usados no artigo.
#
# O que deve ser implementado aqui:
#   - Leitura de todos os arquivos de results/raw/
#   - Agrupamento por cenário e por algoritmo
#   - Cálculo de média e intervalo de confiança de 95% (ver
#     scripts/stats.py) para cada métrica obrigatória: turnaround
#     médio, trocas de contexto e índice de Jain do slowdown
#   - Gravação das tabelas consolidadas em results/consolidated/
#
# Requisitos do enunciado relacionados:
#   - Seção 7 (Seeds e rigor estatístico) e seção 9 (Resultados
#     esperados) — ver docs/PROJETO.md
#
# TODO: implementar. Este arquivo é apenas um esqueleto inicial.
# =====================================================================


def main():
    raise NotImplementedError("TODO: implementar consolidate_results.py")


if __name__ == "__main__":
    main()
