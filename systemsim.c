#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include "scheduler_definitions.h"

scheduling_algorithm alg; //Scheduling algorithm type
int q; //Time quantum
int t1; //Service time for IO device 1
int t2; //Service time for IO device 2
char *burst_dist; //Burst distribution type
int burst_len; //Burst length
int min_burst; //Minimum burst length
int max_burst; //Maximum burst length
float p0; //Probability of terminating a process
float p1; //Probability of running thread going to IO device 1
float p2; //Probability of running thread going to IO device 2
float pg; //Probability of generating a new process
int maxp; //Maximum number of process threads that can exit simultaneously
int allp; //Number of threads that will be created until simulation ends
int outmode; //Output mode

pthread_t *threads; //Array of threads
//queue *ready_queue; //Ready queue
//queue *io1_queue; //IO queue for device 1
//queue *io2_queue; //IO queue for device 2
pthread_mutex_t mutex_rq; //Mutex for threads in ready queue
pthread_mutex_t mutex_io1; //Mutex for threads in io1 queue
pthread_mutex_t mutex_io2; //Mutex for threads in io2 queue
pthread_cond_t cond_rq; //Condition variable for threads in ready queue
pthread_cond_t cond_io1; //Condition variable for threads in io1 queue
pthread_cond_t cond_io2; //Condition variable for threads in io2 queue
int thread_count; //Number of threads created
int current_time; //Current time

void* p_thread_func(void *arg){
    PCB *pcb = (PCB*)arg;
    pcb->start_time = current_time;
    pcb->finish_time = 0;
    pcb->process_state = READY;
    pcb->thread_id = pthread_self();
    pcb->burst_count = 0;
    pcb->device1_io_counter = 0;
    pcb->device2_io_counter = 0;
    pcb->total_time_in_cpu = 0;
    pcb->time_in_ready_queue = 0;

    if (strcmp(burst_dist, "uniform") == 0){
        pcb->remaining_CPU_burst_length = rand() % (max_burst - min_burst + 1) + min_burst;
        pcb->next_CPU_burst_length = (rand() % (max_burst - min_burst + 1)) + min_burst;
    } else if (strcmp(burst_dist, "exponential") == 0){
        pcb->remaining_CPU_burst_length = (int)(-1 * log(1 - ((float)rand() / RAND_MAX)) * (max_burst - min_burst + 1)) + min_burst;
        pcb->next_CPU_burst_length = (int)(-1 * log(1 - ((float)rand() / RAND_MAX)) * (max_burst - min_burst + 1)) + min_burst; 
    } else if (strcmp(burst_dist, "fixed") == 0){
        pcb->remaining_CPU_burst_length = burst_len;
        pcb->next_CPU_burst_length = burst_len;
    }
    //TODO: Enqueue the PCB into the ready queue
    thread_count--;
    pthread_exit(NULL);
}

//TODO: Implement the function that will generate new processes
void* generate_processes(void *arg){
    while (thread_count < allp){
        PCB *pcb = (PCB*)malloc(sizeof(PCB));
        pthread_create(&pcb->thread_id, NULL, p_thread_func, pcb);
        pthread_join(pcb->thread_id, NULL);
        thread_count++;
    }
    pthread_exit(NULL);
}

//TODO: Implement the cpu scheduler
void* cpu_scheduler(void *arg){}

//TODO: Implement the main function
int main(int argc, char *argv[]){}

