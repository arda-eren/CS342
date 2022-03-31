#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


//Defines the type of process states
//used to seperate the states of processes easily
typedef enum process_state{
    WAITING,
    READY,
    RUNNING,
    TERMINATED,
    USING_IO1,
    USING_IO2
}process_state;

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
    int last_ready_queue_enterance;
}PCB;


//Defines the scheduling algorithm to be run
//used to seperate the algorithm for cpu easily
typedef enum scheduling_algorithm{
    FCFS,
    SJF,
    RR
}scheduling_algorithm;





