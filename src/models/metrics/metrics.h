#ifndef METRICS_H
#define METRICS_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    double arrival_time;
    double completion_time;
    double total_cpu_time;
    double total_io_time;
} ProcessMetricsSample;

typedef struct {
    double mean_turnaround;
    size_t context_switches;
    double jain_slowdown_percent;
} SimulationMetrics;

double metrics_turnaround(double arrival_time, double completion_time);
double metrics_ideal_time(double total_cpu_time, double total_io_time);
double metrics_slowdown(double turnaround, double ideal_time);

double metrics_mean_turnaround(
    const ProcessMetricsSample *samples,
    size_t count
);

double metrics_jain_slowdown_percent(
    const ProcessMetricsSample *samples,
    size_t count
);

bool metrics_calculate(
    const ProcessMetricsSample *samples,
    size_t count,
    size_t context_switches,
    SimulationMetrics *result
);

#endif
