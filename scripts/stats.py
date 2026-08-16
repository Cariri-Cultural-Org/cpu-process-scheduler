#!/usr/bin/env python3
"""Funções estatísticas usadas na análise dos experimentos.

média +/- 1,96 * (desvio padrão amostral / sqrt(n))

As funções aceitam qualquer iterável numérico finito. Pelo menos duas
observações independentes são exigidas para o desvio padrão amostral e para
o intervalo de confiança.
"""

from collections.abc import Iterable
from math import fsum, isfinite, sqrt


CONFIDENCE_Z_95 = 1.96


def _validated_values(values: Iterable[float]) -> list[float]:
    """Materializa e valida uma amostra numérica."""
    if values is None:
        raise TypeError("values não pode ser None")

    try:
        sample = [float(value) for value in values]
    except TypeError as error:
        raise TypeError("values deve ser um iterável numérico") from error
    except ValueError as error:
        raise ValueError("values deve conter apenas números") from error

    if not sample:
        raise ValueError("a amostra deve conter pelo menos um valor")
    if not all(isfinite(value) for value in sample):
        raise ValueError("a amostra deve conter apenas valores finitos")

    return sample


def sample_mean(values: Iterable[float]) -> float:
    """Retorna a média aritmética dos valores da amostra."""
    sample = _validated_values(values)
    return fsum(sample) / len(sample)


def sample_standard_deviation(values: Iterable[float]) -> float:
    """Retorna o desvio padrão amostral, usando ``n - 1`` no divisor."""
    sample = _validated_values(values)
    if len(sample) < 2:
        raise ValueError(
            "o desvio padrão amostral exige pelo menos dois valores"
        )

    mean = fsum(sample) / len(sample)
    squared_deviations = fsum((value - mean) ** 2 for value in sample)
    return sqrt(squared_deviations / (len(sample) - 1))


def confidence_interval_95(values: Iterable[float]) -> tuple[float, float]:
    """Retorna os limites inferior e superior do IC normal de 95%.
    O cálculo usa o desvio padrão amostral, portanto requer no mínimo duas observações.
    """
    sample = _validated_values(values)
    if len(sample) < 2:
        raise ValueError("o intervalo de confiança exige pelo menos dois valores")

    mean = fsum(sample) / len(sample)
    squared_deviations = fsum((value - mean) ** 2 for value in sample)
    standard_deviation = sqrt(squared_deviations / (len(sample) - 1))
    margin = CONFIDENCE_Z_95 * standard_deviation / sqrt(len(sample))

    return mean - margin, mean + margin
