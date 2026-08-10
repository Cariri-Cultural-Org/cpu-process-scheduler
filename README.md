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
├── src/                    # Código-fonte do simulador (C)
│   ├── main.c              # Ponto de entrada / integração
│   ├── process.h / .c      # Modelo de processo
│   ├── simulator.h / .c    # Núcleo da simulação (fila de prontos, E/S, troca de contexto)
│   ├── scheduler.h         # Contrato comum dos algoritmos de escalonamento
│   ├── config.h            # Parâmetros de configuração dos experimentos
│   ├── algorithms/         # FCFS, Round Robin, Prioridade e algoritmo próprio
│   ├── workload/           # Geração de carga de trabalho controlada por seed
│   └── metrics/            # Cálculo de métricas (turnaround, trocas de contexto, Jain)
├── scripts/                # Scripts auxiliares (execução em lote, consolidação, gráficos, estatística)
├── results/                # Resultados das execuções (brutos, consolidados, gráficos)
├── report/                 # Artigo científico (relatório final)
├── slides/                 # Slides da apresentação
├── docs/                   # Documentação do projeto
└── Makefile
```

Cada arquivo do esqueleto acima contém, em formato de comentário, uma descrição do que deve
ser implementado nele e a frente de trabalho responsável — ver a divisão completa em
`docs/DIVISAO_TAREFAS.md`.

## Como compilar

```
make
```

Compila o simulador em `bin/simulador` a partir de tudo que existir em `src/`. Ajustar o
`Makefile` conforme necessário à medida que o projeto evoluir.

## Como rodar os experimentos

```
./scripts/run_experiments.sh
```

Script a ser implementado pela equipe para rodar todas as combinações de cenário x seed x
algoritmo exigidas pelo enunciado e salvar as saídas em `results/raw/`. Os scripts Python de
consolidação e geração de gráficos ficam em `scripts/` (ver `scripts/requirements.txt`).

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
