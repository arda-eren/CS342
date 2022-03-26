#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


//Definition for the process control block
//holds information about the related thread
typedef struct PCB{
    int pid;
    pthread_t thread_id;
    process_state process_state;
    int next_CPU_burst_length;
    int remaining_CPU_burst_length;
    int burst_count;
    int time_in_ready_queue;
    int device1_io_counter;
    int device2_io_counter;
    int start_time;
    int finish_time;
    int total_time_in_cpu;
}PCB;


//Defines the type of burst distribution
//used to seperate distributions easily
typedef enum burst_distribution_type{
    UNIFORM,
    EXPONENTIAL,
    FIXED
}burst_distribution_type;


//Defines the type of process states
//used to seperate the states of processes easily
typedef enum process_state{
    WAITING,
    READY,
    RUNNING
}process_state;


//Defines the scheduling algorithm to be run
//used to seperate the algorithm for cpu easily
typedef enum scheduling_algorithm{
    FCFS,
    SJF,
    RR
}scheduling_algorithm;





