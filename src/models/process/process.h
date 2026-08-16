#ifndef PROCESS_H
#define PROCESS_H

enum BurstType {
    CPU,
    IO,
};

typedef struct {
    enum BurstType type;
    int duration;
} Burst;

enum Process_possible_states {
    NEW,
    READY,
    EXECUTING,
    BLOCKED,
    FINISHED,
};

typedef struct {
    int pid;
    int arrival_time; // instante da simulação a qual o processo entrou na fila
    int priority;
    enum Process_possible_states state;
    Burst *bursts;
    int num_bursts;
    int current_burst_index;
} Process;

Process process_create(int pid, int arrival_time, int priority, int num_bursts);
void set_state(Process *p, enum Process_possible_states state);

int calculate_remaining_time(Process *p);
int calculate_current_burst_remaining_time(Process *p, int current_time);

#endif
