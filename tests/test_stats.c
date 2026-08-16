/*
 * Runner C para integrar os testes de scripts/stats.py à suíte do projeto.
 *
 * As asserções das funções Python ficam em test_stats.py. Este executável é
 * descoberto pelo padrão tests/test_*.c do Makefile da branch principal e
 * propaga ao `make test` qualquer falha da suíte Python.
 */

#include <stdlib.h>

#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_python_stats_unit_suite_passes(void) {
    const int status = system("python3 -m unittest -v tests/test_stats.py");

    TEST_ASSERT_EQUAL_INT_MESSAGE(
        0,
        status,
        "Falha nos testes unitários de scripts/stats.py."
    );
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_python_stats_unit_suite_passes);

    return UNITY_END();
}
