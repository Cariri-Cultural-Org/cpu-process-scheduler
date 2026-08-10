# Documentação do Projeto — Simulador de Escalonamento de Processos

> Resumo organizado do enunciado do professor (Sistemas Operacionais, Projeto da Unidade 3,
> última etapa avaliativa), para consulta rápida pela equipe. Em caso de dúvida ou
> divergência, o enunciado original enviado pelo professor prevalece.

## 1. Objetivo

Desenvolver um simulador de escalonamento de processos capaz de:

- Gerar cargas de trabalho controladas por seed (reprodutíveis).
- Executar algoritmos clássicos de escalonamento.
- Comparar os resultados com um algoritmo proposto pela própria equipe.

Isso envolve:

- Aplicar conceitos de estados de processos, filas de prontos, bloqueio por E/S, preempção e
  troca de contexto.
- Comparar políticas de escalonamento com base em métricas quantitativas.
- Produzir um relatório em formato de artigo científico, com metodologia, resultados e
  conclusão.

## 2. Organização geral

- Código versionado em Git/GitHub, com commits que indiquem a participação de cada
  integrante.
- Simulador implementado em **C**.
- Scripts auxiliares (gráficos, consolidação de resultados, análise estatística) podem ser
  feitos em outra linguagem (neste projeto: Python), desde que documentados no repositório.

## 3. Modelo de processo

Cada processo é uma entidade simulada com, no mínimo:

- Identificador do processo.
- Tempo de chegada.
- Prioridade — **a equipe deve definir e documentar** se valores menores ou maiores
  representam maior prioridade, e usar essa mesma convenção em todos os experimentos (ver
  `docs/MODELAGEM.md`).
- Duração total de CPU ou lista de rajadas de CPU.
- Número de requisições de E/S e duração de cada uma.
- Estado atual: **novo, pronto, executando, bloqueado ou finalizado**.

Modelo comum: sequência de rajadas `CPU -> E/S -> CPU -> E/S -> CPU`.

## 4. Simulação

- Tempo discreto ou simulação por eventos.
- A fila de prontos contém apenas processos aptos a usar a CPU.
- Ao solicitar E/S, o processo sai da CPU e entra em estado bloqueado; ao terminar a E/S,
  retorna à fila de prontos.
- **A equipe deve definir e documentar** (em `docs/MODELAGEM.md`) como a E/S foi modelada:
  quando é solicitada, por quanto tempo bloqueia, se há E/S em paralelo, se há um ou mais
  dispositivos com fila própria, e quando o processo retorna à fila de prontos. A mesma
  modelagem deve valer para todos os algoritmos avaliados.
- **Custo de troca de contexto**: configurável e **maior que zero** nos experimentos
  principais, com o mesmo valor para todos os algoritmos em um mesmo experimento. A equipe
  deve documentar quando ocorre, quanto tempo consome, se a CPU fica indisponível durante a
  troca e se ela é contabilizada quando a CPU sai do estado ocioso. Experimentos adicionais
  com custo zero são permitidos como análise complementar.

## 5. Geração das cargas de trabalho

Geradas aleatoriamente a partir de uma seed; a mesma seed, no mesmo cenário, deve gerar
sempre a mesma carga.

**Configuração mínima (experimentos principais):**

- Pelo menos 1.000 processos por execução.
- Pelo menos 100 seeds por cenário.
- Mesmos cenários e mesmas seeds para todos os algoritmos.

**Configuração ampliada (opcional):** 10.000+ processos, mais de 100 seeds, cenários
adicionais ou análises de sensibilidade — não substitui a configuração mínima.

O modelo de chegada dos processos (todos no instante 0, intervalos fixos, aleatórios ou em
lotes) também deve ser gerado pela seed, documentado, e ser o mesmo para todos os algoritmos
dentro de um cenário. Chegada toda no instante 0 só é permitida se justificada e discutida no
relatório.

## 6. Cenários obrigatórios

1. **Aleatório equilibrado**: mistura de processos curtos, longos, com pouca e muita E/S.
2. **I/O-bound**: rajadas de CPU menores, mais requisições de E/S.
3. **CPU-bound / processos longos**: rajadas de CPU maiores, menos requisições de E/S.
4. **Prioridades desbalanceadas**: muitos processos com prioridade alta e alguns com
   prioridade baixa.

## 7. Seeds e rigor estatístico

- Mínimo obrigatório: **100 seeds diferentes por cenário**.
- Meta recomendada: 1000 seeds por cenário, se o tempo de execução permitir.
- Para cada métrica: média e Intervalo de Confiança (IC) de 95%.
- Gráficos devem mostrar média e IC (sombreado, barras de erro ou outra forma).

Fórmula sugerida:

```
IC95% = x̄ ± 1,96 · (s / √n)
```

onde `x̄` é a média amostral, `s` o desvio padrão amostral e `n` o número de execuções
independentes (ex.: número de seeds usadas).

## 8. Algoritmos de escalonamento

**Clássicos obrigatórios:**

- FCFS (First-Come, First-Served).
- Round Robin, com quantum configurável.
- Escalonamento por Prioridade não preemptivo.
- (Opcional) algoritmos adicionais como comparação extra, desde que a equipe explique quais
  informações usam e se representam conhecimento futuro.

**Algoritmo próprio:** cada equipe propõe e implementa um algoritmo com contribuição própria
(criação original, adaptação ou combinação de estratégias existentes), com diferença clara
em relação aos algoritmos clássicos e a qualquer inspiração usada. Ideias de terceiros devem
ser citadas no relatório; não é aceito copiar/renomear um algoritmo existente sem alteração.

Para o algoritmo próprio, documentar (ver `docs/MODELAGEM.md`):

- Qual problema tenta resolver.
- Quais informações usa para decidir.
- Como o próximo processo é escolhido.
- Por que deveria melhorar algum aspecto da simulação.
- Quais limitações possui.
- Se baseado em ideia conhecida: qual, o que foi modificado e por quê.

**Restrição importante**: o algoritmo não pode usar informações futuras que um SO real não
teria no momento do escalonamento — decisões apenas com base no estado atual e no histórico
já observado.

## 9. Resultados esperados — Métricas obrigatórias

| Métrica | O que mede |
|---|---|
| Turnaround médio | Tempo médio entre chegada e término dos processos; desempenho geral. |
| Trocas de contexto | Quantidade de mudanças de processo executando na CPU; custo do escalonador. |
| Jain do slowdown | Grau de equilíbrio entre os slowdowns dos processos; indicador de justiça. |

**Slowdown** de um processo:

```
slowdown_i = turnaround_i / tempo_mínimo_ideal_i
tempo_mínimo_ideal_i = Σ CPU_i + Σ E/S_i
```

**Índice de Jain** aplicado aos slowdowns (n = número de processos simulados):

```
Jain_slowdown(%) = (Σ slowdown_i)² / (n · Σ slowdown_i²) × 100
```

Valores próximos de 100% indicam slowdowns semelhantes entre processos (mais justo); valores
menores indicam maior desigualdade. A justiça deve ser interpretada **junto** com o
turnaround médio: um algoritmo pode ser "justo" por prejudicar todos igualmente, mas ainda
ter desempenho ruim.

## 10. Gráficos e comparações

- Comparar o algoritmo próprio com **todos** os algoritmos clássicos obrigatórios, em
  **todos** os cenários obrigatórios.
- Gráficos com média e IC95% para as métricas principais.
- Discussão focada no algoritmo próprio: em quais cenários/métricas foi melhor ou pior, que
  custos introduziu (ex.: mais trocas de contexto, maior turnaround) e por quê.
- Evitar conclusões vagas ("o algoritmo X é melhor"): ligar sempre ao cenário, à métrica e à
  comparação com os clássicos.
- Todo gráfico deve identificar claramente métrica, cenários, algoritmos e unidade de medida.

## 11. Relatório em formato de artigo

- Algoritmos clássicos: descrição resumida, só o suficiente para reprodutibilidade (critérios
  de desempate, quantum do Round Robin, interpretação da prioridade etc.).
- Algoritmo próprio: descrição detalhada (motivação, funcionamento, informações usadas,
  diferenças em relação aos clássicos, limitações).
- **4 a 6 páginas**, contendo: título/autores/resumo; introdução e motivação; descrição do
  modelo de sistema; descrição dos algoritmos (incluindo o próprio); metodologia experimental
  (cenários, seeds, número de processos, métricas); resultados (tabelas e gráficos);
  discussão; conclusão; referências (quando usadas).
- Templates sugeridos pelo professor: formato IEEE
  (https://www.ieee.org/conferences/publishing/templates), além de links específicos para
  Overleaf/Word citados no enunciado original — conferir com o professor caso não estejam
  acessíveis a partir do PDF do enunciado.

## 12. Entregáveis

- Link do repositório Git/GitHub.
- Relatório em formato de artigo, em **PDF**.
- Slides da apresentação.
- Lista de integrantes e responsabilidades (`docs/DIVISAO_TAREFAS.md`).

## 13. Apresentação

- Apresentar motivação, modelo de simulação, algoritmo próprio e principais resultados,
  defendendo as decisões tomadas e explicando os gráficos.
- Avaliação considera se a equipe sabe **interpretar** os resultados, não só mostrar que o
  programa roda.
- Presença obrigatória na apresentação das demais equipes (falta reduz nota individual).
- Todos os integrantes participam, demonstrando domínio sobre alguma parte do trabalho.
- Duração: 10 a 12 minutos (apresentações mais longas podem ser interrompidas).
- Ordem definida pelo professor no dia.

## 14. Critérios de avaliação

| Critério | Pontos | O que será observado |
|---|---|---|
| Artigo em formato científico | 6,0 | Clareza do problema, modelo de simulação, algoritmo próprio, metodologia, gráficos com IC95%, discussão crítica, conclusão ligada aos resultados. |
| Apresentação e defesa | 3,0 | Domínio do projeto, explicação do algoritmo próprio, interpretação dos gráficos, clareza na comparação com os algoritmos de referência, tempo. |
| Reprodutibilidade e organização do repositório | 1,0 | Repositório organizado, README, seeds, comandos/scripts de execução e resultados consolidados usados no artigo. |

## 15. Datas importantes

| Data | Detalhes |
|---|---|
| 19/08/2026 | Entrega do trabalho e primeiro dia de apresentações |
| 21/08/2026 | Segundo dia de apresentações |

A entrega deve ser realizada até 19/08/2026, independentemente da data de apresentação da
equipe.
