#!/usr/bin/env python3
"""Gera gráficos SVG de média e IC95% a partir do CSV consolidado.

O gerador usa apenas a biblioteca padrão do Python. Cada métrica produz um
gráfico de barras agrupadas por cenário, com uma barra por algoritmo e barras
de erro correspondentes ao intervalo de confiança de 95%.
"""

from __future__ import annotations

import argparse
import csv
import math
import os
from pathlib import Path
import sys
import tempfile
from xml.sax.saxutils import escape


FIELDS = (
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
METRIC_INFO = {
    "mean_turnaround": ("Turnaround médio por cenário", "Unidades de tempo"),
    "context_switches": ("Trocas de contexto por cenário", "Quantidade de trocas"),
    "jain_slowdown_percent": ("Justiça do slowdown por cenário", "Índice de Jain (%)"),
}
ALGORITHM_LABELS = {
    "fcfs": "FCFS",
    "round_robin": "Round Robin",
    "priority": "Prioridade",
    "predictive_sjf": "SJF Preditivo + Aging",
}
SCENARIO_LABELS = {
    "balanced": "Equilibrado",
    "io_bound": "I/O-bound",
    "cpu_bound": "CPU-bound",
    "priority_skewed": "Prioridades",
}
COLORS = ("#4472C4", "#ED7D31", "#70AD47", "#A64D79", "#5B9BD5", "#FFC000")


class GraphError(ValueError):
    """Erro nos dados consolidados ou na geração dos gráficos."""


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--input-file",
        type=Path,
        default=Path("results/consolidated/summary.csv"),
    )
    parser.add_argument("--output-dir", type=Path, default=Path("results/graphs"))
    return parser.parse_args(argv)


def numeric(row: dict[str, str], field: str, source: Path) -> float:
    try:
        value = float(row[field])
    except (KeyError, TypeError, ValueError) as error:
        raise GraphError(f"{source}: campo numérico inválido '{field}'") from error
    if not math.isfinite(value):
        raise GraphError(f"{source}: campo não finito '{field}'")
    return value


def load_rows(path: Path) -> list[dict[str, object]]:
    try:
        with path.open(newline="", encoding="utf-8") as stream:
            reader = csv.DictReader(stream)
            if tuple(reader.fieldnames or ()) != FIELDS:
                raise GraphError(f"{path}: cabeçalho consolidado inesperado")
            raw_rows = list(reader)
    except OSError as error:
        raise GraphError(f"não foi possível ler {path}: {error}") from error
    if not raw_rows:
        raise GraphError(f"{path}: arquivo consolidado vazio")

    rows: list[dict[str, object]] = []
    seen: set[tuple[str, str, str]] = set()
    for raw in raw_rows:
        if None in raw or any(value is None for value in raw.values()):
            raise GraphError(f"{path}: quantidade de colunas inválida")
        metric = raw["metric"]
        if metric not in METRIC_INFO:
            raise GraphError(f"{path}: métrica desconhecida '{metric}'")
        mean = numeric(raw, "mean", path)
        lower = numeric(raw, "ci95_lower", path)
        upper = numeric(raw, "ci95_upper", path)
        n = numeric(raw, "n", path)
        if n < 2 or lower > mean or mean > upper:
            raise GraphError(f"{path}: IC95% inválido para {raw['scenario']}/{raw['algorithm']}/{metric}")
        key = (raw["scenario"], raw["algorithm"], metric)
        if key in seen:
            raise GraphError(f"{path}: grupo consolidado duplicado {key}")
        seen.add(key)
        rows.append(
            {
                "scenario": raw["scenario"],
                "algorithm": raw["algorithm"],
                "metric": metric,
                "mean": mean,
                "lower": lower,
                "upper": upper,
            }
        )

    scenarios = tuple(dict.fromkeys(str(row["scenario"]) for row in rows))
    algorithms = tuple(dict.fromkeys(str(row["algorithm"]) for row in rows))
    expected = {
        (scenario, algorithm, metric)
        for scenario in scenarios
        for algorithm in algorithms
        for metric in METRIC_INFO
    }
    if seen != expected:
        missing = sorted(expected - seen)
        raise GraphError(f"{path}: grupos ausentes no consolidado: {missing[:3]}")
    return rows


def svg_chart(rows: list[dict[str, object]], metric: str) -> str:
    selected = [row for row in rows if row["metric"] == metric]
    scenarios = tuple(dict.fromkeys(str(row["scenario"]) for row in selected))
    algorithms = tuple(dict.fromkeys(str(row["algorithm"]) for row in selected))
    by_key = {(str(row["scenario"]), str(row["algorithm"])): row for row in selected}

    width, height = 1280, 720
    left, right, top, bottom = 105.0, 35.0, 95.0, 155.0
    plot_width = width - left - right
    plot_height = height - top - bottom
    maximum = max(float(row["upper"]) for row in selected)
    if metric == "jain_slowdown_percent":
        maximum = 100.0
    else:
        maximum = maximum * 1.08 if maximum > 0.0 else 1.0

    def y(value: float) -> float:
        return top + plot_height * (1.0 - value / maximum)

    title, y_label = METRIC_INFO[metric]
    parts = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img">',
        f'<title>{escape(title)} — média e IC 95%</title>',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        f'<text x="{width / 2:.1f}" y="38" text-anchor="middle" font-family="sans-serif" font-size="24" font-weight="bold">{escape(title)}</text>',
        f'<text x="{width / 2:.1f}" y="66" text-anchor="middle" font-family="sans-serif" font-size="15" fill="#555">Média e intervalo de confiança de 95% entre seeds</text>',
    ]

    for tick in range(6):
        value = maximum * tick / 5.0
        py = y(value)
        label = f"{value:.1f}" if maximum <= 120 else f"{value:.0f}"
        parts.append(f'<line x1="{left}" y1="{py:.2f}" x2="{width-right}" y2="{py:.2f}" stroke="#dddddd" stroke-width="1"/>')
        parts.append(f'<text x="{left-10}" y="{py+5:.2f}" text-anchor="end" font-family="sans-serif" font-size="13">{label}</text>')

    parts.append(f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top+plot_height}" stroke="#333"/>')
    parts.append(f'<line x1="{left}" y1="{top+plot_height}" x2="{width-right}" y2="{top+plot_height}" stroke="#333"/>')
    parts.append(f'<text transform="translate(28 {top+plot_height/2:.2f}) rotate(-90)" text-anchor="middle" font-family="sans-serif" font-size="15">{escape(y_label)}</text>')

    group_width = plot_width / len(scenarios)
    usable = group_width * 0.78
    bar_width = usable / len(algorithms)
    for scenario_index, scenario in enumerate(scenarios):
        group_start = left + scenario_index * group_width + (group_width - usable) / 2.0
        center = left + (scenario_index + 0.5) * group_width
        for algorithm_index, algorithm in enumerate(algorithms):
            row = by_key[(scenario, algorithm)]
            mean = float(row["mean"])
            lower_value = max(0.0, float(row["lower"]))
            upper_value = float(row["upper"])
            x = group_start + algorithm_index * bar_width
            bar_top = y(mean)
            color = COLORS[algorithm_index % len(COLORS)]
            parts.append(f'<rect x="{x+1:.2f}" y="{bar_top:.2f}" width="{max(1.0,bar_width-2):.2f}" height="{top+plot_height-bar_top:.2f}" fill="{color}"/>')
            cx = x + bar_width / 2.0
            low_y, high_y = y(lower_value), y(upper_value)
            cap = min(8.0, bar_width * 0.35)
            parts.append(f'<line x1="{cx:.2f}" y1="{high_y:.2f}" x2="{cx:.2f}" y2="{low_y:.2f}" stroke="#111" stroke-width="1.5"/>')
            parts.append(f'<line x1="{cx-cap:.2f}" y1="{high_y:.2f}" x2="{cx+cap:.2f}" y2="{high_y:.2f}" stroke="#111" stroke-width="1.5"/>')
            parts.append(f'<line x1="{cx-cap:.2f}" y1="{low_y:.2f}" x2="{cx+cap:.2f}" y2="{low_y:.2f}" stroke="#111" stroke-width="1.5"/>')
        label = SCENARIO_LABELS.get(scenario, scenario)
        parts.append(f'<text x="{center:.2f}" y="{top+plot_height+25:.2f}" text-anchor="middle" font-family="sans-serif" font-size="14">{escape(label)}</text>')

    legend_y = height - 70.0
    legend_item_width = plot_width / len(algorithms)
    for index, algorithm in enumerate(algorithms):
        x = left + index * legend_item_width
        parts.append(f'<rect x="{x:.2f}" y="{legend_y:.2f}" width="18" height="18" fill="{COLORS[index % len(COLORS)]}"/>')
        label = ALGORITHM_LABELS.get(algorithm, algorithm)
        parts.append(f'<text x="{x+26:.2f}" y="{legend_y+14:.2f}" font-family="sans-serif" font-size="13">{escape(label)}</text>')

    parts.append('</svg>')
    return "\n".join(parts) + "\n"


def write_atomic(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            "w", encoding="utf-8", dir=path.parent, delete=False
        ) as stream:
            temporary_name = stream.name
            stream.write(content)
        os.chmod(temporary_name, 0o644)
        os.replace(temporary_name, path)
    except OSError as error:
        if temporary_name is not None:
            Path(temporary_name).unlink(missing_ok=True)
        raise GraphError(f"não foi possível escrever {path}: {error}") from error


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        rows = load_rows(args.input_file)
        for metric in METRIC_INFO:
            write_atomic(args.output_dir / f"{metric}.svg", svg_chart(rows, metric))
    except GraphError as error:
        print(f"Erro: {error}", file=sys.stderr)
        return 1

    print(f"Gráficos concluídos: {len(METRIC_INFO)} arquivos em {args.output_dir}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
