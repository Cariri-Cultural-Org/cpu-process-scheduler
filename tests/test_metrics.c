#include <math.h>
#include <stddef.h>

#include "unity.h"
#include "../src/models/metrics/metrics.h"

void setUp(void) {}
void tearDown(void) {}

static void assert_double_close(double expected, double actual) {
    TEST_ASSERT_TRUE(fabs(expected - actual) <= 1e-9);
}

void test_turnaround_is_completion_minus_arrival(void) {
    assert_double_close(15.0, metrics_turnaround(5.0, 20.0));
}

void test_turnaround_rejects_invalid_times(void) {
    TEST_ASSERT_TRUE(isnan(metrics_turnaround(5.0, 4.0)));
    TEST_ASSERT_TRUE(isnan(metrics_turnaround(-1.0, 4.0)));
    TEST_ASSERT_TRUE(isnan(metrics_turnaround(NAN, 4.0)));
}

void test_ideal_time_sums_cpu_and_io(void) {
    assert_double_close(17.0, metrics_ideal_time(12.0, 5.0));
    assert_double_close(12.0, metrics_ideal_time(12.0, 0.0));
}

void test_ideal_time_rejects_invalid_durations(void) {
    TEST_ASSERT_TRUE(isnan(metrics_ideal_time(0.0, 5.0)));
    TEST_ASSERT_TRUE(isnan(metrics_ideal_time(5.0, -1.0)));
    TEST_ASSERT_TRUE(isnan(metrics_ideal_time(INFINITY, 1.0)));
}

void test_slowdown_divides_turnaround_by_ideal_time(void) {
    assert_double_close(2.5, metrics_slowdown(25.0, 10.0));
}

void test_slowdown_rejects_invalid_values(void) {
    TEST_ASSERT_TRUE(isnan(metrics_slowdown(0.0, 5.0)));
    TEST_ASSERT_TRUE(isnan(metrics_slowdown(5.0, 0.0)));
    TEST_ASSERT_TRUE(isnan(metrics_slowdown(NAN, 5.0)));
}

void test_mean_turnaround_uses_all_processes(void) {
    const ProcessMetricsSample samples[] = {
        {0.0, 10.0, 5.0, 0.0},
        {3.0, 23.0, 8.0, 2.0},
        {8.0, 38.0, 15.0, 5.0},
    };

    assert_double_close(20.0, metrics_mean_turnaround(samples, 3));
}

void test_jain_is_one_hundred_for_equal_slowdowns(void) {
    const ProcessMetricsSample samples[] = {
        {0.0, 10.0, 5.0, 0.0},
        {2.0, 22.0, 8.0, 2.0},
        {5.0, 35.0, 10.0, 5.0},
    };

    assert_double_close(100.0, metrics_jain_slowdown_percent(samples, 3));
}

void test_jain_matches_unequal_slowdowns(void) {
    const ProcessMetricsSample samples[] = {
        {0.0, 1.0, 1.0, 0.0},
        {0.0, 2.0, 1.0, 0.0},
        {0.0, 3.0, 1.0, 0.0},
    };
    const double expected = (36.0 / 42.0) * 100.0;

    assert_double_close(expected, metrics_jain_slowdown_percent(samples, 3));
}

void test_aggregated_metrics_include_context_switches(void) {
    const ProcessMetricsSample samples[] = {
        {0.0, 10.0, 5.0, 0.0},
        {0.0, 20.0, 10.0, 0.0},
    };
    SimulationMetrics result;

    TEST_ASSERT_TRUE(metrics_calculate(samples, 2, 7, &result));
    assert_double_close(15.0, result.mean_turnaround);
    TEST_ASSERT_EQUAL_UINT64(7, result.context_switches);
    assert_double_close(100.0, result.jain_slowdown_percent);
}

void test_aggregate_functions_reject_invalid_input(void) {
    const ProcessMetricsSample invalid[] = {
        {5.0, 4.0, 1.0, 0.0},
    };
    SimulationMetrics result = {1.0, 2, 3.0};

    TEST_ASSERT_TRUE(isnan(metrics_mean_turnaround(NULL, 1)));
    TEST_ASSERT_TRUE(isnan(metrics_mean_turnaround(invalid, 0)));
    TEST_ASSERT_TRUE(isnan(metrics_jain_slowdown_percent(invalid, 1)));
    TEST_ASSERT_FALSE(metrics_calculate(invalid, 1, 8, &result));
    TEST_ASSERT_FALSE(metrics_calculate(invalid, 1, 8, NULL));
    assert_double_close(1.0, result.mean_turnaround);
    TEST_ASSERT_EQUAL_UINT64(2, result.context_switches);
    assert_double_close(3.0, result.jain_slowdown_percent);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_turnaround_is_completion_minus_arrival);
    RUN_TEST(test_turnaround_rejects_invalid_times);
    RUN_TEST(test_ideal_time_sums_cpu_and_io);
    RUN_TEST(test_ideal_time_rejects_invalid_durations);
    RUN_TEST(test_slowdown_divides_turnaround_by_ideal_time);
    RUN_TEST(test_slowdown_rejects_invalid_values);
    RUN_TEST(test_mean_turnaround_uses_all_processes);
    RUN_TEST(test_jain_is_one_hundred_for_equal_slowdowns);
    RUN_TEST(test_jain_matches_unequal_slowdowns);
    RUN_TEST(test_aggregated_metrics_include_context_switches);
    RUN_TEST(test_aggregate_functions_reject_invalid_input);

    return UNITY_END();
}
