#include "unity.h"
#include "../src/models/scheduler.h"

void setUp(void) {}
void tearDown(void) {}

static Process make_process(int pid, int arrival_time, int priority, enum Process_possible_states state) {
    Process p = {
        .pid = pid,
        .arrival_time = arrival_time,
        .priority = priority,
        .state = state,
        .bursts = NULL,
        .num_bursts = 0,
        .current_burst_index = 0,
    };
    return p;
}

void test_fcfs_escolhe_menor_tempo_de_chegada(void) {
    Process ready_queue[] = {
        make_process(1, 8, 5, READY),
        make_process(2, 3, 5, READY),
        make_process(3, 5, 5, READY),
    };

    TEST_ASSERT_EQUAL(1, scheduler_select_fcfs(ready_queue, 3, 0, NULL));
}

void test_fcfs_desempata_por_menor_pid(void) {
    Process ready_queue[] = {
        make_process(7, 4, 5, READY),
        make_process(2, 4, 5, READY),
        make_process(5, 4, 5, READY),
    };

    TEST_ASSERT_EQUAL(1, scheduler_select_fcfs(ready_queue, 3, 0, NULL));
}

void test_fcfs_ignora_processos_que_nao_estao_prontos(void) {
    Process ready_queue[] = {
        make_process(1, 1, 5, BLOCKED),
        make_process(2, 2, 5, READY),
        make_process(3, 0, 5, FINISHED),
    };

    TEST_ASSERT_EQUAL(1, scheduler_select_fcfs(ready_queue, 3, 0, NULL));
}

void test_prioridade_escolhe_maior_valor_de_prioridade(void) {
    Process ready_queue[] = {
        make_process(1, 0, 4, READY),
        make_process(2, 1, 9, READY),
        make_process(3, 2, 6, READY),
    };

    TEST_ASSERT_EQUAL(1, scheduler_select_priority(ready_queue, 3, 0, NULL));
}

void test_prioridade_desempata_por_chegada_e_pid(void) {
    Process ready_queue[] = {
        make_process(8, 3, 7, READY),
        make_process(5, 2, 7, READY),
        make_process(2, 2, 7, READY),
    };

    TEST_ASSERT_EQUAL(2, scheduler_select_priority(ready_queue, 3, 0, NULL));
}

void test_round_robin_escolhe_primeiro_processo_pronto_da_fila(void) {
    Process ready_queue[] = {
        make_process(1, 0, 5, BLOCKED),
        make_process(2, 1, 5, READY),
        make_process(3, 2, 5, READY),
    };

    TEST_ASSERT_EQUAL(1, scheduler_select_round_robin(ready_queue, 3, 0, NULL));
}

void test_algoritmos_retornam_no_process_quando_nao_ha_prontos(void) {
    Process ready_queue[] = {
        make_process(1, 0, 5, BLOCKED),
        make_process(2, 1, 8, FINISHED),
    };

    TEST_ASSERT_EQUAL(SCHEDULER_NO_PROCESS, scheduler_select_fcfs(ready_queue, 2, 0, NULL));
    TEST_ASSERT_EQUAL(SCHEDULER_NO_PROCESS, scheduler_select_priority(ready_queue, 2, 0, NULL));
    TEST_ASSERT_EQUAL(SCHEDULER_NO_PROCESS, scheduler_select_round_robin(ready_queue, 2, 0, NULL));
}

void test_round_robin_preempta_quando_quantum_expira(void) {
    SchedulerConfig config = { .quantum = 4, .context_switch_cost = 1 };

    TEST_ASSERT_FALSE(scheduler_round_robin_should_preempt(3, &config));
    TEST_ASSERT_TRUE(scheduler_round_robin_should_preempt(4, &config));
    TEST_ASSERT_TRUE(scheduler_round_robin_should_preempt(5, &config));
}

void test_round_robin_nao_preempta_com_quantum_invalido(void) {
    SchedulerConfig config = { .quantum = 0, .context_switch_cost = 1 };

    TEST_ASSERT_FALSE(scheduler_round_robin_should_preempt(10, &config));
    TEST_ASSERT_FALSE(scheduler_round_robin_should_preempt(10, NULL));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_fcfs_escolhe_menor_tempo_de_chegada);
    RUN_TEST(test_fcfs_desempata_por_menor_pid);
    RUN_TEST(test_fcfs_ignora_processos_que_nao_estao_prontos);
    RUN_TEST(test_prioridade_escolhe_maior_valor_de_prioridade);
    RUN_TEST(test_prioridade_desempata_por_chegada_e_pid);
    RUN_TEST(test_round_robin_escolhe_primeiro_processo_pronto_da_fila);
    RUN_TEST(test_algoritmos_retornam_no_process_quando_nao_ha_prontos);
    RUN_TEST(test_round_robin_preempta_quando_quantum_expira);
    RUN_TEST(test_round_robin_nao_preempta_com_quantum_invalido);

    return UNITY_END();
}
