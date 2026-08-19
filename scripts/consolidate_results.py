#!/usr/bin/env python3
"""Valida e consolida os resultados brutos dos experimentos.

O programa exige uma matriz experimental completa. Arquivos malformados,
combinações ausentes/duplicadas e parâmetros inconsistentes fazem a execução
falhar, evitando a produção silenciosa de IC95% sobre dados incompletos.
"""

from __future__ import annotations

import argparse
import csv
import math
import os
from pathlib import Path
import sys
import tempfile

from stats import confidence_interval_95, sample_mean, sample_standard_deviation


DEFAULT_SCENARIOS = ("balanced", "io_bound", "cpu_bound", "priority_skewed")
DEFAULT_ALGORITHMS = ("fcfs", "round_robin", "priority", "predictive_sjf")
RAW_FIELDS = (
    "scenario",
    "seed",
    "algorithm",
    "n_processes",
    "quantum",
    "context_switch_cost",
    "mean_turnaround",
    "context_switches",
    "jain_slowdown_percent",
    "total_time",
    "cpu_busy_time",
    "context_switch_time",
    "idle_time",
)
OUTPUT_FIELDS = (
    "scenario",
    "algorithm",
    "metric",
    "unit",
    "n",
    "mean",
    "sample_stddev",
    "ci95_lower",
    "ci95_upper",
    "ci95_margin",
    "n_processes",
    "quantum",
    "context_switch_cost",
    "seed_start",
    "seed_end",
)
METRICS = (
    ("mean_turnaround", "time_units"),
    ("context_switches", "count"),
    ("jain_slowdown_percent", "percent"),
)


class ConsolidationError(ValueError):
    """Erro de validação dos resultados experimentais."""


def csv_list(text: str, allowed: tuple[str, ...], label: str) -> tuple[str, ...]:
    values = tuple(text.split(","))
    if not values or any(not value for value in values):
        raise argparse.ArgumentTypeError(f"lista de {label} inválida")
    unknown = [value for value in values if value not in allowed]
    if unknown:
        raise argparse.ArgumentTypeError(f"{label} desconhecido: {unknown[0]}")
    if len(set(values)) != len(values):
        raise argparse.ArgumentTypeError(f"lista de {label} contém duplicatas")
    return values


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input-dir", type=Path, default=Path("results/raw"))
    parser.add_argument(
        "--output-file",
        type=Path,
        default=Path("results/consolidated/summary.csv"),
    )
    parser.add_argument("--seed-start", type=int, default=1)
    parser.add_argument("--expected-seeds", type=int, default=100)
    parser.add_argument(
        "--scenarios",
        default=DEFAULT_SCENARIOS,
        type=lambda value: csv_list(value, DEFAULT_SCENARIOS, "cenário"),
    )
    parser.add_argument(
        "--algorithms",
        default=DEFAULT_ALGORITHMS,
        type=lambda value: csv_list(value, DEFAULT_ALGORITHMS, "algoritmo"),
    )
    args = parser.parse_args(argv)
    if args.seed_start < 0:
        parser.error("--seed-start deve ser não negativo")
    if args.expected_seeds < 2:
        parser.error("--expected-seeds deve ser pelo menos 2 para calcular IC95%")
    return args


def parse_int(row: dict[str, str], field: str, source: Path, minimum: int = 0) -> int:
    try:
        value = int(row[field])
    except (KeyError, TypeError, ValueError) as error:
        raise ConsolidationError(f"{source}: campo inteiro inválido '{field}'") from error
    if value < minimum:
        raise ConsolidationError(f"{source}: campo '{field}' deve ser >= {minimum}")
    return value


def parse_float(
    row: dict[str, str], field: str, source: Path, minimum: float = 0.0
) -> float:
    try:
        value = float(row[field])
    except (KeyError, TypeError, ValueError) as error:
        raise ConsolidationError(f"{source}: campo numérico inválido '{field}'") from error
    if not math.isfinite(value) or value < minimum:
        raise ConsolidationError(f"{source}: campo '{field}' não é finito ou é menor que {minimum}")
    return value


def read_raw_file(path: Path) -> dict[str, object]:
    try:
        with path.open(newline="", encoding="utf-8") as stream:
            reader = csv.DictReader(stream)
            if tuple(reader.fieldnames or ()) != RAW_FIELDS:
                raise ConsolidationError(f"{path}: cabeçalho CSV inesperado")
            rows = list(reader)
    except OSError as error:
        raise ConsolidationError(f"não foi possível ler {path}: {error}") from error

    if len(rows) != 1:
        raise ConsolidationError(f"{path}: esperado exatamente um registro, encontrados {len(rows)}")
    row = rows[0]
    if None in row or any(value is None for value in row.values()):
        raise ConsolidationError(f"{path}: quantidade de colunas inválida")

    scenario = row["scenario"]
    algorithm = row["algorithm"]
    seed = parse_int(row, "seed", path)
    n_processes = parse_int(row, "n_processes", path, 1)
    quantum = parse_int(row, "quantum", path, 1)
    context_switch_cost = parse_int(row, "context_switch_cost", path)
    mean_turnaround = parse_float(row, "mean_turnaround", path, 0.0)
    context_switches = parse_int(row, "context_switches", path)
    jain = parse_float(row, "jain_slowdown_percent", path, 0.0)
    total_time = parse_int(row, "total_time", path)
    cpu_busy_time = parse_int(row, "cpu_busy_time", path)
    context_switch_time = parse_int(row, "context_switch_time", path)
    idle_time = parse_int(row, "idle_time", path)

    if mean_turnaround <= 0.0:
        raise ConsolidationError(f"{path}: mean_turnaround deve ser positivo")
    if not 0.0 < jain <= 100.0:
        raise ConsolidationError(f"{path}: Jain deve estar em (0, 100]")
    if total_time != cpu_busy_time + context_switch_time + idle_time:
        raise ConsolidationError(f"{path}: contabilidade de tempo inconsistente")
    if context_switch_time != context_switches * context_switch_cost:
        raise ConsolidationError(f"{path}: tempo de troca de contexto inconsistente")

    return {
        "scenario": scenario,
        "seed": seed,
        "algorithm": algorithm,
        "n_processes": n_processes,
        "quantum": quantum,
        "context_switch_cost": context_switch_cost,
        "mean_turnaround": mean_turnaround,
        "context_switches": context_switches,
        "jain_slowdown_percent": jain,
        "cpu_busy_time": cpu_busy_time,
    }


def load_and_validate(
    input_dir: Path,
    scenarios: tuple[str, ...],
    algorithms: tuple[str, ...],
    seed_start: int,
    expected_seeds: int,
) -> tuple[dict[tuple[str, int, str], dict[str, object]], tuple[int, int, int]]:
    if not input_dir.is_dir():
        raise ConsolidationError(f"diretório de entrada inexistente: {input_dir}")
    paths = sorted(input_dir.glob("*.csv"))
    if not paths:
        raise ConsolidationError(f"nenhum CSV encontrado em {input_dir}")

    records: dict[tuple[str, int, str], dict[str, object]] = {}
    configurations: set[tuple[int, int, int]] = set()
    busy_by_workload: dict[tuple[str, int], int] = {}
    for path in paths:
        record = read_raw_file(path)
        key = (str(record["scenario"]), int(record["seed"]), str(record["algorithm"]))
        if key in records:
            raise ConsolidationError(f"combinação duplicada {key} em {path}")
        records[key] = record
        configurations.add(
            (
                int(record["n_processes"]),
                int(record["quantum"]),
                int(record["context_switch_cost"]),
            )
        )
        workload_key = (key[0], key[1])
        busy = int(record["cpu_busy_time"])
        if workload_key in busy_by_workload and busy_by_workload[workload_key] != busy:
            raise ConsolidationError(
                f"tempo de CPU difere entre algoritmos para cenário/seed {workload_key}"
            )
        busy_by_workload[workload_key] = busy

    if len(configurations) != 1:
        raise ConsolidationError(
            f"resultados misturam {len(configurations)} configurações de processos/quantum/custo"
        )

    seeds = range(seed_start, seed_start + expected_seeds)
    expected = {
        (scenario, seed, algorithm)
        for scenario in scenarios
        for seed in seeds
        for algorithm in algorithms
    }
    actual = set(records)
    missing = sorted(expected - actual)
    unexpected = sorted(actual - expected)
    if missing:
        suffix = "..." if len(missing) > 3 else ""
        raise ConsolidationError(f"faltam {len(missing)} combinações: {missing[:3]}{suffix}")
    if unexpected:
        suffix = "..." if len(unexpected) > 3 else ""
        raise ConsolidationError(
            f"há {len(unexpected)} combinações inesperadas: {unexpected[:3]}{suffix}"
        )

    return records, next(iter(configurations))


def consolidate(
    records: dict[tuple[str, int, str], dict[str, object]],
    configuration: tuple[int, int, int],
    scenarios: tuple[str, ...],
    algorithms: tuple[str, ...],
    seed_start: int,
    expected_seeds: int,
) -> list[dict[str, object]]:
    n_processes, quantum, context_switch_cost = configuration
    output: list[dict[str, object]] = []
    for scenario in scenarios:
        for algorithm in algorithms:
            grouped = [
                records[(scenario, seed, algorithm)]
                for seed in range(seed_start, seed_start + expected_seeds)
            ]
            for metric, unit in METRICS:
                values = [float(record[metric]) for record in grouped]
                mean = sample_mean(values)
                stddev = sample_standard_deviation(values)
                lower, upper = confidence_interval_95(values)
                output.append(
                    {
                        "scenario": scenario,
                        "algorithm": algorithm,
                        "metric": metric,
                        "unit": unit,
                        "n": len(values),
                        "mean": f"{mean:.9f}",
                        "sample_stddev": f"{stddev:.9f}",
                        "ci95_lower": f"{lower:.9f}",
                        "ci95_upper": f"{upper:.9f}",
                        "ci95_margin": f"{(upper - lower) / 2.0:.9f}",
                        "n_processes": n_processes,
                        "quantum": quantum,
                        "context_switch_cost": context_switch_cost,
                        "seed_start": seed_start,
                        "seed_end": seed_start + expected_seeds - 1,
                    }
                )
    return output


def write_atomic_csv(path: Path, rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            "w", newline="", encoding="utf-8", dir=path.parent, delete=False
        ) as stream:
            temporary_name = stream.name
            writer = csv.DictWriter(stream, fieldnames=OUTPUT_FIELDS)
            writer.writeheader()
            writer.writerows(rows)
        os.chmod(temporary_name, 0o644)
        os.replace(temporary_name, path)
    except OSError as error:
        if temporary_name is not None:
            Path(temporary_name).unlink(missing_ok=True)
        raise ConsolidationError(f"não foi possível escrever {path}: {error}") from error


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        records, configuration = load_and_validate(
            args.input_dir,
            tuple(args.scenarios),
            tuple(args.algorithms),
            args.seed_start,
            args.expected_seeds,
        )
        rows = consolidate(
            records,
            configuration,
            tuple(args.scenarios),
            tuple(args.algorithms),
            args.seed_start,
            args.expected_seeds,
        )
        write_atomic_csv(args.output_file, rows)
    except ConsolidationError as error:
        print(f"Erro: {error}", file=sys.stderr)
        return 1

    print(
        f"Consolidação concluída: {len(records)} execuções, "
        f"{len(rows)} linhas em {args.output_file}",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
