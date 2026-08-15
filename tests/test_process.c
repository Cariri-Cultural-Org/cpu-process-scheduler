#include <stdlib.h>

#include "unity.h"
#include "../src/models/process/process.h"

void setUp(void) {}
void tearDown(void) {}

/* ---------------------------------------------------------------------
 * Identificador, tempo de chegada e prioridade
 * (docs/PROJETO.md, seção 3; docs/MODELAGEM.md, seção 1)
 * ------------------------------------------------------------------- */
void test_processo_armazena_pid_chegada_e_prioridade(void) {
    Process p = process_create(1, 0, 5, 1);

    TEST_ASSERT_EQUAL(1, p.pid);
    TEST_ASSERT_EQUAL(0, p.arrival_time);
    TEST_ASSERT_EQUAL(5, p.priority);

    free(p.bursts);
}

void test_processo_novo_comeca_no_estado_novo(void) {
    Process p = process_create(1, 0, 5, 1);

    TEST_ASSERT_EQUAL(NEW, p.state);
    TEST_ASSERT_EQUAL(0, p.current_burst_index);

    free(p.bursts);
}

void test_prioridade_maior_valor_significa_maior_prioridade(void) {
    /* Convencao documentada em docs/MODELAGEM.md, secao 1: valores
     * MAIORES representam maior prioridade. */
    Process alta = { .pid = 1, .arrival_time = 0, .priority = 9, .state = NEW,
                      .bursts = NULL, .num_bursts = 0, .current_burst_index = 0 };
    Process baixa = { .pid = 2, .arrival_time = 0, .priority = 1, .state = NEW,
                       .bursts = NULL, .num_bursts = 0, .current_burst_index = 0 };

    TEST_ASSERT_TRUE(alta.priority > baixa.priority);
}

/* ---------------------------------------------------------------------
 * Rajadas de CPU e de E/S (BurstType, Burst, sequência CPU -> E/S -> CPU)
 * (docs/PROJETO.md, seção 3)
 * ------------------------------------------------------------------- */
void test_processo_armazena_sequencia_de_rajadas_cpu_e_es(void) {
    /* process_create alterna CPU/E-S comecando por CPU (indice par). */
    Process p = process_create(1, 0, 5, 3);

    TEST_ASSERT_EQUAL(3, p.num_bursts);
    TEST_ASSERT_EQUAL(CPU, p.bursts[0].type);
    TEST_ASSERT_EQUAL(IO, p.bursts[1].type);
    TEST_ASSERT_EQUAL(CPU, p.bursts[2].type);

    free(p.bursts);
}

void test_numero_e_duracao_das_requisicoes_de_es(void) {
    Burst bursts[] = {
        { .type = CPU, .duration = 10 },
        { .type = IO, .duration = 3 },
        { .type = CPU, .duration = 5 },
        { .type = IO, .duration = 4 },
        { .type = CPU, .duration = 2 },
    };

    Process p = {
        .pid = 1, .arrival_time = 0, .priority = 5, .state = NEW,
        .bursts = bursts, .num_bursts = 5, .current_burst_index = 0,
    };

    int io_count = 0;
    for (int i = 0; i < p.num_bursts; i++) {
        if (p.bursts[i].type == IO) io_count++;
    }
    TEST_ASSERT_EQUAL(2, io_count);
    TEST_ASSERT_EQUAL(3, p.bursts[1].duration);
    TEST_ASSERT_EQUAL(4, p.bursts[3].duration);
}

/* ---------------------------------------------------------------------
 * Transições de estado (novo -> pronto -> executando -> bloqueado ->
 * pronto -> ... -> finalizado), via set_state (process.h).
 * ------------------------------------------------------------------- */
void test_transicao_novo_para_pronto(void) {
    Process p = { .pid = 1, .arrival_time = 0, .priority = 5, .state = NEW,
                   .bursts = NULL, .num_bursts = 0, .current_burst_index = 0 };

    set_state(&p, READY);
    TEST_ASSERT_EQUAL(READY, p.state);
}

void test_transicao_pronto_para_executando(void) {
    Process p = { .pid = 1, .arrival_time = 0, .priority = 5, .state = READY,
                   .bursts = NULL, .num_bursts = 0, .current_burst_index = 0 };

    set_state(&p, EXECUTING);
    TEST_ASSERT_EQUAL(EXECUTING, p.state);
}

void test_transicao_executando_para_bloqueado_ao_atingir_rajada_de_es(void) {
    Burst bursts[] = {
        { .type = CPU, .duration = 10 },
        { .type = IO, .duration = 3 },
    };

    Process p = { .pid = 1, .arrival_time = 0, .priority = 5, .state = EXECUTING,
                   .bursts = bursts, .num_bursts = 2, .current_burst_index = 0 };

    /* fim da 1a rajada de CPU -> avanca para a rajada de E/S -> bloqueia */
    p.current_burst_index = 1;
    set_state(&p, BLOCKED);

    TEST_ASSERT_EQUAL(BLOCKED, p.state);
    TEST_ASSERT_EQUAL(IO, p.bursts[p.current_burst_index].type);
}

void test_transicao_bloqueado_para_pronto_ao_terminar_es(void) {
    Process p = { .pid = 1, .arrival_time = 0, .priority = 5, .state = BLOCKED,
                   .bursts = NULL, .num_bursts = 0, .current_burst_index = 0 };

    set_state(&p, READY);
    TEST_ASSERT_EQUAL(READY, p.state);
}

void test_transicao_executando_para_finalizado_na_ultima_rajada(void) {
    Burst bursts[] = {
        { .type = CPU, .duration = 10 },
    };

    Process p = { .pid = 1, .arrival_time = 0, .priority = 5, .state = EXECUTING,
                   .bursts = bursts, .num_bursts = 1, .current_burst_index = 0 };

    /* nao ha proxima rajada -> processo finaliza */
    p.current_burst_index = 1; /* == num_bursts: sem rajadas pendentes */
    set_state(&p, FINISHED);

    TEST_ASSERT_EQUAL(FINISHED, p.state);
    TEST_ASSERT_EQUAL(p.num_bursts, p.current_burst_index);
}

/* ---------------------------------------------------------------------
 * Funções auxiliares de process.h/.c
 * ------------------------------------------------------------------- */
void test_calculate_remaining_time_soma_duracao_das_rajadas_pendentes(void) {
    /* calculate_remaining_time soma a duracao de TODAS as rajadas
     * (CPU e E/S) a partir de current_burst_index (inclusive). */
    Burst bursts[] = {
        { .type = CPU, .duration = 10 },
        { .type = IO, .duration = 3 },
        { .type = CPU, .duration = 5 },
        { .type = IO, .duration = 4 },
        { .type = CPU, .duration = 2 },
    };

    Process p = { .pid = 1, .arrival_time = 0, .priority = 5, .state = READY,
                   .bursts = bursts, .num_bursts = 5, .current_burst_index = 0 };

    TEST_ASSERT_EQUAL(24, calculate_remaining_time(&p)); /* 10+3+5+4+2 */

    p.current_burst_index = 2;
    TEST_ASSERT_EQUAL(11, calculate_remaining_time(&p)); /* 5+4+2 */
}

void test_calculate_remaining_time_zero_quando_nao_ha_rajadas_pendentes(void) {
    Burst bursts[] = {
        { .type = CPU, .duration = 10 },
    };

    Process p = { .pid = 1, .arrival_time = 0, .priority = 5, .state = FINISHED,
                   .bursts = bursts, .num_bursts = 1, .current_burst_index = 1 };

    TEST_ASSERT_EQUAL(0, calculate_remaining_time(&p));
}

void test_calculate_current_burst_remaining_time_desconta_tempo_decorrido(void) {
    /* Formula atual (process.c): duracao da rajada atual menos o tempo
     * decorrido desde arrival_time. */
    Burst bursts[] = {
        { .type = CPU, .duration = 10 },
        { .type = IO, .duration = 3 },
    };

    Process p = { .pid = 1, .arrival_time = 0, .priority = 5, .state = EXECUTING,
                   .bursts = bursts, .num_bursts = 2, .current_burst_index = 0 };

    TEST_ASSERT_EQUAL(10, calculate_current_burst_remaining_time(&p, 0));
    TEST_ASSERT_EQUAL(6, calculate_current_burst_remaining_time(&p, 4));
}

void test_calculate_current_burst_remaining_time_nao_fica_negativo(void) {
    Burst bursts[] = {
        { .type = CPU, .duration = 5 },
    };

    Process p = { .pid = 1, .arrival_time = 0, .priority = 5, .state = EXECUTING,
                   .bursts = bursts, .num_bursts = 1, .current_burst_index = 0 };

    TEST_ASSERT_EQUAL(0, calculate_current_burst_remaining_time(&p, 20));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_processo_armazena_pid_chegada_e_prioridade);
    RUN_TEST(test_processo_novo_comeca_no_estado_novo);
    RUN_TEST(test_prioridade_maior_valor_significa_maior_prioridade);

    RUN_TEST(test_processo_armazena_sequencia_de_rajadas_cpu_e_es);
    RUN_TEST(test_numero_e_duracao_das_requisicoes_de_es);

    RUN_TEST(test_transicao_novo_para_pronto);
    RUN_TEST(test_transicao_pronto_para_executando);
    RUN_TEST(test_transicao_executando_para_bloqueado_ao_atingir_rajada_de_es);
    RUN_TEST(test_transicao_bloqueado_para_pronto_ao_terminar_es);
    RUN_TEST(test_transicao_executando_para_finalizado_na_ultima_rajada);

    RUN_TEST(test_calculate_remaining_time_soma_duracao_das_rajadas_pendentes);
    RUN_TEST(test_calculate_remaining_time_zero_quando_nao_ha_rajadas_pendentes);
    RUN_TEST(test_calculate_current_burst_remaining_time_desconta_tempo_decorrido);
    RUN_TEST(test_calculate_current_burst_remaining_time_nao_fica_negativo);

    return UNITY_END();
}
