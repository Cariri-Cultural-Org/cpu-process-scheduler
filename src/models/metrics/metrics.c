#include "metrics.h"

#include <math.h>

double metrics_turnaround(double arrival_time, double completion_time) {
    if (!isfinite(arrival_time) || !isfinite(completion_time) ||
        arrival_time < 0.0 || completion_time <= arrival_time) {
        return NAN;
    }

    return completion_time - arrival_time;
}

double metrics_ideal_time(double total_cpu_time, double total_io_time) {
    if (!isfinite(total_cpu_time) || !isfinite(total_io_time) ||
        total_cpu_time <= 0.0 || total_io_time < 0.0) {
        return NAN;
    }

    const double ideal_time = total_cpu_time + total_io_time;
    return isfinite(ideal_time) ? ideal_time : NAN;
}

double metrics_slowdown(double turnaround, double ideal_time) {
    if (!isfinite(turnaround) || !isfinite(ideal_time) ||
        turnaround <= 0.0 || ideal_time <= 0.0) {
        return NAN;
    }

    return turnaround / ideal_time;
}

static double sample_turnaround(const ProcessMetricsSample *sample) {
    return metrics_turnaround(sample->arrival_time, sample->completion_time);
}

static double sample_slowdown(const ProcessMetricsSample *sample) {
    const double turnaround = sample_turnaround(sample);
    const double ideal_time = metrics_ideal_time(
        sample->total_cpu_time,
        sample->total_io_time
    );

    return metrics_slowdown(turnaround, ideal_time);
}

double metrics_mean_turnaround(
    const ProcessMetricsSample *samples,
    size_t count
) {
    if (samples == NULL || count == 0) {
        return NAN;
    }

    double mean = 0.0;
    for (size_t index = 0; index < count; index++) {
        const double turnaround = sample_turnaround(&samples[index]);
        if (!isfinite(turnaround)) {
            return NAN;
        }

        mean += (turnaround - mean) / (double)(index + 1);
    }

    return mean;
}

double metrics_jain_slowdown_percent(
    const ProcessMetricsSample *samples,
    size_t count
) {
    if (samples == NULL || count == 0) {
        return NAN;
    }

    double maximum = 0.0;
    for (size_t index = 0; index < count; index++) {
        const double slowdown = sample_slowdown(&samples[index]);
        if (!isfinite(slowdown)) {
            return NAN;
        }
        if (slowdown > maximum) {
            maximum = slowdown;
        }
    }

    double normalized_sum = 0.0;
    double normalized_square_sum = 0.0;
    for (size_t index = 0; index < count; index++) {
        const double normalized = sample_slowdown(&samples[index]) / maximum;
        normalized_sum += normalized;
        normalized_square_sum += normalized * normalized;
    }

    const double denominator = (double)count * normalized_square_sum;
    if (!isfinite(denominator) || denominator <= 0.0) {
        return NAN;
    }

    return (normalized_sum * normalized_sum / denominator) * 100.0;
}

bool metrics_calculate(
    const ProcessMetricsSample *samples,
    size_t count,
    size_t context_switches,
    SimulationMetrics *result
) {
    if (result == NULL) {
        return false;
    }

    const double mean_turnaround = metrics_mean_turnaround(samples, count);
    const double jain = metrics_jain_slowdown_percent(samples, count);
    if (!isfinite(mean_turnaround) || !isfinite(jain)) {
        return false;
    }

    const SimulationMetrics calculated = {
        .mean_turnaround = mean_turnaround,
        .context_switches = context_switches,
        .jain_slowdown_percent = jain,
    };
    *result = calculated;

    return true;
}
