# Modelagem do Simulador — Decisões de Projeto

> Template a ser preenchido pela equipe. O enunciado exige explicitamente que estas decisões
> sejam definidas e documentadas, e que sejam as **mesmas para todos os algoritmos** dentro
> de um mesmo experimento/cenário (ver `docs/PROJETO.md`, seções 3, 4 e 5). O conteúdo deste
> arquivo, depois de preenchido, alimenta diretamente a seção de metodologia do artigo.

## 1. Convenção de prioridade

- [ ] Valores **menores** representam maior prioridade, ou valores **maiores**?
- Justificativa:

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
- Justificativa (obrigatória caso a chegada seja toda no instante 0, incluindo discussão da
  limitação dessa escolha):

## 5. Critérios de desempate dos algoritmos clássicos

- FCFS — desempate entre processos que chegam no mesmo instante:
- Round Robin — quantum utilizado nos experimentos principais:
- Prioridade (não preemptivo) — desempate entre processos de mesma prioridade:

## 6. Algoritmo próprio

- Nome do algoritmo:
- Qual problema o algoritmo tenta resolver:
- Quais informações o algoritmo usa para tomar decisões (deve usar apenas estado atual e
  histórico observado — nunca conhecimento futuro):
- Como o próximo processo é escolhido:
- Por que o algoritmo deveria melhorar algum aspecto da simulação:
- Quais limitações ele possui:
- Baseado em alguma ideia/artigo/fonte existente? Qual? O que foi modificado e por quê?
  (a fonte deve ser citada no artigo)

## 7. Cenários obrigatórios — parâmetros escolhidos

Para cada um dos 4 cenários obrigatórios (ver `docs/PROJETO.md`, seção 6), documentar os
parâmetros concretos usados na geração da carga (ex.: faixas de duração de rajada de CPU,
número de requisições de E/S, distribuição de prioridades):

1. Aleatório equilibrado:
2. I/O-bound:
3. CPU-bound / processos longos:
4. Prioridades desbalanceadas:
