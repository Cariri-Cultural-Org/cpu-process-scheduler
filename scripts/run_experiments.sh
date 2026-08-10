#!/usr/bin/env bash
# =====================================================================
# run_experiments.sh
# =====================================================================
#
# Frente responsável: Cícero Jesus (métricas, estatística e
# visualização).
#
# Objetivo:
#   Automatizar a execução do simulador (bin/simulador) para todas as
#   combinações de cenário x seed x algoritmo exigidas pelo
#   enunciado, e salvar as saídas brutas em results/raw/.
#
# O que deve ser implementado aqui:
#   - Lista dos 4 cenários obrigatórios (ver docs/PROJETO.md, seção 6)
#   - Loop sobre pelo menos 100 seeds por cenário (mesmas seeds para
#     todos os algoritmos, conforme exigido no enunciado)
#   - Loop sobre os algoritmos: FCFS, Round Robin, Prioridade e o
#     algoritmo próprio da equipe
#   - Chamada ao binário do simulador com os parâmetros de cada
#     combinação (cenário, seed, algoritmo, quantum, custo de troca
#     de contexto etc.)
#   - Salvar a saída de cada execução em results/raw/, com um nome de
#     arquivo que identifique cenário, seed e algoritmo
#
# TODO: implementar. Este arquivo é apenas um esqueleto inicial.
# =====================================================================

set -euo pipefail

echo "TODO: implementar run_experiments.sh"
