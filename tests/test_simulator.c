#include <math.h>
#include <stdlib.h>

#include "unity.h"
#include "../src/models/simulator/simulator.h"

void setUp(void) {}
void tearDown(void) {}

/* ---------------------------------------------------------------------
 * Auxiliares: montagem de cargas de trabalho determinísticas escritas à
 * mão, com as rajadas alternando CPU -> E/S -> CPU (docs/PROJETO.md,
 * seção 3).
 * ------------------------------------------------------------------- */
static Workload make_workload(size_t count) {
    Workload workload;
    workload.processes = malloc(count * sizeof(Process));
    workload.count = count;
    TEST_ASSERT_NOT_NULL(workload.processes);
    return workload;
}

static void put_process(
    Workload *workload,
    size_t index,
    int pid,
    int arrival_time,
    int priority,
    const int *durations,
    int num_bursts
) {
    Process process = process_create(pid, arrival_time, priority, num_bursts);
    for (int burst = 0; burst < num_bursts; burst++) {
        process.bursts[burst].duration = durations[burst];
    }
    workload->processes[index] = process;
}

static SimulationConfig config_of(SchedulerAlgorithm algorithm, int quantum, int cost) {
    SimulationConfig config;
    config.algorithm = algorithm;
    config.scheduler.quantum = quantum;
    config.scheduler.context_switch_cost = cost;
    return config;
}

static void assert_double_close(double expected, double actual) {
    TEST_ASSERT_TRUE(fabs(expected - actual) <= 1e-9);
}

/* Duas rajadas de CPU sem E/S: P1 (5 unidades) e P2 (3 unidades),
 * ambos chegando no instante 0. */
static Workload two_cpu_bound_processes(void) {
    const int first[] = {5};
    const int second[] = {3};

    Workload workload = make_workload(2);
    put_process(&workload, 0, 1, 0, 1, first, 1);
    put_process(&workload, 1, 2, 0, 1, second, 1);

    return workload;
}

/* ---------------------------------------------------------------------
 * Fila de prontos
 * ------------------------------------------------------------------- */
void test_fila_de_prontos_mantem_a_ordem_de_entrada(void) {
    ReadyQueue queue;
    ready_queue_init(&queue);

    for (int pid = 1; pid <= 3; pid++) {
        Process process = process_create(pid, 0, 1, 1);
        ready_queue_push(&queue, &process, (size_t)(pid - 1));
        free(process.bursts);
    }

    TEST_ASSERT_EQUAL_UINT64(3, queue.count);
    TEST_ASSERT_EQUAL(1, queue.items[0].pid);
    TEST_ASSERT_EQUAL(2, queue.items[1].pid);
    TEST_ASSERT_EQUAL(3, queue.items[2].pid);

    ready_queue_free(&queue);
    TEST_ASSERT_EQUAL_UINT64(0, queue.count);
}

void test_fila_de_prontos_remove_por_indice_preservando_a_ordem(void) {
    ReadyQueue queue;
    ready_queue_init(&queue);

    for (int pid = 1; pid <= 3; pid++) {
        Process process = process_create(pid, 0, 1, 1);
        ready_queue_push(&queue, &process, (size_t)(pid + 9));
        free(process.bursts);
    }

    Process removed;
    size_t id = 0;
    TEST_ASSERT_TRUE(ready_queue_remove_at(&queue, 1, &removed, &id));

    TEST_ASSERT_EQUAL(2, removed.pid);
    TEST_ASSERT_EQUAL_UINT64(11, id);
    TEST_ASSERT_EQUAL_UINT64(2, queue.count);
    TEST_ASSERT_EQUAL(1, queue.items[0].pid);
    TEST_ASSERT_EQUAL(3, queue.items[1].pid);
    TEST_ASSERT_EQUAL_UINT64(12, queue.ids[1]);

    ready_queue_free(&queue);
}

void test_fila_de_prontos_rejeita_indice_invalido(void) {
    ReadyQueue queue;
    ready_queue_init(&queue);

    TEST_ASSERT_FALSE(ready_queue_remove_at(&queue, 0, NULL, NULL));

    Process process = process_create(1, 0, 1, 1);
    ready_queue_push(&queue, &process, 0);
    free(process.bursts);

    TEST_ASSERT_FALSE(ready_queue_remove_at(&queue, 1, NULL, NULL));
    TEST_ASSERT_EQUAL_UINT64(1, queue.count);

    ready_queue_free(&queue);
}

/* ---------------------------------------------------------------------
 * Fila de E/S
 * ------------------------------------------------------------------- */
void test_fila_de_es_ordena_por_instante_de_liberacao(void) {
    IoQueue queue;
    io_queue_init(&queue);

    io_queue_push(&queue, 3, 10);
    io_queue_push(&queue, 1, 5);
    io_queue_push(&queue, 2, 5); /* empate no instante: desempata pelo id */

    TEST_ASSERT_EQUAL_UINT64(1, queue.items[0].id);
    TEST_ASSERT_EQUAL_UINT64(2, queue.items[1].id);
    TEST_ASSERT_EQUAL_UINT64(3, queue.items[2].id);

    io_queue_free(&queue);
}

void test_fila_de_es_so_libera_processos_com_a_es_concluida(void) {
    IoQueue queue;
    io_queue_init(&queue);

    io_queue_push(&queue, 1, 5);
    io_queue_push(&queue, 2, 10);

    BlockedProcess released;
    TEST_ASSERT_FALSE(io_queue_pop_released(&queue, 4, &released));

    TEST_ASSERT_TRUE(io_queue_pop_released(&queue, 5, &released));
    TEST_ASSERT_EQUAL_UINT64(1, released.id);
    TEST_ASSERT_EQUAL(5, released.release_time);

    TEST_ASSERT_FALSE(io_queue_pop_released(&queue, 9, &released));
    TEST_ASSERT_TRUE(io_queue_pop_released(&queue, 10, &released));
    TEST_ASSERT_EQUAL_UINT64(2, released.id);
    TEST_ASSERT_EQUAL_UINT64(0, queue.count);

    io_queue_free(&queue);
}

/* ---------------------------------------------------------------------
 * Validação dos parâmetros de execução
 * ------------------------------------------------------------------- */
void test_simulador_rejeita_entradas_invalidas(void) {
    Workload workload = two_cpu_bound_processes();
    SimulationConfig config = config_of(SCHEDULER_FCFS, 0, 0);
    SimulationResult result = {NULL, 7, 7, 7, 7, 7, 7};

    Workload empty = {NULL, 0};
    TEST_ASSERT_FALSE(simulator_run(NULL, &config, &result));
    TEST_ASSERT_FALSE(simulator_run(&empty, &config, &result));
    TEST_ASSERT_FALSE(simulator_run(&workload, NULL, &result));
    TEST_ASSERT_FALSE(simulator_run(&workload, &config, NULL));

    /* o custo de troca de contexto é configurável e nunca negativo
     * (docs/PROJETO.md, seção 4) */
    SimulationConfig negative_cost = config_of(SCHEDULER_FCFS, 0, -1);
    TEST_ASSERT_FALSE(simulator_run(&workload, &negative_cost, &result));

    /* resultado do chamador permanece intacto quando a execução falha */
    TEST_ASSERT_EQUAL_UINT64(7, result.count);
    TEST_ASSERT_EQUAL_UINT64(7, result.context_switches);

    free_workload(&workload);
}

void test_simulador_rejeita_round_robin_sem_quantum(void) {
    Workload workload = two_cpu_bound_processes();
    SimulationConfig config = config_of(SCHEDULER_ROUND_ROBIN, 0, 0);
    SimulationResult result;

    TEST_ASSERT_FALSE(simulator_run(&workload, &config, &result));

    config = config_of(SCHEDULER_ROUND_ROBIN, 2, 0);
    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    simulator_free_result(&result);
    free_workload(&workload);
}

void test_simulador_rejeita_algoritmo_ainda_nao_implementado(void) {
    /* o algoritmo próprio (src/algorithms/custom_algorithm.c) ainda não
     * foi implementado: a simulação deve falhar em vez de executar. */
    Workload workload = two_cpu_bound_processes();
    SimulationConfig config = config_of(SCHEDULER_CUSTOM, 0, 0);
    SimulationResult result;

    TEST_ASSERT_FALSE(simulator_run(&workload, &config, &result));

    free_workload(&workload);
}

/* ---------------------------------------------------------------------
 * Laço principal: ordem de execução, término e integridade da carga
 * ------------------------------------------------------------------- */
void test_fcfs_executa_na_ordem_de_chegada(void) {
    Workload workload = two_cpu_bound_processes();
    SimulationConfig config = config_of(SCHEDULER_FCFS, 0, 0);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    TEST_ASSERT_EQUAL_UINT64(2, result.count);
    assert_double_close(5.0, result.samples[0].completion_time); /* P1: 0 -> 5 */
    assert_double_close(8.0, result.samples[1].completion_time); /* P2: 5 -> 8 */
    TEST_ASSERT_EQUAL(8, result.total_time);
    TEST_ASSERT_EQUAL(8, result.cpu_busy_time);
    TEST_ASSERT_EQUAL(0, result.idle_time);

    simulator_free_result(&result);
    free_workload(&workload);
}

void test_amostras_registram_chegada_e_tempo_minimo_ideal(void) {
    const int bursts[] = {2, 5, 3}; /* CPU 2 -> E/S 5 -> CPU 3 */
    Workload workload = make_workload(1);
    put_process(&workload, 0, 1, 4, 1, bursts, 3);

    SimulationConfig config = config_of(SCHEDULER_FCFS, 0, 0);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    assert_double_close(4.0, result.samples[0].arrival_time);
    assert_double_close(5.0, result.samples[0].total_cpu_time);
    assert_double_close(5.0, result.samples[0].total_io_time);
    assert_double_close(14.0, result.samples[0].completion_time); /* 4 + 2 + 5 + 3 */

    simulator_free_result(&result);
    free_workload(&workload);
}

void test_simulacao_nao_modifica_a_carga_de_trabalho(void) {
    Workload workload = two_cpu_bound_processes();
    SimulationConfig config = config_of(SCHEDULER_FCFS, 0, 1);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    for (size_t index = 0; index < workload.count; index++) {
        TEST_ASSERT_EQUAL(NEW, workload.processes[index].state);
        TEST_ASSERT_EQUAL(0, workload.processes[index].current_burst_index);
    }
    TEST_ASSERT_EQUAL(5, workload.processes[0].bursts[0].duration);
    TEST_ASSERT_EQUAL(3, workload.processes[1].bursts[0].duration);

    simulator_free_result(&result);
    free_workload(&workload);
}

/* ---------------------------------------------------------------------
 * Troca de contexto (docs/PROJETO.md, seção 4; docs/MODELAGEM.md, seção 3)
 * ------------------------------------------------------------------- */
void test_troca_de_contexto_consome_tempo_e_e_contabilizada(void) {
    Workload workload = two_cpu_bound_processes();
    SimulationConfig config = config_of(SCHEDULER_FCFS, 0, 2);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    TEST_ASSERT_EQUAL_UINT64(2, result.context_switches);
    TEST_ASSERT_EQUAL(4, result.context_switch_time);
    assert_double_close(7.0, result.samples[0].completion_time);  /* 2 + 5 */
    assert_double_close(12.0, result.samples[1].completion_time); /* 7 + 2 + 3 */
    TEST_ASSERT_EQUAL(12, result.total_time);
    TEST_ASSERT_EQUAL(8, result.cpu_busy_time);

    simulator_free_result(&result);
    free_workload(&workload);
}

void test_custo_zero_nao_altera_o_numero_de_trocas_de_contexto(void) {
    /* experimento complementar com custo = 0 (docs/PROJETO.md, seção 4):
     * as trocas continuam sendo contadas, apenas não consomem tempo. */
    Workload workload = two_cpu_bound_processes();
    SimulationConfig config = config_of(SCHEDULER_FCFS, 0, 0);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    TEST_ASSERT_EQUAL_UINT64(2, result.context_switches);
    TEST_ASSERT_EQUAL(0, result.context_switch_time);

    simulator_free_result(&result);
    free_workload(&workload);
}

void test_redespachar_o_mesmo_processo_nao_conta_troca_de_contexto(void) {
    /* único processo pronto: mesmo com o quantum expirando 3 vezes, o
     * processo que volta à CPU é o mesmo, logo não há troca. */
    const int bursts[] = {10};
    Workload workload = make_workload(1);
    put_process(&workload, 0, 1, 0, 1, bursts, 1);

    SimulationConfig config = config_of(SCHEDULER_ROUND_ROBIN, 3, 5);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    TEST_ASSERT_EQUAL_UINT64(1, result.context_switches);
    TEST_ASSERT_EQUAL(5, result.context_switch_time);
    TEST_ASSERT_EQUAL(10, result.cpu_busy_time);
    assert_double_close(15.0, result.samples[0].completion_time);

    simulator_free_result(&result);
    free_workload(&workload);
}

void test_cpu_fica_ociosa_ate_a_proxima_chegada(void) {
    const int first[] = {3};
    const int second[] = {2};
    Workload workload = make_workload(2);
    put_process(&workload, 0, 1, 0, 1, first, 1);
    put_process(&workload, 1, 2, 10, 1, second, 1);

    SimulationConfig config = config_of(SCHEDULER_FCFS, 0, 0);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    TEST_ASSERT_EQUAL(7, result.idle_time); /* de 3 até 10 */
    TEST_ASSERT_EQUAL(5, result.cpu_busy_time);
    TEST_ASSERT_EQUAL(12, result.total_time);
    assert_double_close(12.0, result.samples[1].completion_time);

    simulator_free_result(&result);
    free_workload(&workload);
}

/* ---------------------------------------------------------------------
 * Bloqueio por E/S (docs/PROJETO.md, seção 4; docs/MODELAGEM.md, seção 2)
 * ------------------------------------------------------------------- */
void test_processo_bloqueado_em_es_libera_a_cpu_e_volta_a_fila_de_prontos(void) {
    const int with_io[] = {2, 5, 2}; /* CPU 2 -> E/S 5 -> CPU 2 */
    const int cpu_only[] = {4};
    Workload workload = make_workload(2);
    put_process(&workload, 0, 1, 0, 1, with_io, 3);
    put_process(&workload, 1, 2, 0, 1, cpu_only, 1);

    SimulationConfig config = config_of(SCHEDULER_FCFS, 0, 0);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    /* P1 executa 0..2, bloqueia até 7; P2 aproveita a CPU em 2..6;
     * P1 volta a executar em 7..9. */
    assert_double_close(9.0, result.samples[0].completion_time);
    assert_double_close(6.0, result.samples[1].completion_time);
    TEST_ASSERT_EQUAL(1, result.idle_time); /* 6..7, esperando a E/S */
    TEST_ASSERT_EQUAL(8, result.cpu_busy_time);
    TEST_ASSERT_EQUAL_UINT64(3, result.context_switches);

    simulator_free_result(&result);
    free_workload(&workload);
}

void test_es_de_processos_diferentes_ocorrem_em_paralelo(void) {
    /* modelagem adotada: sem disputa por dispositivo de E/S, os dois
     * processos ficam bloqueados ao mesmo tempo. */
    const int bursts[] = {1, 10, 1};
    Workload workload = make_workload(2);
    put_process(&workload, 0, 1, 0, 1, bursts, 3);
    put_process(&workload, 1, 2, 0, 1, bursts, 3);

    SimulationConfig config = config_of(SCHEDULER_FCFS, 0, 0);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    /* P1: CPU 0..1, E/S 1..11; P2: CPU 1..2, E/S 2..12. Se as E/S
     * fossem serializadas, o término seria bem mais tarde. */
    assert_double_close(12.0, result.samples[0].completion_time);
    assert_double_close(13.0, result.samples[1].completion_time);

    simulator_free_result(&result);
    free_workload(&workload);
}

/* ---------------------------------------------------------------------
 * Preempção por quantum (Round Robin)
 * ------------------------------------------------------------------- */
void test_round_robin_alterna_os_processos_a_cada_quantum(void) {
    Workload workload = two_cpu_bound_processes();
    SimulationConfig config = config_of(SCHEDULER_ROUND_ROBIN, 2, 0);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    /* fatias: P1 0..2, P2 2..4, P1 4..6, P2 6..7 (fim), P1 7..8 (fim) */
    assert_double_close(8.0, result.samples[0].completion_time);
    assert_double_close(7.0, result.samples[1].completion_time);
    TEST_ASSERT_EQUAL_UINT64(5, result.context_switches);
    TEST_ASSERT_EQUAL(8, result.cpu_busy_time);

    simulator_free_result(&result);
    free_workload(&workload);
}

void test_round_robin_gera_mais_trocas_de_contexto_que_fcfs(void) {
    Workload workload = two_cpu_bound_processes();
    SimulationResult round_robin;
    SimulationResult fcfs;

    SimulationConfig rr_config = config_of(SCHEDULER_ROUND_ROBIN, 2, 1);
    SimulationConfig fcfs_config = config_of(SCHEDULER_FCFS, 0, 1);

    TEST_ASSERT_TRUE(simulator_run(&workload, &rr_config, &round_robin));
    TEST_ASSERT_TRUE(simulator_run(&workload, &fcfs_config, &fcfs));

    TEST_ASSERT_TRUE(round_robin.context_switches > fcfs.context_switches);

    simulator_free_result(&round_robin);
    simulator_free_result(&fcfs);
    free_workload(&workload);
}

/* ---------------------------------------------------------------------
 * Escalonamento por prioridade (docs/MODELAGEM.md, seções 1 e 5)
 * ------------------------------------------------------------------- */
void test_prioridade_executa_primeiro_o_maior_valor(void) {
    const int bursts[] = {2};
    Workload workload = make_workload(3);
    put_process(&workload, 0, 1, 0, 1, bursts, 1);
    put_process(&workload, 1, 2, 0, 5, bursts, 1);
    put_process(&workload, 2, 3, 0, 9, bursts, 1);

    SimulationConfig config = config_of(SCHEDULER_PRIORITY, 0, 0);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    assert_double_close(2.0, result.samples[2].completion_time); /* prioridade 9 */
    assert_double_close(4.0, result.samples[1].completion_time); /* prioridade 5 */
    assert_double_close(6.0, result.samples[0].completion_time); /* prioridade 1 */

    simulator_free_result(&result);
    free_workload(&workload);
}

void test_prioridade_nao_preempta_o_processo_em_execucao(void) {
    const int running[] = {5};
    const int arriving[] = {2};
    Workload workload = make_workload(2);
    put_process(&workload, 0, 1, 0, 1, running, 1);
    put_process(&workload, 1, 2, 1, 9, arriving, 1);

    SimulationConfig config = config_of(SCHEDULER_PRIORITY, 0, 0);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    /* P2 tem prioridade maior e chega em t=1, mas só executa quando P1
     * termina a rajada de CPU (algoritmo não preemptivo). */
    assert_double_close(5.0, result.samples[0].completion_time);
    assert_double_close(7.0, result.samples[1].completion_time);

    simulator_free_result(&result);
    free_workload(&workload);
}

/* ---------------------------------------------------------------------
 * Integração com o gerador de carga e com as métricas
 * ------------------------------------------------------------------- */
void test_todos_os_processos_da_carga_gerada_terminam(void) {
    const SchedulerAlgorithm algorithms[] = {
        SCHEDULER_FCFS,
        SCHEDULER_ROUND_ROBIN,
        SCHEDULER_PRIORITY,
    };

    Workload workload = generate_workload(SCENARIO_BALANCED, 42, 300);

    for (size_t index = 0; index < 3; index++) {
        /* quantum 4 e custo > 0: parâmetros dos experimentos principais
         * (docs/MODELAGEM.md, seção 5). */
        SimulationConfig config = config_of(algorithms[index], 4, 1);
        SimulationResult result;

        TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));
        TEST_ASSERT_EQUAL_UINT64(workload.count, result.count);

        for (size_t sample = 0; sample < result.count; sample++) {
            const ProcessMetricsSample *current = &result.samples[sample];
            const double ideal = current->total_cpu_time + current->total_io_time;

            /* nenhum processo pode terminar antes de executar todo o seu
             * tempo de CPU e de E/S */
            TEST_ASSERT_TRUE(current->completion_time >= current->arrival_time + ideal);
        }

        TEST_ASSERT_TRUE(result.cpu_busy_time > 0);
        TEST_ASSERT_TRUE(result.total_time > 0);

        simulator_free_result(&result);
    }

    free_workload(&workload);
}

void test_mesma_carga_e_configuracao_produzem_o_mesmo_resultado(void) {
    Workload workload = generate_workload(SCENARIO_IO_BOUND, 7, 200);
    SimulationConfig config = config_of(SCHEDULER_ROUND_ROBIN, 4, 1);

    SimulationResult first;
    SimulationResult second;
    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &first));
    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &second));

    TEST_ASSERT_EQUAL_UINT64(first.context_switches, second.context_switches);
    TEST_ASSERT_EQUAL(first.total_time, second.total_time);
    TEST_ASSERT_EQUAL(first.idle_time, second.idle_time);
    for (size_t index = 0; index < first.count; index++) {
        assert_double_close(
            first.samples[index].completion_time,
            second.samples[index].completion_time
        );
    }

    simulator_free_result(&first);
    simulator_free_result(&second);
    free_workload(&workload);
}

void test_resultado_alimenta_o_calculo_das_metricas(void) {
    Workload workload = two_cpu_bound_processes();
    SimulationConfig config = config_of(SCHEDULER_FCFS, 0, 0);
    SimulationResult result;
    SimulationMetrics metrics;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));
    TEST_ASSERT_TRUE(metrics_calculate(
        result.samples,
        result.count,
        result.context_switches,
        &metrics
    ));

    assert_double_close(6.5, metrics.mean_turnaround); /* (5 + 8) / 2 */
    TEST_ASSERT_EQUAL_UINT64(2, metrics.context_switches);
    TEST_ASSERT_TRUE(metrics.jain_slowdown_percent > 0.0);
    TEST_ASSERT_TRUE(metrics.jain_slowdown_percent <= 100.0);

    simulator_free_result(&result);
    free_workload(&workload);
}

void test_liberar_o_resultado_e_seguro_e_idempotente(void) {
    Workload workload = two_cpu_bound_processes();
    SimulationConfig config = config_of(SCHEDULER_FCFS, 0, 0);
    SimulationResult result;

    TEST_ASSERT_TRUE(simulator_run(&workload, &config, &result));

    simulator_free_result(&result);
    TEST_ASSERT_NULL(result.samples);
    TEST_ASSERT_EQUAL_UINT64(0, result.count);

    simulator_free_result(&result);
    simulator_free_result(NULL);

    free_workload(&workload);
}

void test_nome_do_algoritmo_identifica_a_execucao(void) {
    TEST_ASSERT_EQUAL_STRING("fcfs", simulator_algorithm_name(SCHEDULER_FCFS));
    TEST_ASSERT_EQUAL_STRING("round_robin", simulator_algorithm_name(SCHEDULER_ROUND_ROBIN));
    TEST_ASSERT_EQUAL_STRING("priority", simulator_algorithm_name(SCHEDULER_PRIORITY));
    TEST_ASSERT_EQUAL_STRING("custom", simulator_algorithm_name(SCHEDULER_CUSTOM));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_fila_de_prontos_mantem_a_ordem_de_entrada);
    RUN_TEST(test_fila_de_prontos_remove_por_indice_preservando_a_ordem);
    RUN_TEST(test_fila_de_prontos_rejeita_indice_invalido);

    RUN_TEST(test_fila_de_es_ordena_por_instante_de_liberacao);
    RUN_TEST(test_fila_de_es_so_libera_processos_com_a_es_concluida);

    RUN_TEST(test_simulador_rejeita_entradas_invalidas);
    RUN_TEST(test_simulador_rejeita_round_robin_sem_quantum);
    RUN_TEST(test_simulador_rejeita_algoritmo_ainda_nao_implementado);

    RUN_TEST(test_fcfs_executa_na_ordem_de_chegada);
    RUN_TEST(test_amostras_registram_chegada_e_tempo_minimo_ideal);
    RUN_TEST(test_simulacao_nao_modifica_a_carga_de_trabalho);

    RUN_TEST(test_troca_de_contexto_consome_tempo_e_e_contabilizada);
    RUN_TEST(test_custo_zero_nao_altera_o_numero_de_trocas_de_contexto);
    RUN_TEST(test_redespachar_o_mesmo_processo_nao_conta_troca_de_contexto);
    RUN_TEST(test_cpu_fica_ociosa_ate_a_proxima_chegada);

    RUN_TEST(test_processo_bloqueado_em_es_libera_a_cpu_e_volta_a_fila_de_prontos);
    RUN_TEST(test_es_de_processos_diferentes_ocorrem_em_paralelo);

    RUN_TEST(test_round_robin_alterna_os_processos_a_cada_quantum);
    RUN_TEST(test_round_robin_gera_mais_trocas_de_contexto_que_fcfs);

    RUN_TEST(test_prioridade_executa_primeiro_o_maior_valor);
    RUN_TEST(test_prioridade_nao_preempta_o_processo_em_execucao);

    RUN_TEST(test_todos_os_processos_da_carga_gerada_terminam);
    RUN_TEST(test_mesma_carga_e_configuracao_produzem_o_mesmo_resultado);
    RUN_TEST(test_resultado_alimenta_o_calculo_das_metricas);
    RUN_TEST(test_liberar_o_resultado_e_seguro_e_idempotente);
    RUN_TEST(test_nome_do_algoritmo_identifica_a_execucao);

    return UNITY_END();
}
