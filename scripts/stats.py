#!/usr/bin/env python3
# =====================================================================
# stats.py
# =====================================================================
#
# Frente responsável: Cícero Jesus (métricas, estatística e
# visualização).
#
# Objetivo:
#   Centralizar as funções estatísticas usadas na consolidação dos
#   resultados e na geração dos gráficos, em especial o cálculo do
#   intervalo de confiança de 95%.
#
# O que deve ser implementado aqui:
#   - Função de média amostral
#   - Função de desvio padrão amostral
#   - Função de IC95%, seguindo a fórmula sugerida no enunciado:
#       IC95% = média +/- 1,96 * (desvio_padrao / sqrt(n))
#     onde n é o número de execuções independentes (ex.: número de
#     seeds usadas em um cenário)
#
# Requisitos do enunciado relacionados:
#   - Seção 7 (Seeds e rigor estatístico) — ver docs/PROJETO.md
#
# TODO: implementar. Este arquivo é apenas um esqueleto inicial.
# =====================================================================


def confidence_interval_95(values):
    """TODO: implementar cálculo do IC95% (ver docstring do módulo)."""
    raise NotImplementedError("TODO: implementar confidence_interval_95")
