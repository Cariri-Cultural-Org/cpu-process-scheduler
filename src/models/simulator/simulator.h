#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdbool.h>
#include <stddef.h>

#include "../metrics/metrics.h"
#include "../process/process.h"
#include "../scheduler.h"
#include "../workload/workload_generator.h"

/* =====================================================================
 * simulator.h
 * =====================================================================
 *
 * Frente responsável: Alan Mendes (núcleo da simulação).
 *
 * Objetivo:
 *   Interface do núcleo da simulação: o laço principal que avança o
 *   tempo, gerencia a fila de prontos, os bloqueios por E/S e a troca
 *   de contexto, delegando ao algoritmo de escalonamento ativo
 *   (scheduler.h) a decisão de qual processo executar.
 *
 * Modelo adotado (ver docs/PROJETO.md, seção 4, e docs/MODELAGEM.md):
 *   - Tempo discreto, em unidades inteiras, avançado por eventos: o
 *     relógio salta direto para o próximo instante relevante (fim de
 *     rajada, fim de quantum, chegada de processo ou fim de E/S), o
 *     que é equivalente a percorrer o tempo unidade a unidade.
 *   - Uma única CPU. A fila de prontos contém apenas processos aptos a
 *     usar a CPU e é mantida em ordem de entrada (FIFO); cada
 *     algoritmo aplica seu próprio critério sobre ela.
 *   - E/S com paralelismo ilimitado: cada processo bloqueado tem seu
 *     próprio dispositivo, não há disputa por dispositivo de E/S. O
 *     processo bloqueia pela duração da rajada de E/S e retorna à fila
 *     de prontos no instante exato em que ela termina.
 *   - Empates de instante: em um mesmo instante entram na fila de
 *     prontos primeiro os processos que retornam da E/S, depois os que
 *     chegam ao sistema e, por último, o processo que acabou de deixar
 *     a CPU por fim de quantum.
 *   - Troca de contexto: contabilizada (e paga) sempre que a CPU passa
 *     a executar um processo diferente do último que ocupou a CPU,
 *     inclusive no primeiro despacho da simulação. Redespachar o mesmo
 *     processo (ex.: quantum expirado sem outro processo pronto) não
 *     conta como troca. Durante a troca a CPU fica indisponível pelo
 *     custo configurado.
 *   - Preempção: apenas por fim de quantum, no Round Robin. Os demais
 *     algoritmos são não preemptivos — o processo só deixa a CPU ao
 *     terminar a rajada de CPU atual (para E/S ou para finalizar).
 * ===================================================================== */

/* ---------------------------------------------------------------------
 * Fila de prontos
 *
 * Guarda um vetor contíguo de Process (formato exigido pelo contrato de
 * scheduler.h) e, em paralelo, o identificador interno de cada
 * processo: o índice dele em Workload.processes. As entradas do vetor
 * são cópias do estado do processo no momento em que ele entrou na
 * fila; o estado vivo é mantido pelo simulador.
 * ------------------------------------------------------------------- */
typedef struct {
    Process *items;
    size_t *ids;
    size_t count;
    size_t capacity;
} ReadyQueue;

void ready_queue_init(ReadyQueue *queue);
void ready_queue_push(ReadyQueue *queue, const Process *process, size_t id);
bool ready_queue_remove_at(
    ReadyQueue *queue,
    size_t index,
    Process *out_process,
    size_t *out_id
);
void ready_queue_free(ReadyQueue *queue);

/* ---------------------------------------------------------------------
 * Fila de E/S
 *
 * Como não há disputa por dispositivo, basta manter os processos
 * bloqueados ordenados pelo instante em que a E/S termina (desempate
 * pelo identificador interno), liberando-os em ordem.
 * ------------------------------------------------------------------- */
typedef struct {
    size_t id;
    int release_time;
} BlockedProcess;

typedef struct {
    BlockedProcess *items;
    size_t count;
    size_t capacity;
} IoQueue;

void io_queue_init(IoQueue *queue);
void io_queue_push(IoQueue *queue, size_t id, int release_time);
bool io_queue_pop_released(IoQueue *queue, int now, BlockedProcess *out);
void io_queue_free(IoQueue *queue);

/* ---------------------------------------------------------------------
 * Configuração e resultado de uma execução
 * ------------------------------------------------------------------- */
typedef struct {
    SchedulerAlgorithm algorithm;
    SchedulerConfig scheduler; /* quantum e custo de troca de contexto */
} SimulationConfig;

/* Dados brutos consumidos por src/models/metrics/. samples traz uma
 * amostra por processo, na mesma ordem de Workload.processes. */
typedef struct {
    ProcessMetricsSample *samples;
    size_t count;
    size_t context_switches;
    int total_time;          /* instante em que o último processo terminou */
    int cpu_busy_time;       /* tempo de CPU efetivamente executando processos */
    int context_switch_time; /* tempo de CPU consumido pelas trocas de contexto */
    int idle_time;           /* tempo de CPU ociosa (sem processo pronto) */
} SimulationResult;

/* Executa a simulação completa da carga com o algoritmo escolhido.
 * Retorna false (sem tocar em *result) para entradas inválidas: carga
 * vazia, custo de troca de contexto negativo, Round Robin sem quantum
 * positivo ou algoritmo ainda não implementado. Em caso de sucesso, o
 * chamador deve liberar *result com simulator_free_result(). */
bool simulator_run(
    const Workload *workload,
    const SimulationConfig *config,
    SimulationResult *result
);

void simulator_free_result(SimulationResult *result);

const char *simulator_algorithm_name(SchedulerAlgorithm algorithm);

#endif /* SIMULATOR_H */
