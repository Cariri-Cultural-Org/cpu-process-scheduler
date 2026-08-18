/* =====================================================================
 * main.c
 * =====================================================================
 *
 * Frente responsável: Alan Mendes (núcleo da simulação), com
 * integração das partes desenvolvidas pelos demais integrantes.
 *
 * Objetivo:
 *   Ponto de entrada do simulador. Este arquivo apenas orquestra uma
 *   única execução do simulador a partir de parâmetros de linha de
 *   comando: interpreta os argumentos, gera a carga de trabalho
 *   (src/models/workload/), executa o núcleo da simulação
 *   (src/models/simulator/) com o algoritmo de escalonamento
 *   escolhido (src/models/scheduler.h) e calcula as métricas
 *   (src/models/metrics/), imprimindo o resultado como uma única
 *   linha CSV em stdout.
 *
 *   O laço sobre cenários, seeds e algoritmos para os experimentos
 *   completos NÃO vive aqui: fica em scripts/run_experiments.sh, que
 *   invoca este binário uma vez por combinação.
 *
 * Uso:
 *   bin/simulador --scenario <balanced|io_bound|cpu_bound|priority_skewed>
 *                  --seed <unsigned long>
 *                  --algorithm <fcfs|round_robin|priority|predictive_sjf>
 *                  [--processes N]
 *                  [--quantum Q]
 *                  [--cs-cost C]
 *                  [--header]
 *                  [--help]
 *
 * Saída (stdout, CSV):
 *   scenario,seed,algorithm,n_processes,quantum,context_switch_cost,
 *   mean_turnaround,context_switches,jain_slowdown_percent,total_time,
 *   cpu_busy_time,context_switch_time,idle_time
 * ===================================================================== */

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "models/config.h"
#include "models/metrics/metrics.h"
#include "models/process/process.h"
#include "models/scheduler.h"
#include "models/simulator/simulator.h"
#include "models/workload/workload_generator.h"

/* ---------------------------------------------------------------------
 * Parâmetros de execução coletados da linha de comando.
 * ------------------------------------------------------------------- */
typedef struct {
    bool has_scenario;
    bool has_seed;
    bool has_algorithm;
    Scenario scenario;
    unsigned long seed;
    SchedulerAlgorithm algorithm;
    const char *algorithm_name; /* nome como recebido no CLI, para o CSV */
    size_t n_processes;
    int quantum;
    int cs_cost;
    bool print_header;
} CliOptions;

static void print_usage(FILE *out) {
    fprintf(out,
        "Uso: bin/simulador --scenario <balanced|io_bound|cpu_bound|priority_skewed>\n"
        "                    --seed <unsigned long>\n"
        "                    --algorithm <fcfs|round_robin|priority|predictive_sjf>\n"
        "                    [--processes N]     (padrão: %d)\n"
        "                    [--quantum Q]       (padrão: %d)\n"
        "                    [--cs-cost C]       (padrão: %d)\n"
        "                    [--header]          (emite a linha de cabeçalho antes dos dados)\n"
        "                    [--help]\n",
        CONFIG_MIN_PROCESSES, CONFIG_RR_QUANTUM, CONFIG_CONTEXT_SWITCH_COST);
}

/* Interpreta um cenário a partir do nome recebido no CLI. Retorna true
 * em caso de sucesso. */
static bool parse_scenario(const char *text, Scenario *out) {
    if (strcmp(text, "balanced") == 0) {
        *out = SCENARIO_BALANCED;
        return true;
    }
    if (strcmp(text, "io_bound") == 0) {
        *out = SCENARIO_IO_BOUND;
        return true;
    }
    if (strcmp(text, "cpu_bound") == 0) {
        *out = SCENARIO_CPU_BOUND;
        return true;
    }
    if (strcmp(text, "priority_skewed") == 0) {
        *out = SCENARIO_PRIORITY_SKEWED;
        return true;
    }
    return false;
}

/* Interpreta um algoritmo a partir do nome recebido no CLI. Retorna
 * true em caso de sucesso. "predictive_sjf" mapeia para
 * SCHEDULER_CUSTOM. */
static bool parse_algorithm(const char *text, SchedulerAlgorithm *out) {
    if (strcmp(text, "fcfs") == 0) {
        *out = SCHEDULER_FCFS;
        return true;
    }
    if (strcmp(text, "round_robin") == 0) {
        *out = SCHEDULER_ROUND_ROBIN;
        return true;
    }
    if (strcmp(text, "priority") == 0) {
        *out = SCHEDULER_PRIORITY;
        return true;
    }
    if (strcmp(text, "predictive_sjf") == 0) {
        *out = SCHEDULER_CUSTOM;
        return true;
    }
    return false;
}

/* Interpreta um unsigned long estritamente (sem lixo, sem sinal
 * negativo, sem overflow). Retorna true em caso de sucesso. */
static bool parse_ulong(const char *text, unsigned long *out) {
    if (text == NULL || text[0] == '\0') {
        return false;
    }
    /* strtoul aceita sinal negativo silenciosamente (faz wraparound);
     * rejeitamos explicitamente para evitar seeds "negativas". */
    size_t i = 0;
    while (text[i] == ' ' || text[i] == '\t') {
        i++;
    }
    if (text[i] == '-') {
        return false;
    }

    errno = 0;
    char *endptr = NULL;
    unsigned long value = strtoul(text, &endptr, 10);
    if (endptr == text || *endptr != '\0' || errno == ERANGE) {
        return false;
    }
    *out = value;
    return true;
}

/* Interpreta um int estritamente (sem lixo, sem overflow). A faixa de
 * int é verificada explicitamente: truncar um long silenciosamente
 * faria uma execução rodar com quantum/custo diferente do pedido, o
 * que corromperia o experimento sem nenhum aviso. */
static bool parse_int(const char *text, int *out) {
    if (text == NULL || text[0] == '\0') {
        return false;
    }
    errno = 0;
    char *endptr = NULL;
    long value = strtol(text, &endptr, 10);
    if (endptr == text || *endptr != '\0' || errno == ERANGE) {
        return false;
    }
    if (value < INT_MIN || value > INT_MAX) {
        return false;
    }
    *out = (int)value;
    return true;
}

/* Retorna o valor associado a --flag, ou NULL (com mensagem em
 * stderr) se o argumento seguinte não existir. */
static const char *next_arg(int argc, char **argv, int *i, const char *flag) {
    if (*i + 1 >= argc) {
        fprintf(stderr, "Erro: %s requer um valor.\n", flag);
        return NULL;
    }
    (*i)++;
    return argv[*i];
}

/* Interpreta todos os argumentos da linha de comando. Retorna true em
 * caso de sucesso; em caso de erro, já imprime a mensagem descritiva
 * (e o uso, quando aplicável) em stderr. *should_exit_success indica
 * --help (uso impresso em stdout, sucesso). */
static bool parse_args(
    int argc,
    char **argv,
    CliOptions *opts,
    bool *should_exit_success
) {
    *should_exit_success = false;

    opts->has_scenario = false;
    opts->has_seed = false;
    opts->has_algorithm = false;
    opts->n_processes = (size_t)CONFIG_MIN_PROCESSES;
    opts->quantum = CONFIG_RR_QUANTUM;
    opts->cs_cost = CONFIG_CONTEXT_SWITCH_COST;
    opts->print_header = false;
    opts->algorithm_name = NULL;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--help") == 0) {
            print_usage(stdout);
            *should_exit_success = true;
            return true;
        } else if (strcmp(arg, "--scenario") == 0) {
            const char *value = next_arg(argc, argv, &i, "--scenario");
            if (value == NULL) {
                return false;
            }
            if (!parse_scenario(value, &opts->scenario)) {
                fprintf(stderr,
                    "Erro: cenário inválido '%s'. Use balanced, io_bound, "
                    "cpu_bound ou priority_skewed.\n", value);
                return false;
            }
            opts->has_scenario = true;
        } else if (strcmp(arg, "--seed") == 0) {
            const char *value = next_arg(argc, argv, &i, "--seed");
            if (value == NULL) {
                return false;
            }
            if (!parse_ulong(value, &opts->seed)) {
                fprintf(stderr, "Erro: seed inválida '%s'.\n", value);
                return false;
            }
            /* O gerador de carga usa apenas os 32 bits baixos da seed
             * (rng_init em workload_generator.c). Sem este limite,
             * seeds distintas acima de 2^32 gerariam cargas idênticas
             * e a consolidação as contaria como réplicas independentes,
             * inflando a amostra com dados repetidos. */
            if (opts->seed > UINT32_MAX) {
                fprintf(stderr,
                    "Erro: seed '%s' excede o limite de 32 bits do gerador "
                    "de carga (máximo %u).\n", value, UINT32_MAX);
                return false;
            }
            opts->has_seed = true;
        } else if (strcmp(arg, "--algorithm") == 0) {
            const char *value = next_arg(argc, argv, &i, "--algorithm");
            if (value == NULL) {
                return false;
            }
            if (!parse_algorithm(value, &opts->algorithm)) {
                fprintf(stderr,
                    "Erro: algoritmo inválido '%s'. Use fcfs, round_robin, "
                    "priority ou predictive_sjf.\n", value);
                return false;
            }
            opts->algorithm_name = value;
            opts->has_algorithm = true;
        } else if (strcmp(arg, "--processes") == 0) {
            const char *value = next_arg(argc, argv, &i, "--processes");
            if (value == NULL) {
                return false;
            }
            unsigned long parsed;
            if (!parse_ulong(value, &parsed)) {
                fprintf(stderr, "Erro: número de processos inválido '%s'.\n", value);
                return false;
            }
            /* generate_workload faz malloc(sizeof(Process) * n) sem
             * checar overflow: um valor grande o bastante daria a volta
             * no size_t, alocaria quase nada e a carga seria escrita
             * fora do buffer. Barramos aqui. */
            if (parsed > SIZE_MAX / sizeof(Process)) {
                fprintf(stderr,
                    "Erro: número de processos '%s' é grande demais.\n", value);
                return false;
            }
            opts->n_processes = (size_t)parsed;
        } else if (strcmp(arg, "--quantum") == 0) {
            const char *value = next_arg(argc, argv, &i, "--quantum");
            if (value == NULL) {
                return false;
            }
            int parsed;
            if (!parse_int(value, &parsed)) {
                fprintf(stderr, "Erro: quantum inválido '%s'.\n", value);
                return false;
            }
            opts->quantum = parsed;
        } else if (strcmp(arg, "--cs-cost") == 0) {
            const char *value = next_arg(argc, argv, &i, "--cs-cost");
            if (value == NULL) {
                return false;
            }
            int parsed;
            if (!parse_int(value, &parsed)) {
                fprintf(stderr, "Erro: custo de troca de contexto inválido '%s'.\n", value);
                return false;
            }
            opts->cs_cost = parsed;
        } else if (strcmp(arg, "--header") == 0) {
            opts->print_header = true;
        } else {
            fprintf(stderr, "Erro: argumento desconhecido '%s'.\n", arg);
            return false;
        }
    }

    if (!opts->has_scenario || !opts->has_seed || !opts->has_algorithm) {
        fprintf(stderr,
            "Erro: --scenario, --seed e --algorithm são obrigatórios.\n");
        return false;
    }

    if (opts->n_processes == 0) {
        fprintf(stderr, "Erro: --processes deve ser maior que zero.\n");
        return false;
    }

    /* O quantum é validado para todos os algoritmos, e não só para o
     * Round Robin: ele é gravado na linha CSV e os scripts de
     * consolidação agrupam por essa coluna. Aceitar um valor absurdo
     * em fcfs/priority/predictive_sjf só porque ele é ignorado pela
     * simulação criaria uma configuração de experimento fantasma nos
     * resultados, em vez de um erro. */
    if (opts->quantum <= 0) {
        fprintf(stderr, "Erro: --quantum deve ser maior que zero.\n");
        return false;
    }

    if (opts->cs_cost < 0) {
        fprintf(stderr,
            "Erro: --cs-cost não pode ser negativo.\n");
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    CliOptions opts;
    bool should_exit_success = false;

    if (!parse_args(argc, argv, &opts, &should_exit_success)) {
        print_usage(stderr);
        return EXIT_FAILURE;
    }
    if (should_exit_success) {
        return EXIT_SUCCESS;
    }

    /* O SJF Preditivo mantém estado global (histórico de estimativas)
     * entre invocações de scheduler_select_predictive_sjf. Como cada
     * execução do binário é uma simulação isolada, zeramos esse estado
     * antes de começar para não herdar nada de execuções anteriores
     * no mesmo processo do SO (relevante sobretudo em testes). */
    if (opts.algorithm == SCHEDULER_CUSTOM) {
        scheduler_predictive_sjf_reset();
    }

    Workload workload = generate_workload(opts.scenario, opts.seed, opts.n_processes);
    if (workload.processes == NULL || workload.count == 0) {
        fprintf(stderr, "Erro: falha ao gerar a carga de trabalho.\n");
        free_workload(&workload);
        return EXIT_FAILURE;
    }

    SimulationConfig sim_config;
    sim_config.algorithm = opts.algorithm;
    sim_config.scheduler.quantum = opts.quantum;
    sim_config.scheduler.context_switch_cost = opts.cs_cost;

    SimulationResult result;
    if (!simulator_run(&workload, &sim_config, &result)) {
        fprintf(stderr, "Erro: falha ao executar a simulação (entrada inválida).\n");
        free_workload(&workload);
        return EXIT_FAILURE;
    }

    SimulationMetrics metrics;
    if (!metrics_calculate(result.samples, result.count, result.context_switches, &metrics)) {
        fprintf(stderr, "Erro: falha ao calcular métricas (resultado não finito).\n");
        simulator_free_result(&result);
        free_workload(&workload);
        return EXIT_FAILURE;
    }

    if (opts.print_header) {
        printf("scenario,seed,algorithm,n_processes,quantum,context_switch_cost,"
               "mean_turnaround,context_switches,jain_slowdown_percent,total_time,"
               "cpu_busy_time,context_switch_time,idle_time\n");
    }

    printf("%s,%lu,%s,%zu,%d,%d,%.6f,%zu,%.6f,%d,%d,%d,%d\n",
        scenario_name(opts.scenario),
        opts.seed,
        opts.algorithm_name,
        opts.n_processes,
        opts.quantum,
        opts.cs_cost,
        metrics.mean_turnaround,
        metrics.context_switches,
        metrics.jain_slowdown_percent,
        result.total_time,
        result.cpu_busy_time,
        result.context_switch_time,
        result.idle_time);

    simulator_free_result(&result);
    free_workload(&workload);

    return EXIT_SUCCESS;
}
