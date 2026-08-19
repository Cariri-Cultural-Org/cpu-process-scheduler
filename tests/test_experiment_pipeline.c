#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "unity.h"

static char temporary_root[PATH_MAX];
static char raw_dir[PATH_MAX];
static char consolidated_dir[PATH_MAX];
static char graphs_dir[PATH_MAX];
static char summary_file[PATH_MAX];

static void join_path(char *destination, size_t capacity, const char *parent, const char *name) {
    const size_t parent_length = strlen(parent);
    const size_t name_length = strlen(name);
    TEST_ASSERT_TRUE_MESSAGE(parent_length + 1 + name_length + 1 <= capacity, "caminho temporário excede PATH_MAX");
    memcpy(destination, parent, parent_length);
    destination[parent_length] = '/';
    memcpy(destination + parent_length + 1, name, name_length + 1);
}

static int remove_tree(const char *path) {
    struct stat info;
    if (lstat(path, &info) != 0) {
        return errno == ENOENT ? 0 : -1;
    }
    if (!S_ISDIR(info.st_mode)) {
        return unlink(path);
    }

    DIR *directory = opendir(path);
    if (directory == NULL) {
        return -1;
    }
    struct dirent *entry;
    int result = 0;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char child[PATH_MAX];
        int written = snprintf(child, sizeof child, "%s/%s", path, entry->d_name);
        if (written < 0 || (size_t)written >= sizeof child || remove_tree(child) != 0) {
            result = -1;
            break;
        }
    }
    closedir(directory);
    if (result != 0) {
        return result;
    }
    return rmdir(path);
}

static void make_directory(const char *path) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, mkdir(path, 0700), path);
}

void setUp(void) {
    char template[] = "/tmp/cpu-scheduler-pipeline-XXXXXX";
    char *created = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(created);
    TEST_ASSERT_TRUE(strlen(created) < sizeof temporary_root);
    strcpy(temporary_root, created);

    join_path(raw_dir, sizeof raw_dir, temporary_root, "raw");
    join_path(consolidated_dir, sizeof consolidated_dir, temporary_root, "consolidated");
    join_path(graphs_dir, sizeof graphs_dir, temporary_root, "graphs");
    join_path(summary_file, sizeof summary_file, consolidated_dir, "summary.csv");
    make_directory(raw_dir);
    make_directory(consolidated_dir);
    make_directory(graphs_dir);
}

void tearDown(void) {
    if (temporary_root[0] != '\0') {
        TEST_ASSERT_EQUAL_INT(0, remove_tree(temporary_root));
        temporary_root[0] = '\0';
    }
}

static int exit_code(const char *command) {
    int status = system(command);
    if (status == -1 || !WIFEXITED(status)) {
        return -1;
    }
    return WEXITSTATUS(status);
}

static int run_small_experiment(void) {
    char command[PATH_MAX * 2];
    snprintf(
        command,
        sizeof command,
        "./scripts/run_experiments.sh --simulator ./bin/simulador "
        "--output-dir %s --seed-start 1 --seeds 2 --processes 8 "
        "--quantum 4 --cs-cost 1 --scenarios balanced,io_bound "
        "--algorithms fcfs,predictive_sjf >/dev/null 2>&1",
        raw_dir
    );
    return exit_code(command);
}

static int run_consolidation(void) {
    char command[PATH_MAX * 3];
    snprintf(
        command,
        sizeof command,
        "python3 scripts/consolidate_results.py --input-dir %s "
        "--output-file %s --seed-start 1 --expected-seeds 2 "
        "--scenarios balanced,io_bound --algorithms fcfs,predictive_sjf "
        ">/dev/null 2>&1",
        raw_dir,
        summary_file
    );
    return exit_code(command);
}

static int run_single_group_consolidation(void) {
    char command[PATH_MAX * 3];
    snprintf(
        command,
        sizeof command,
        "python3 scripts/consolidate_results.py --input-dir %s "
        "--output-file %s --seed-start 1 --expected-seeds 2 "
        "--scenarios balanced --algorithms fcfs >/dev/null 2>&1",
        raw_dir,
        summary_file
    );
    return exit_code(command);
}

static int run_graph_generation(const char *input_file) {
    char command[PATH_MAX * 3];
    snprintf(
        command,
        sizeof command,
        "python3 scripts/generate_graphs.py --input-file %s --output-dir %s "
        ">/dev/null 2>&1",
        input_file,
        graphs_dir
    );
    return exit_code(command);
}

static int has_suffix(const char *text, const char *suffix) {
    size_t text_length = strlen(text);
    size_t suffix_length = strlen(suffix);
    return text_length >= suffix_length &&
           strcmp(text + text_length - suffix_length, suffix) == 0;
}

static int count_files_with_suffix(const char *directory_path, const char *suffix) {
    DIR *directory = opendir(directory_path);
    TEST_ASSERT_NOT_NULL(directory);
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (has_suffix(entry->d_name, suffix)) {
            count++;
        }
    }
    closedir(directory);
    return count;
}

static long file_size(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return -1;
    }
    return (long)info.st_size;
}

static int count_lines(const char *path) {
    FILE *stream = fopen(path, "r");
    TEST_ASSERT_NOT_NULL(stream);
    int lines = 0;
    int character;
    while ((character = fgetc(stream)) != EOF) {
        if (character == '\n') {
            lines++;
        }
    }
    fclose(stream);
    return lines;
}

static int file_contains(const char *path, const char *needle) {
    FILE *stream = fopen(path, "r");
    if (stream == NULL) {
        return 0;
    }
    char buffer[4096];
    int found = 0;
    while (fgets(buffer, sizeof buffer, stream) != NULL) {
        if (strstr(buffer, needle) != NULL) {
            found = 1;
            break;
        }
    }
    fclose(stream);
    return found;
}

static int copy_file(const char *source, const char *destination) {
    FILE *input = fopen(source, "rb");
    if (input == NULL) {
        return -1;
    }
    FILE *output = fopen(destination, "wb");
    if (output == NULL) {
        fclose(input);
        return -1;
    }
    char buffer[4096];
    size_t read_count;
    int result = 0;
    while ((read_count = fread(buffer, 1, sizeof buffer, input)) > 0) {
        if (fwrite(buffer, 1, read_count, output) != read_count) {
            result = -1;
            break;
        }
    }
    if (ferror(input)) {
        result = -1;
    }
    fclose(input);
    if (fclose(output) != 0) {
        result = -1;
    }
    return result;
}

static void write_known_raw_result(
    const char *filename,
    int seed,
    double turnaround,
    int context_switches,
    double jain
) {
    char path[PATH_MAX];
    join_path(path, sizeof path, raw_dir, filename);
    FILE *stream = fopen(path, "w");
    TEST_ASSERT_NOT_NULL(stream);
    fputs("scenario,seed,algorithm,n_processes,quantum,context_switch_cost,"
          "mean_turnaround,context_switches,jain_slowdown_percent,total_time,"
          "cpu_busy_time,context_switch_time,idle_time\n", stream);
    fprintf(
        stream,
        "balanced,%d,fcfs,8,4,1,%.6f,%d,%.6f,%d,10,%d,0\n",
        seed,
        turnaround,
        context_switches,
        jain,
        10 + context_switches,
        context_switches
    );
    TEST_ASSERT_EQUAL_INT(0, fclose(stream));
}

static int read_consolidated_metric(
    const char *wanted_metric,
    int *sample_count,
    double *mean,
    double *stddev,
    double *lower,
    double *upper,
    double *margin
) {
    FILE *stream = fopen(summary_file, "r");
    if (stream == NULL) {
        return 0;
    }
    char line[4096];
    if (fgets(line, sizeof line, stream) == NULL) { /* cabeçalho */
        fclose(stream);
        return 0;
    }
    int found = 0;
    while (fgets(line, sizeof line, stream) != NULL) {
        char scenario[64], algorithm[64], metric[64], unit[64];
        int parsed = sscanf(
            line,
            "%63[^,],%63[^,],%63[^,],%63[^,],%d,%lf,%lf,%lf,%lf,%lf",
            scenario,
            algorithm,
            metric,
            unit,
            sample_count,
            mean,
            stddev,
            lower,
            upper,
            margin
        );
        if (parsed == 10 && strcmp(metric, wanted_metric) == 0) {
            found = 1;
            break;
        }
    }
    fclose(stream);
    return found;
}

static void assert_close(double expected, double actual) {
    TEST_ASSERT_TRUE(fabs(expected - actual) <= 1e-8);
}

void test_execucao_em_lote_gera_toda_a_matriz_reduzida(void) {
    TEST_ASSERT_EQUAL_INT(0, run_small_experiment());
    TEST_ASSERT_EQUAL_INT(8, count_files_with_suffix(raw_dir, ".csv"));

    char sample[PATH_MAX];
    join_path(sample, sizeof sample, raw_dir, "balanced_seed-1_fcfs.csv");
    TEST_ASSERT_TRUE(file_size(sample) > 100);
    TEST_ASSERT_EQUAL_INT(2, count_lines(sample));
    TEST_ASSERT_TRUE(file_contains(sample, "scenario,seed,algorithm"));
    TEST_ASSERT_TRUE(file_contains(sample, "balanced,1,fcfs,8,4,1"));
}

void test_execucao_em_lote_rejeita_parametros_invalidos(void) {
    char command[PATH_MAX * 2];
    snprintf(
        command,
        sizeof command,
        "./scripts/run_experiments.sh --simulator ./bin/simulador --output-dir %s "
        "--seeds 0 >/dev/null 2>&1",
        raw_dir
    );
    TEST_ASSERT_NOT_EQUAL(0, exit_code(command));
    TEST_ASSERT_EQUAL_INT(0, count_files_with_suffix(raw_dir, ".csv"));
}

void test_consolidacao_calcula_tres_metricas_por_grupo(void) {
    TEST_ASSERT_EQUAL_INT(0, run_small_experiment());
    TEST_ASSERT_EQUAL_INT(0, run_consolidation());

    /* 2 cenários x 2 algoritmos x 3 métricas + cabeçalho. */
    TEST_ASSERT_EQUAL_INT(13, count_lines(summary_file));
    TEST_ASSERT_TRUE(file_contains(summary_file, "sample_stddev,ci95_lower,ci95_upper,ci95_margin"));
    TEST_ASSERT_TRUE(file_contains(summary_file, "mean_turnaround,time_units,2,"));
    TEST_ASSERT_TRUE(file_contains(summary_file, "context_switches,count,2,"));
    TEST_ASSERT_TRUE(file_contains(summary_file, "jain_slowdown_percent,percent,2,"));
}

void test_consolidacao_calcula_media_desvio_e_ic95_exatos(void) {
    write_known_raw_result("seed-1.csv", 1, 10.0, 2, 80.0);
    write_known_raw_result("seed-2.csv", 2, 14.0, 4, 100.0);
    TEST_ASSERT_EQUAL_INT(0, run_single_group_consolidation());

    int n = 0;
    double mean = 0.0, stddev = 0.0, lower = 0.0, upper = 0.0, margin = 0.0;
    TEST_ASSERT_TRUE(read_consolidated_metric(
        "mean_turnaround", &n, &mean, &stddev, &lower, &upper, &margin
    ));
    TEST_ASSERT_EQUAL_INT(2, n);
    assert_close(12.0, mean);
    assert_close(sqrt(8.0), stddev);
    assert_close(8.08, lower);
    assert_close(15.92, upper);
    assert_close(3.92, margin);

    TEST_ASSERT_TRUE(read_consolidated_metric(
        "context_switches", &n, &mean, &stddev, &lower, &upper, &margin
    ));
    assert_close(3.0, mean);
    assert_close(sqrt(2.0), stddev);
    assert_close(1.04, lower);
    assert_close(4.96, upper);
    assert_close(1.96, margin);
}

void test_consolidacao_rejeita_combinacao_ausente(void) {
    TEST_ASSERT_EQUAL_INT(0, run_small_experiment());
    char missing[PATH_MAX];
    join_path(missing, sizeof missing, raw_dir, "io_bound_seed-2_predictive_sjf.csv");
    TEST_ASSERT_EQUAL_INT(0, unlink(missing));
    TEST_ASSERT_NOT_EQUAL(0, run_consolidation());
}

void test_consolidacao_rejeita_combinacao_duplicada(void) {
    TEST_ASSERT_EQUAL_INT(0, run_small_experiment());
    char source[PATH_MAX];
    char duplicate[PATH_MAX];
    join_path(source, sizeof source, raw_dir, "balanced_seed-1_fcfs.csv");
    join_path(duplicate, sizeof duplicate, raw_dir, "duplicado.csv");
    TEST_ASSERT_EQUAL_INT(0, copy_file(source, duplicate));
    TEST_ASSERT_NOT_EQUAL(0, run_consolidation());
}

void test_consolidacao_rejeita_csv_malformado(void) {
    TEST_ASSERT_EQUAL_INT(0, run_small_experiment());
    char malformed[PATH_MAX];
    join_path(malformed, sizeof malformed, raw_dir, "balanced_seed-1_fcfs.csv");
    FILE *stream = fopen(malformed, "w");
    TEST_ASSERT_NOT_NULL(stream);
    fputs("scenario,seed,algorithm,n_processes,quantum,context_switch_cost,"
          "mean_turnaround,context_switches,jain_slowdown_percent,total_time,"
          "cpu_busy_time,context_switch_time,idle_time\n", stream);
    fputs("balanced,1,fcfs,8,4,1,nao-numero,2,90,10,8,2,0\n", stream);
    TEST_ASSERT_EQUAL_INT(0, fclose(stream));
    TEST_ASSERT_NOT_EQUAL(0, run_consolidation());
}

void test_geracao_de_graficos_cria_tres_svgs_com_ic95(void) {
    TEST_ASSERT_EQUAL_INT(0, run_small_experiment());
    TEST_ASSERT_EQUAL_INT(0, run_consolidation());
    TEST_ASSERT_EQUAL_INT(0, run_graph_generation(summary_file));
    TEST_ASSERT_EQUAL_INT(3, count_files_with_suffix(graphs_dir, ".svg"));

    const char *names[] = {
        "mean_turnaround.svg",
        "context_switches.svg",
        "jain_slowdown_percent.svg",
    };
    for (size_t index = 0; index < sizeof names / sizeof names[0]; index++) {
        char path[PATH_MAX];
        join_path(path, sizeof path, graphs_dir, names[index]);
        TEST_ASSERT_TRUE(file_size(path) > 1000);
        TEST_ASSERT_TRUE(file_contains(path, "<svg"));
        TEST_ASSERT_TRUE(file_contains(path, "intervalo de confiança de 95%"));
        TEST_ASSERT_TRUE(file_contains(path, "SJF Preditivo + Aging"));
    }
}

void test_geracao_de_graficos_rejeita_grupos_ausentes(void) {
    TEST_ASSERT_EQUAL_INT(0, run_small_experiment());
    TEST_ASSERT_EQUAL_INT(0, run_consolidation());

    char incomplete[PATH_MAX];
    join_path(incomplete, sizeof incomplete, consolidated_dir, "incomplete.csv");
    FILE *input = fopen(summary_file, "r");
    FILE *output = fopen(incomplete, "w");
    TEST_ASSERT_NOT_NULL(input);
    TEST_ASSERT_NOT_NULL(output);
    char line[4096];
    TEST_ASSERT_NOT_NULL(fgets(line, sizeof line, input));
    fputs(line, output);
    TEST_ASSERT_NOT_NULL(fgets(line, sizeof line, input));
    fputs(line, output);
    fclose(input);
    TEST_ASSERT_EQUAL_INT(0, fclose(output));

    TEST_ASSERT_NOT_EQUAL(0, run_graph_generation(incomplete));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_execucao_em_lote_gera_toda_a_matriz_reduzida);
    RUN_TEST(test_execucao_em_lote_rejeita_parametros_invalidos);
    RUN_TEST(test_consolidacao_calcula_tres_metricas_por_grupo);
    RUN_TEST(test_consolidacao_calcula_media_desvio_e_ic95_exatos);
    RUN_TEST(test_consolidacao_rejeita_combinacao_ausente);
    RUN_TEST(test_consolidacao_rejeita_combinacao_duplicada);
    RUN_TEST(test_consolidacao_rejeita_csv_malformado);
    RUN_TEST(test_geracao_de_graficos_cria_tres_svgs_com_ic95);
    RUN_TEST(test_geracao_de_graficos_rejeita_grupos_ausentes);
    return UNITY_END();
}
