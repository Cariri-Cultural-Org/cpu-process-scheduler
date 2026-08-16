#include "unity.h"
#include "../src/models/workload/workload_generator.h"

void setUp(void) {}
void tearDown(void) {}

void test_gera_a_quantidade_de_processos_pedida(void) {
    Workload w = generate_workload(SCENARIO_BALANCED, 42, 1000);

    TEST_ASSERT_EQUAL(1000, w.count);
    TEST_ASSERT_NOT_NULL(w.processes);

    free_workload(&w);
}

void test_mesma_seed_e_cenario_geram_a_mesma_carga(void) {
    Workload a = generate_workload(SCENARIO_BALANCED, 7, 200);
    Workload b = generate_workload(SCENARIO_BALANCED, 7, 200);

    for (size_t i = 0; i < a.count; i++) {
        TEST_ASSERT_EQUAL(a.processes[i].pid, b.processes[i].pid);
        TEST_ASSERT_EQUAL(a.processes[i].arrival_time, b.processes[i].arrival_time);
        TEST_ASSERT_EQUAL(a.processes[i].priority, b.processes[i].priority);
        TEST_ASSERT_EQUAL(a.processes[i].num_bursts, b.processes[i].num_bursts);
        for (int j = 0; j < a.processes[i].num_bursts; j++) {
            TEST_ASSERT_EQUAL(a.processes[i].bursts[j].type, b.processes[i].bursts[j].type);
            TEST_ASSERT_EQUAL(a.processes[i].bursts[j].duration, b.processes[i].bursts[j].duration);
        }
    }

    free_workload(&a);
    free_workload(&b);
}

void test_seeds_diferentes_geram_cargas_diferentes(void) {
    Workload a = generate_workload(SCENARIO_BALANCED, 1, 200);
    Workload b = generate_workload(SCENARIO_BALANCED, 2, 200);

    int diferem = 0;
    for (size_t i = 0; i < a.count; i++) {
        if (a.processes[i].arrival_time != b.processes[i].arrival_time
            || a.processes[i].priority != b.processes[i].priority) {
            diferem = 1;
            break;
        }
    }
    TEST_ASSERT_TRUE(diferem);

    free_workload(&a);
    free_workload(&b);
}

void test_processos_novos_comecam_no_estado_novo(void) {
    Workload w = generate_workload(SCENARIO_BALANCED, 3, 100);

    for (size_t i = 0; i < w.count; i++) {
        TEST_ASSERT_EQUAL(NEW, w.processes[i].state);
        TEST_ASSERT_EQUAL(0, w.processes[i].current_burst_index);
    }

    free_workload(&w);
}

void test_tempos_de_chegada_sao_nao_decrescentes(void) {
    /* seção 4 de MODELAGEM.md: chegada por processo de Poisson, logo
     * já ordenada por construção. */
    Workload w = generate_workload(SCENARIO_BALANCED, 99, 500);

    for (size_t i = 1; i < w.count; i++) {
        TEST_ASSERT_TRUE(w.processes[i].arrival_time >= w.processes[i - 1].arrival_time);
    }

    free_workload(&w);
}

void test_rajadas_alternam_cpu_e_es_terminando_em_cpu(void) {
    Workload w = generate_workload(SCENARIO_BALANCED, 5, 300);

    for (size_t i = 0; i < w.count; i++) {
        Process *p = &w.processes[i];
        TEST_ASSERT_EQUAL(CPU, p->bursts[0].type);
        TEST_ASSERT_EQUAL(CPU, p->bursts[p->num_bursts - 1].type);
        for (int j = 0; j < p->num_bursts; j++) {
            TEST_ASSERT_EQUAL(j % 2 == 0 ? CPU : IO, p->bursts[j].type);
        }
    }

    free_workload(&w);
}

void test_cenario_io_bound_gera_rajadas_de_cpu_mais_curtas_que_cpu_bound(void) {
    Workload io_bound = generate_workload(SCENARIO_IO_BOUND, 11, 500);
    Workload cpu_bound = generate_workload(SCENARIO_CPU_BOUND, 11, 500);

    double soma_io = 0, soma_cpu = 0;
    int n_io = 0, n_cpu = 0;
    for (size_t i = 0; i < io_bound.count; i++) {
        for (int j = 0; j < io_bound.processes[i].num_bursts; j++) {
            if (io_bound.processes[i].bursts[j].type == CPU) {
                soma_io += io_bound.processes[i].bursts[j].duration;
                n_io++;
            }
        }
    }
    for (size_t i = 0; i < cpu_bound.count; i++) {
        for (int j = 0; j < cpu_bound.processes[i].num_bursts; j++) {
            if (cpu_bound.processes[i].bursts[j].type == CPU) {
                soma_cpu += cpu_bound.processes[i].bursts[j].duration;
                n_cpu++;
            }
        }
    }

    TEST_ASSERT_TRUE((soma_io / n_io) < (soma_cpu / n_cpu));

    free_workload(&io_bound);
    free_workload(&cpu_bound);
}

void test_cenario_prioridades_desbalanceadas_favorece_prioridade_baixa(void) {
    /* seção 7 de MODELAGEM.md: 80% prioridade baixa [1,3], 20% alta [8,10]. */
    Workload w = generate_workload(SCENARIO_PRIORITY_SKEWED, 21, 1000);

    int baixa = 0;
    for (size_t i = 0; i < w.count; i++) {
        int prio = w.processes[i].priority;
        TEST_ASSERT_TRUE((prio >= 1 && prio <= 3) || (prio >= 8 && prio <= 10));
        if (prio <= 3) baixa++;
    }

    TEST_ASSERT_TRUE(baixa > (int)(w.count / 2));

    free_workload(&w);
}

void test_scenario_name_retorna_nome_legivel_para_cada_cenario(void) {
    TEST_ASSERT_EQUAL_STRING("balanced", scenario_name(SCENARIO_BALANCED));
    TEST_ASSERT_EQUAL_STRING("io_bound", scenario_name(SCENARIO_IO_BOUND));
    TEST_ASSERT_EQUAL_STRING("cpu_bound", scenario_name(SCENARIO_CPU_BOUND));
    TEST_ASSERT_EQUAL_STRING("priority_skewed", scenario_name(SCENARIO_PRIORITY_SKEWED));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_gera_a_quantidade_de_processos_pedida);
    RUN_TEST(test_mesma_seed_e_cenario_geram_a_mesma_carga);
    RUN_TEST(test_seeds_diferentes_geram_cargas_diferentes);
    RUN_TEST(test_processos_novos_comecam_no_estado_novo);
    RUN_TEST(test_tempos_de_chegada_sao_nao_decrescentes);
    RUN_TEST(test_rajadas_alternam_cpu_e_es_terminando_em_cpu);
    RUN_TEST(test_cenario_io_bound_gera_rajadas_de_cpu_mais_curtas_que_cpu_bound);
    RUN_TEST(test_cenario_prioridades_desbalanceadas_favorece_prioridade_baixa);
    RUN_TEST(test_scenario_name_retorna_nome_legivel_para_cada_cenario);

    return UNITY_END();
}
