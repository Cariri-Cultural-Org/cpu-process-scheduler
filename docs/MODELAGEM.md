# Modelagem do Simulador — Decisões de Projeto (Metodologia)

## 1. Convenção de prioridade

- Valores **maiores** representam maior prioridade.
- Justificativa: representa uma modelagem mais intuitíva.

## 2. Modelagem de E/S (Entrada/Saída)

- Quando um processo solicita E/S:
- Por quanto tempo ele permanece bloqueado:
- Múltiplas operações de E/S podem ocorrer em paralelo?
- Existe um ou mais dispositivos de E/S, cada um com fila própria?
- Quando o processo retorna à fila de prontos:
- Discussão: como essa modelagem pode influenciar os resultados obtidos:

## 3. Modelagem da troca de contexto

- Quando ocorre:
- Quanto tempo consome (valor configurável, **> 0** nos experimentos principais):
- A CPU fica indisponível para execução de processos durante a troca?
- A troca é contabilizada quando a CPU sai do estado ocioso para executar um processo?
- Valor usado nos experimentos principais:
- (Opcional) valor usado nos experimentos complementares com custo de troca de contexto = 0:

## 4. Modelo de chegada dos processos

- Todos chegam no instante 0 / intervalos fixos / intervalos aleatórios / em lotes?
  **Intervalos aleatórios**: o tempo entre chegadas consecutivas segue distribuição exponencial
  (processo de chegada de Poisson), com taxa média `λ` configurável por cenário — a mesma seed,
  no mesmo cenário, deve sempre gerar a mesma sequência de instantes de chegada. `λ` pode variar
  entre cenários (ex.: cenários mais carregados usam `λ` maior) mas é o **mesmo `λ` do cenário
  para todos os algoritmos comparados**, conforme exigido pelo enunciado.
- Justificativa (obrigatória caso a chegada seja toda no instante 0, incluindo discussão da
  limitação dessa escolha): não se aplica — a equipe optou por chegada em intervalos aleatórios
  em vez de todos no instante 0. Justificativa da escolha: chegada toda no instante 0 reduz o
  cenário a "todos os processos já estão na fila desde o início", o que elimina qualquer efeito
  de escalonamento *online* (decisões tomadas sem saber quem ainda vai chegar) — justamente o
  aspecto mais realista de um SO e o que mais diferencia os algoritmos entre si (em especial o
  algoritmo próprio, cuja estimativa de rajada só faz sentido quando processos chegam ao longo
  do tempo). Um processo de Poisson é o modelo padrão de chegada em teoria de filas/sistemas
  operacionais, simples de parametrizar por seed e de justificar na metodologia do artigo.

## 5. Critérios de desempate dos algoritmos clássicos

- FCFS — desempate entre processos que chegam no mesmo instante: em caso de empate no tempo
  de chegada, executa primeiro o processo com menor `pid`; se ainda houver empate, mantém-se a
  ordem em que os processos aparecem na fila de prontos.
- Round Robin — quantum utilizado nos experimentos principais: `4` unidades de tempo. Quando
  o quantum expira e a rajada de CPU ainda não terminou, o processo volta ao final da fila de
  prontos e a troca de contexto é contabilizada pela simulação.
- Prioridade (não preemptivo) — desempate entre processos de mesma prioridade: como valores
  maiores representam maior prioridade, escolhe-se primeiro o maior `priority`; em caso de
  empate, aplica-se FCFS (menor tempo de chegada) e, persistindo o empate, menor `pid`.

## 6. Algoritmo próprio

- Nome do algoritmo: **SJF Preditivo com Aging** (`predictive_sjf` / arquivo
  `src/algorithms/predictive_sjf.c`, renomeando `custom_algorithm.c`).
- Qual problema o algoritmo tenta resolver: o SJF clássico minimiza o turnaround médio, mas
  exige conhecer a duração da rajada de CPU *antes* dela executar — informação que um SO real
  não tem. O algoritmo próprio tenta obter o mesmo benefício (priorizar processos "curtos")
  usando apenas uma **estimativa** construída a partir do comportamento passado de cada
  processo, e usa aging para não deixar processos historicamente "longos" passarem fome.
- Quais informações o algoritmo usa para tomar decisões (apenas estado atual e histórico
  observado — nunca conhecimento futuro):
  - Para cada processo: a duração real de cada rajada de CPU já concluída por ele (histórico
    próprio, não de outros processos).
  - O tempo de espera acumulado do processo na fila de prontos (para o aging).
  - Nunca é usada a duração da rajada atual/futura do processo, nem informação de processos
    que ainda vão chegar.
- Como o próximo processo é escolhido:
  1. Cada processo mantém uma estimativa τ da duração da sua próxima rajada de CPU, atualizada
     ao final de cada rajada real `t` por média exponencial:
     `τ(n+1) = α · t(n) + (1 − α) · τ(n)`, com `0 < α ≤ 1` configurável (valor inicial proposto:
     `α = 0,5`).
  2. Para a primeira rajada de um processo (sem histórico), usa-se uma estimativa inicial
     `τ(0)` = duração média das rajadas já observadas em todos os processos até aquele momento
     na mesma execução (fallback: um valor padrão de `config.h` caso ainda não haja nenhuma
     rajada observada).
  3. Prioridade efetiva de escalonamento = `τ estimado − β · tempo_de_espera`, onde `β` é o
     fator de aging (configurável; valor inicial proposto `β = 0,1`). Entre os processos
     prontos, escolhe-se o de **menor** prioridade efetiva. Atenção: esse "menor" é interno ao
     algoritmo (menor rajada estimada vence, como no SJF) e **não** usa o campo `priority` do
     processo nem a convenção da seção 1 (valores maiores = maior prioridade) — as duas noções
     de prioridade são independentes neste algoritmo.
  4. Seleção não-preemptiva: como Priority e FCFS, a escolha só ocorre quando a CPU fica livre
     (fim de rajada, bloqueio por E/S ou preempção por chegada não se aplica aqui).
- Por que o algoritmo deveria melhorar algum aspecto da simulação: espera-se turnaround médio
  menor que FCFS e Priority (aproximando-se do SJF ideal à medida que as estimativas convergem
  para o comportamento real de cada processo), com melhor equilíbrio (Jain do slowdown) que um
  SJF preditivo "puro" sem aging, já que o aging evita que processos com histórico de rajadas
  longas fiquem indefinidamente atrás de processos "curtos" recém-chegados.
- Quais limitações ele possui:
  - A estimativa da primeira rajada de cada processo é necessariamente imprecisa (não há
    histórico ainda).
  - Sensível à escolha de `α` (memória curta vs. longa do histórico) e `β` (força do aging);
    exige ajuste empírico durante os experimentos.
  - Ainda pode sofrer do "efeito comboio" do SJF clássico em menor grau, caso vários processos
    "longos" cheguem e acumulem histórico antes de processos "curtos" chegarem.
  - Overhead de manter estado (τ, tempo de espera) por processo — desprezível computacionalmente,
    mas é uma diferença estrutural em relação aos clássicos, que não mantêm esse tipo de estado.
- Baseado em alguma ideia/artigo/fonte existente? Qual? O que foi modificado e por quê?
  A estimativa de rajada por média exponencial é a técnica clássica de previsão de próxima
  rajada de CPU descrita em Silberschatz, Galvin e Gagne, *Operating System Concepts* (a citar
  no artigo). A modificação/contribuição da equipe é a combinação dessa estimativa com um termo
  de aging baseado no tempo de espera, formando uma prioridade efetiva híbrida — o SJF preditivo
  "puro" da literatura não inclui esse mecanismo anti-starvation.

## 7. Cenários obrigatórios — parâmetros escolhidos

Para cada um dos 4 cenários obrigatórios (ver `docs/PROJETO.md`, seção 6), documentar os
parâmetros concretos usados na geração da carga (ex.: faixas de duração de rajada de CPU,
número de requisições de E/S, distribuição de prioridades):

> Parâmetros propostos abaixo (unidades de tempo abstratas, consistentes com o restante da
> simulação) — a validar com o time antes da implementação de `workload_generator.c`. Todos os
> cenários usam ≥1000 processos e ≥100 seeds nos experimentos principais (seção 5).

1. Aleatório equilibrado: número de rajadas de CPU por processo uniforme em [1, 5]; duração de
   cada rajada de CPU uniforme em [1, 20]; número de requisições de E/S por processo = número de
   rajadas de CPU − 1 (uniforme em [0, 4], alternando CPU→E/S→CPU); duração de cada
   E/S uniforme em [5, 15]; prioridade uniforme em [1, 10]; taxa de chegada `λ` moderada (mistura
   representativa de curtos/longos e com/sem E/S).
2. I/O-bound: número de rajadas de CPU por processo uniforme em [3, 7] (mais trocas com E/S);
   duração de cada rajada de CPU uniforme em [1, 5] (rajadas curtas); número de requisições de
   E/S uniforme em [2, 6]; duração de cada E/S uniforme em [10, 30] (bloqueios mais longos);
   prioridade uniforme em [1, 10].
3. CPU-bound / processos longos: número de rajadas de CPU por processo uniforme em [1, 2];
   duração de cada rajada de CPU uniforme em [20, 50] (rajadas longas); número de requisições de
   E/S uniforme em [0, 1]; duração de cada E/S uniforme em [5, 15]; prioridade uniforme em
   [1, 10].
4. Prioridades desbalanceadas: mesma distribuição de rajadas/E/S do cenário 1 (aleatório
   equilibrado), variando apenas a prioridade: seguindo a convenção da seção 1 (valores maiores
   = maior prioridade), 80% dos processos com prioridade "baixa" em [1, 3] e 20% com prioridade
   "alta" em [8, 10].
