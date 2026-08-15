#include "process.h"
#include "stdio.h"
#include "stdlib.h"

Burst burst_create(enum BurstType type, int duration) {
    Burst b;
    b.type = type;
    b.duration = duration;
    return b;
}

Burst *init_bursts_list(int num_bursts) {
    Burst *bursts = (Burst *)malloc(num_bursts * sizeof(Burst));
    if (bursts == NULL) {
        fprintf(stderr, "Erro: não foi possível alocar memória para a lista de rajadas.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_bursts; i++) {
        bursts[i] = burst_create(i % 2 == 0 ? CPU : IO, 1);
    }

    return bursts;
}

Process process_create(int pid, int arrival_time, int priority, int num_bursts) {
    Process p;
    p.pid = pid;
    p.arrival_time = arrival_time;
    p.priority = priority;
    p.state = NEW;
    p.bursts = init_bursts_list(num_bursts);
    p.num_bursts = num_bursts;
    p.current_burst_index = 0;
    return p;
}

void set_state(Process *p, enum Process_possible_states state) {
    p->state = state;
}

int calculate_remaining_time(Process *p) {
    int remaining = 0;
    for (int i = p->current_burst_index; i < p->num_bursts; i++) {
        remaining += p->bursts[i].duration;
    }
    return remaining;
}

int calculate_current_burst_remaining_time(Process *p, int current_time) {
    int remaining = p->bursts[p->current_burst_index].duration - (current_time - p->arrival_time);
    return remaining > 0 ? remaining : 0;
}
