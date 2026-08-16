#include "unity.h"
#include "../src/models/scheduler.h"

void setUp(void) {
    scheduler_predictive_sjf_reset();
}

void tearDown(void) {}

static Process make_process(
    int pid,
    int arrival_time,
    Burst *bursts,
    int num_bursts,
    int current_burst_index
) {
    Process p;
    p.pid = pid;
    p.arrival_time = arrival_time;
    p.priority = 1;
    p.state = READY;
    p.bursts = bursts;
    p.num_bursts = num_bursts;
    p.current_burst_index = current_burst_index;
    return p;
}

void test_fila_vazia_retorna_no_process(void) {
    int selected = scheduler_select_predictive_sjf(NULL, 0, 0, NULL);
    TEST_ASSERT_EQUAL(SCHEDULER_NO_PROCESS, selected);
}

void test_nenhum_processo_pronto_retorna_no_process(void) {
    Burst bursts[] = { { CPU, 5 } };
    Process p = make_process(1, 0, bursts, 1, 0);
    p.state = BLOCKED;

    int selected = scheduler_select_predictive_sjf(&p, 1, 0, NULL);
    TEST_ASSERT_EQUAL(SCHEDULER_NO_PROCESS, selected);
}

void test_unico_processo_pronto_e_selecionado(void) {
    Burst bursts[] = { { CPU, 5 } };
    Process p = make_process(7, 0, bursts, 1, 0);

    int selected = scheduler_select_predictive_sjf(&p, 1, 0, NULL);
    TEST_ASSERT_EQUAL(0, selected);
}

/* Sem histórico algum, todos caem no mesmo fallback de tau (nenhuma
 * rajada de CPU concluída na simulação): desempate por chegada. */
void test_sem_historico_desempate_por_chegada(void) {
    Burst bursts_a[] = { { CPU, 3 } };
    Burst bursts_b[] = { { CPU, 3 } };
    Process queue[2] = {
        make_process(5, 3, bursts_a, 1, 0),
        make_process(2, 1, bursts_b, 1, 0),
    };

    int selected = scheduler_select_predictive_sjf(queue, 2, 3, NULL);
    TEST_ASSERT_EQUAL(1, selected); /* pid 2, chegou em 1 */
}

/* Empate também na chegada: desempate por pid. */
void test_sem_historico_desempate_por_pid(void) {
    Burst bursts_a[] = { { CPU, 3 } };
    Burst bursts_b[] = { { CPU, 3 } };
    Process queue[2] = {
        make_process(5, 0, bursts_a, 1, 0),
        make_process(2, 0, bursts_b, 1, 0),
    };

    int selected = scheduler_select_predictive_sjf(queue, 2, 0, NULL);
    TEST_ASSERT_EQUAL(1, selected); /* pid 2 < pid 5 */
}

/* Processo com histórico de rajadas curtas deve ser preferido a um com
 * histórico de rajadas longas, com wait igual (0) para ambos. */
void test_historico_curto_e_preferido_a_historico_longo(void) {
    /* Cada processo já concluiu 1 rajada de CPU + 1 de E/S, aguardando
     * a próxima rajada de CPU (ainda não executada, não deve influenciar). */
    Burst bursts_curto[] = { { CPU, 2 }, { IO, 5 }, { CPU, 999 } };
    Burst bursts_longo[] = { { CPU, 40 }, { IO, 5 }, { CPU, 999 } };
    Process queue[2] = {
        make_process(1, 0, bursts_curto, 3, 2),
        make_process(2, 0, bursts_longo, 3, 2),
    };

    int selected = scheduler_select_predictive_sjf(queue, 2, 0, NULL);
    TEST_ASSERT_EQUAL(0, selected); /* histórico curto (pid 1) vence */
}

/* Aging: um processo sem histórico mas esperando há muito tempo deve
 * vencer um processo igualmente sem histórico que acabou de chegar à
 * fila de prontos (mesmo tau de fallback para os dois). */
void test_aging_favorece_processo_que_espera_ha_mais_tempo(void) {
    Burst bursts_w[] = { { CPU, 1 } };
    Process w = make_process(1, 0, bursts_w, 1, 0);

    /* t=0: só W está pronto; registra o instante em que ele entrou na fila. */
    int first_selected = scheduler_select_predictive_sjf(&w, 1, 0, NULL);
    TEST_ASSERT_EQUAL(0, first_selected);

    /* t=20: Q chega agora (wait=0); W segue na fila, esperando há 20. */
    Burst bursts_q[] = { { CPU, 1 } };
    Process queue[2] = {
        w,
        make_process(2, 20, bursts_q, 1, 0),
    };

    int selected = scheduler_select_predictive_sjf(queue, 2, 20, NULL);
    TEST_ASSERT_EQUAL(0, selected); /* W (pid 1) vence por aging */
}

/* A rajada atual (ainda não executada) não pode influenciar a decisão:
 * dois processos sem histórico, diferindo só na duração da rajada
 * pendente, continuam empatados em tau (decide por desempate). */
void test_rajada_atual_nao_influencia_a_estimativa(void) {
    Burst bursts_a[] = { { CPU, 1 } };
    Burst bursts_b[] = { { CPU, 1000 } };
    Process queue[2] = {
        make_process(1, 0, bursts_a, 1, 0),
        make_process(2, 0, bursts_b, 1, 0),
    };

    int selected = scheduler_select_predictive_sjf(queue, 2, 0, NULL);
    TEST_ASSERT_EQUAL(0, selected); /* pid 1 vence por desempate, não pela duração */

    scheduler_predictive_sjf_reset();

    /* Invertendo qual processo tem a rajada pendente enorme: o vencedor
     * continua sendo o de menor pid, provando que a duração pendente
     * não pesou na conta. */
    Process queue2[2] = {
        make_process(1, 0, bursts_b, 1, 0),
        make_process(2, 0, bursts_a, 1, 0),
    };

    int selected2 = scheduler_select_predictive_sjf(queue2, 2, 0, NULL);
    TEST_ASSERT_EQUAL(0, selected2);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_fila_vazia_retorna_no_process);
    RUN_TEST(test_nenhum_processo_pronto_retorna_no_process);
    RUN_TEST(test_unico_processo_pronto_e_selecionado);
    RUN_TEST(test_sem_historico_desempate_por_chegada);
    RUN_TEST(test_sem_historico_desempate_por_pid);
    RUN_TEST(test_historico_curto_e_preferido_a_historico_longo);
    RUN_TEST(test_aging_favorece_processo_que_espera_ha_mais_tempo);
    RUN_TEST(test_rajada_atual_nao_influencia_a_estimativa);

    return UNITY_END();
}
