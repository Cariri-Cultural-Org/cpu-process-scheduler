# Simulador de Escalonamento de Processos

Projeto da Unidade 3 da disciplina de Sistemas Operacionais (última etapa avaliativa).

## Objetivo

Desenvolver, em C, um simulador de escalonamento de processos capaz de gerar cargas de
trabalho controladas por seed, executar algoritmos clássicos de escalonamento (FCFS, Round
Robin e Prioridade não preemptiva) e compará-los com um algoritmo próprio proposto pela
equipe, usando métricas quantitativas (turnaround médio, trocas de contexto e índice de Jain
do slowdown) com rigor estatístico (média + IC95% sobre múltiplas seeds).

A especificação completa do trabalho está em [`docs/PROJETO.md`](docs/PROJETO.md).

## Equipe

- Alan Mendes
- Antônio Neto
- Diogo Gomes
- Cícero Jesus

A divisão de tarefas entre os integrantes está em [`docs/DIVISAO_TAREFAS.md`](docs/DIVISAO_TAREFAS.md).

## Documentação

- [`docs/PROJETO.md`](docs/PROJETO.md) — objetivos, o que deve ser desenvolvido, entregáveis
  e critérios de avaliação, resumidos a partir do enunciado do professor.
- [`docs/DIVISAO_TAREFAS.md`](docs/DIVISAO_TAREFAS.md) — divisão de tarefas entre os
  integrantes e cronograma sugerido.
- [`docs/MODELAGEM.md`](docs/MODELAGEM.md) — template das decisões de modelagem que a
  equipe precisa definir e documentar (E/S, troca de contexto, prioridade, chegada de
  processos, algoritmo próprio).

## Estrutura do repositório

```
.
├── src/                          # Código-fonte do simulador (C)
│   ├── main.c                    # Ponto de entrada / integração (parsing de CLI, execução)
│   ├── models/
│   │   ├── config.h              # Parâmetros de configuração dos experimentos
│   │   ├── scheduler.h           # Contrato comum dos algoritmos de escalonamento
│   │   ├── process/              # Modelo de processo (process.h / process.c)
│   │   ├── simulator/            # Núcleo da simulação (fila de prontos, E/S, troca de contexto)
│   │   ├── metrics/              # Cálculo de métricas (turnaround, trocas de contexto, Jain)
│   │   └── workload/             # Geração de carga de trabalho controlada por seed (workload_generator.h / .c)
│   └── algorithms/               # FCFS, Round Robin, Prioridade e algoritmo próprio (predictive_sjf)
├── tests/                        # Testes unitários (Unity, ver "Como rodar os testes")
├── scripts/                      # Scripts auxiliares (execução em lote, consolidação, gráficos, estatística)
├── results/                      # Resultados das execuções (brutos, consolidados, gráficos)
├── report/                       # Artigo científico (relatório final)
├── slides/                       # Slides da apresentação
├── docs/                         # Documentação do projeto
└── Makefile
```

O núcleo do simulador e o pipeline experimental estão implementados e testados: `main.c`, os
módulos em `src/models/`, os quatro algoritmos em `src/algorithms/` e os scripts de execução em
lote, consolidação estatística e geração de gráficos em `scripts/`. Ver a divisão completa em
`docs/DIVISAO_TAREFAS.md`.

## Como compilar

```
make
```

Compila o simulador em `bin/simulador` a partir de tudo que existir em `src/`. Ajustar o
`Makefile` conforme necessário à medida que o projeto evoluir.

## Como rodar uma simulação

```
bin/simulador --scenario <balanced|io_bound|cpu_bound|priority_skewed>
              --seed <unsigned long>
              --algorithm <fcfs|round_robin|priority|predictive_sjf>
              [--processes N] [--quantum Q] [--cs-cost C] [--header] [--help]
```

Os valores padrão de `--processes` (1000), `--quantum` (4) e `--cs-cost` (1) vêm das macros
`CONFIG_MIN_PROCESSES`, `CONFIG_RR_QUANTUM` e `CONFIG_CONTEXT_SWITCH_COST` em
`src/models/config.h`.

A saída é uma única linha CSV em stdout com o resultado da execução; use `--header` para
emitir também a linha de cabeçalho antes dos dados. Colunas:

`scenario,seed,algorithm,n_processes,quantum,context_switch_cost,mean_turnaround,context_switches,jain_slowdown_percent,total_time,cpu_busy_time,context_switch_time,idle_time`

Exemplo:

```
$ ./bin/simulador --scenario balanced --seed 42 --algorithm fcfs --header
scenario,seed,algorithm,n_processes,quantum,context_switch_cost,mean_turnaround,context_switches,jain_slowdown_percent,total_time,cpu_busy_time,context_switch_time,idle_time
balanced,42,fcfs,1000,4,1,15241.292000,2896,14.905603,32533,29635,2896,2
```

## Como rodar os testes

```
git submodule update --init --recursive   # necessário na primeira vez, baixa o Unity em tests/unity
make test
```

Compila e executa um binário por arquivo `tests/test_*.c` usando o framework Unity
(submodule em `tests/unity`); `test_stats.c` também aciona a suíte `unittest` em Python de
`scripts/stats.py`. `test_experiment_pipeline.c`, escrito em C, testa o pipeline completo em
escala reduzida e verifica arquivos ausentes, duplicados e malformados.

## Como rodar os experimentos

```
./scripts/run_experiments.sh
python3 scripts/consolidate_results.py
python3 scripts/generate_graphs.py
```

O primeiro comando executa, por padrão, as 1.600 combinações obrigatórias (4 cenários × 100
seeds × 4 algoritmos), com 1.000 processos, quantum 4 e custo de troca 1, gravando um CSV por
execução em `results/raw/`. Use `--help` para alterar a escala ou os diretórios — útil para
experimentos complementares e testes rápidos.

`consolidate_results.py` exige a matriz completa, rejeita resultados ausentes, duplicados,
malformados ou com configurações inconsistentes e grava `results/consolidated/summary.csv`.
Para cada cenário, algoritmo e métrica, o arquivo contém média, desvio padrão amostral e IC95%.

`generate_graphs.py` produz três gráficos SVG em `results/graphs/`, um para cada métrica
obrigatória, com média e barras de erro do IC95%. Os scripts Python utilizam apenas a biblioteca
padrão; não há dependências externas para instalar.

## Entregáveis

- [ ] Link do repositório Git/GitHub
- [ ] Relatório em formato de artigo científico (PDF, 4 a 6 páginas) — `report/`
- [ ] Slides da apresentação — `slides/`
- [ ] Lista de integrantes e responsabilidades — `docs/DIVISAO_TAREFAS.md`

## Datas importantes

| Data | Detalhes |
|---|---|
| 19/08/2026 | Entrega do trabalho e primeiro dia de apresentações |
| 21/08/2026 | Segundo dia de apresentações |

A entrega deve ser realizada até 19/08/2026, independentemente da data de apresentação da equipe.
