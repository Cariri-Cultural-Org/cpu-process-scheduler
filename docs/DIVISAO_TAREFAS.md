# Divisão de Tarefas — Simulador de Escalonamento de Processos

## Integrantes

- Alan Mendes
- Antônio Neto
- Diogo Gomes
- Cícero Jesus

> O enunciado sugere equipes de aproximadamente 5 integrantes; esta equipe conta com 4. A
> divisão abaixo distribui as frentes de trabalho do projeto entre os 4 integrantes, com a
> frente de núcleo/integração sob responsabilidade do Alan. Ajustar livremente conforme a
> disponibilidade real de cada um.

## Frentes de trabalho e responsáveis

### 1. Alan Mendes — Núcleo da simulação e integração

- Modelo de processo (`src/process.h/.c`): estados (novo, pronto, executando, bloqueado,
  finalizado), atributos, rajadas de CPU/E/S.
- Núcleo da simulação (`src/simulator.h/.c`): laço de simulação (tempo discreto ou por
  eventos), fila de prontos, bloqueio/retorno de E/S, aplicação do custo de troca de contexto.
- Contrato comum dos algoritmos de escalonamento (`src/scheduler.h`), definido em conjunto
  com quem for implementar os algoritmos.
- `src/main.c` e `src/config.h`: integração de todas as peças (geração de carga →
  simulação → métricas → saída) e parâmetros globais dos experimentos.
- Organização geral do repositório, `Makefile` e revisão final de integração antes da
  entrega.

### 2. Antônio Neto — Algoritmos clássicos de escalonamento

- `src/algorithms/fcfs.c` — First-Come, First-Served.
- `src/algorithms/round_robin.c` — Round Robin com quantum configurável.
- `src/algorithms/priority.c` — Prioridade não preemptiva.
- Documentar critérios de desempate e o quantum usado em `docs/MODELAGEM.md`.
- No artigo: seção com a descrição resumida dos algoritmos clássicos (o suficiente para
  reprodutibilidade).

### 3. Diogo Gomes — Algoritmo próprio e geração de carga de trabalho

- `src/workload/workload_generator.h/.c` — geração de processos controlada por seed, para os
  4 cenários obrigatórios (equilibrado, I/O-bound, CPU-bound, prioridades desbalanceadas).
- `src/algorithms/custom_algorithm.c` — algoritmo próprio da equipe (renomear o arquivo
  quando o nome do algoritmo for definido).
- Documentar em `docs/MODELAGEM.md` a motivação, o funcionamento e as limitações do
  algoritmo próprio, e o modelo de chegada dos processos.
- No artigo: seção detalhada do algoritmo próprio (a parte mais importante do artigo,
  conforme o enunciado) e descrição do modelo de sistema/geração de cargas.

### 4. Cícero Jesus — Métricas, estatística e visualização

- `src/metrics/metrics.h/.c` — turnaround médio, contagem de trocas de contexto, slowdown e
  índice de Jain.
- `scripts/run_experiments.sh` — execução em lote de todas as combinações cenário x seed x
  algoritmo.
- `scripts/consolidate_results.py` e `scripts/stats.py` — consolidação dos resultados
  brutos e cálculo de média/IC95%.
- `scripts/generate_graphs.py` — geração dos gráficos (média + IC95%, com métrica, cenário,
  algoritmo e unidade de medida sempre identificados).
- No artigo: seção de metodologia experimental (cenários, seeds, número de processos,
  métricas) e de resultados (tabelas e gráficos).

## Responsabilidades compartilhadas por todos

- Revisar (via pull request) o código dos colegas antes de mesclar em `main`.
- Escrever, no artigo, a seção referente à própria frente de trabalho; introdução, discussão
  dos resultados e conclusão são escritas em conjunto.
- Dominar o trabalho completo o suficiente para a apresentação — exigência explícita do
  enunciado (todos os integrantes participam, demonstrando domínio sobre alguma parte).
- Montar os slides em conjunto; cada integrante apresenta a parte que desenvolveu.
- Preencher `docs/MODELAGEM.md` em conjunto antes de começar a implementação de cada parte,
  já que várias decisões de modelagem (prioridade, E/S, troca de contexto, chegada) afetam
  todos os algoritmos e precisam ser as mesmas para todos.

## Regras de commit

- Commits devem indicar a participação de cada integrante (exigência do enunciado, seção 2).
  Cada um deve commitar usando seu próprio nome/e-mail configurado no Git
  (`git config user.name` / `git config user.email`), para que a autoria fique correta no
  histórico.
- Sugestão de prefixo por frente de trabalho, ex.: `[nucleo]`, `[algoritmos]`, `[proprio]`,
  `[metricas]`, `[docs]` — ajuda a rastrear a contribuição de cada um no repositório.

## Cronograma sugerido (entrega final: 19/08/2026)

O prazo é curto para o escopo do trabalho (simulador em C, 4 cenários x ≥100 seeds x ≥1000
processos x 4 algoritmos, artigo científico e slides). Recomenda-se começar imediatamente
pelas decisões de modelagem, que são pré-requisito para o restante.

| Período | Foco |
|---|---|
| 10–11/08 | Definir em conjunto as decisões de `docs/MODELAGEM.md` (E/S, troca de contexto, prioridade, chegada); iniciar o núcleo da simulação. |
| 12–13/08 | Implementar os algoritmos clássicos e o gerador de carga de trabalho para os 4 cenários. |
| 14/08 | Implementar e integrar o algoritmo próprio; primeira integração completa (núcleo + todos os algoritmos). |
| 15/08 | Rodar os experimentos principais (4 cenários x ≥100 seeds x ≥1000 processos, mesmos parâmetros para todos os algoritmos); começar scripts de consolidação/gráficos. |
| 16/08 | Gerar gráficos com média + IC95%; revisar consistência dos resultados. |
| 17/08 | Escrever o artigo científico (todas as seções) e montar os slides. |
| 18/08 | Revisão final do artigo e do repositório (README, reprodutibilidade); ensaiar a apresentação. |
| 19/08 | Entrega: repositório, artigo em PDF, slides e lista de responsabilidades. |
