#!/usr/bin/env bash

# Executa a matriz cenário x seed x algoritmo e grava um CSV por execução.
# Os valores padrão correspondem ao experimento mínimo obrigatório descrito
# em docs/PROJETO.md: 4 cenários, 100 seeds, 4 algoritmos e 1000 processos.

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PROJECT_ROOT=$(cd "$SCRIPT_DIR/.." && pwd)

SIMULATOR="$PROJECT_ROOT/bin/simulador"
OUTPUT_DIR="$PROJECT_ROOT/results/raw"
SEED_START=1
SEED_COUNT=100
PROCESSES=1000
QUANTUM=4
CS_COST=1
SCENARIOS_CSV="balanced,io_bound,cpu_bound,priority_skewed"
ALGORITHMS_CSV="fcfs,round_robin,priority,predictive_sjf"
CURRENT_TEMP=""

usage() {
    cat <<'EOF'
Uso: scripts/run_experiments.sh [opções]

Opções:
  --simulator PATH       binário do simulador (padrão: bin/simulador)
  --output-dir DIR       diretório dos CSVs brutos (padrão: results/raw)
  --seed-start N         primeira seed, inclusive (padrão: 1)
  --seeds N              quantidade de seeds (padrão: 100)
  --processes N          processos por execução (padrão: 1000)
  --quantum N            quantum do Round Robin (padrão: 4)
  --cs-cost N            custo de troca de contexto (padrão: 1)
  --scenarios LISTA      nomes separados por vírgula
  --algorithms LISTA     nomes separados por vírgula
  --help                 mostra esta ajuda
EOF
}

fail() {
    echo "Erro: $*" >&2
    exit 1
}

need_value() {
    local option=$1
    local count=$2
    (( count >= 2 )) || fail "$option requer um valor."
}

is_nonnegative_integer() {
    [[ $1 =~ ^[0-9]+$ ]]
}

validate_csv_list() {
    local kind=$1
    local csv=$2
    local item
    local -A seen=()
    local -a values

    IFS=',' read -r -a values <<< "$csv"
    (( ${#values[@]} > 0 )) || fail "lista de $kind vazia."

    for item in "${values[@]}"; do
        [[ -n $item ]] || fail "lista de $kind contém item vazio."
        if [[ $kind == "cenários" ]]; then
            case "$item" in
                balanced|io_bound|cpu_bound|priority_skewed) ;;
                *) fail "cenário inválido '$item'." ;;
            esac
        else
            case "$item" in
                fcfs|round_robin|priority|predictive_sjf) ;;
                *) fail "algoritmo inválido '$item'." ;;
            esac
        fi
        [[ -z ${seen[$item]+x} ]] || fail "$kind contém item duplicado '$item'."
        seen[$item]=1
    done
}

while (( $# > 0 )); do
    case "$1" in
        --simulator)
            need_value "$1" "$#"; SIMULATOR=$2; shift 2 ;;
        --output-dir)
            need_value "$1" "$#"; OUTPUT_DIR=$2; shift 2 ;;
        --seed-start)
            need_value "$1" "$#"; SEED_START=$2; shift 2 ;;
        --seeds)
            need_value "$1" "$#"; SEED_COUNT=$2; shift 2 ;;
        --processes)
            need_value "$1" "$#"; PROCESSES=$2; shift 2 ;;
        --quantum)
            need_value "$1" "$#"; QUANTUM=$2; shift 2 ;;
        --cs-cost)
            need_value "$1" "$#"; CS_COST=$2; shift 2 ;;
        --scenarios)
            need_value "$1" "$#"; SCENARIOS_CSV=$2; shift 2 ;;
        --algorithms)
            need_value "$1" "$#"; ALGORITHMS_CSV=$2; shift 2 ;;
        --help)
            usage; exit 0 ;;
        *)
            fail "opção desconhecida '$1'. Use --help para consultar o uso." ;;
    esac
done

is_nonnegative_integer "$SEED_START" || fail "--seed-start deve ser um inteiro não negativo."
is_nonnegative_integer "$SEED_COUNT" || fail "--seeds deve ser um inteiro positivo."
is_nonnegative_integer "$PROCESSES" || fail "--processes deve ser um inteiro positivo."
is_nonnegative_integer "$QUANTUM" || fail "--quantum deve ser um inteiro positivo."
is_nonnegative_integer "$CS_COST" || fail "--cs-cost deve ser um inteiro não negativo."
(( SEED_COUNT > 0 )) || fail "--seeds deve ser maior que zero."
(( PROCESSES > 0 )) || fail "--processes deve ser maior que zero."
(( QUANTUM > 0 )) || fail "--quantum deve ser maior que zero."
(( SEED_START + SEED_COUNT - 1 <= 4294967295 )) || fail "intervalo de seeds excede 32 bits."

validate_csv_list "cenários" "$SCENARIOS_CSV"
validate_csv_list "algoritmos" "$ALGORITHMS_CSV"

if [[ ! -x $SIMULATOR ]]; then
    if [[ $SIMULATOR == "$PROJECT_ROOT/bin/simulador" ]]; then
        make -C "$PROJECT_ROOT"
    else
        fail "simulador não encontrado ou não executável: $SIMULATOR"
    fi
fi
[[ -x $SIMULATOR ]] || fail "simulador não encontrado ou não executável: $SIMULATOR"

mkdir -p "$OUTPUT_DIR"
trap 'if [[ -n $CURRENT_TEMP && -e $CURRENT_TEMP ]]; then rm -f -- "$CURRENT_TEMP"; fi' EXIT

IFS=',' read -r -a SCENARIOS <<< "$SCENARIOS_CSV"
IFS=',' read -r -a ALGORITHMS <<< "$ALGORITHMS_CSV"

total=$((${#SCENARIOS[@]} * ${#ALGORITHMS[@]} * SEED_COUNT))
completed=0

for scenario in "${SCENARIOS[@]}"; do
    for algorithm in "${ALGORITHMS[@]}"; do
        for ((offset = 0; offset < SEED_COUNT; offset++)); do
            seed=$((SEED_START + offset))
            output="$OUTPUT_DIR/${scenario}_seed-${seed}_${algorithm}.csv"
            CURRENT_TEMP=$(mktemp "$OUTPUT_DIR/.experiment.XXXXXX")

            "$SIMULATOR" \
                --scenario "$scenario" \
                --seed "$seed" \
                --algorithm "$algorithm" \
                --processes "$PROCESSES" \
                --quantum "$QUANTUM" \
                --cs-cost "$CS_COST" \
                --header > "$CURRENT_TEMP"

            line_count=$(wc -l < "$CURRENT_TEMP")
            (( line_count == 2 )) || fail "saída inválida do simulador para $scenario/seed=$seed/$algorithm."
            chmod 0644 "$CURRENT_TEMP"
            mv -f -- "$CURRENT_TEMP" "$output"
            CURRENT_TEMP=""
            completed=$((completed + 1))
        done
        echo "Concluído: $scenario / $algorithm ($SEED_COUNT seeds)" >&2
    done
done

echo "Experimentos concluídos: $completed/$total arquivos em $OUTPUT_DIR" >&2
