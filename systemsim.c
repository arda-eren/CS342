#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include "scheduler_defs.h"

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
int current_thread_count; //Number of threads created
int current_time; //Current time
int* pid_list; //List of pids

 void initialize_list(int *pid_list){
    pid_list = malloc(sizeof(int) * maxp);
    for(int i = 0; i < maxp; i++){
        pid_list[i] = -1;
    }
    return pid_list;
}

int set_pid(int *pid_list){
    for (int i = 0; i < maxp; i++)
    {
        if (pid_list[i] == -1)
        {
            pid_list[i] = i;
            return i;
        }
    }
}

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
}

//Function that will generate new processes
void* generate_processes(void *arg){
    int total_thread_count = 0;
    
    if (allp < 10)
    {
       for (int i = 0; i < maxp; i++)
       {
            PCB *pcb = (PCB*)malloc(sizeof(PCB));
            int temp = set_pid(pid_list);
            pcb->pid = temp;
            pthread_create(&threads[temp], NULL, p_thread_func, (void*) pcb);
            pthread_join(threads[temp], NULL);
            current_thread_count++;
            total_thread_count++;
       }            
    } else{
        for (size_t i = 0; i < 10; i++)
        {
            PCB *pcb = (PCB*)malloc(sizeof(PCB));
            int temp = set_pid(pid_list);
            pcb->pid = temp;
            pthread_create(&threads[temp], NULL, p_thread_func, (void*) pcb);
            pthread_join(threads[temp], NULL);
            current_thread_count++;
            total_thread_count++;
        }
    }

    while (total_thread_count < allp)
    {
        usleep(5000);
        current_time += 5;
        if (current_thread_count < maxp)
        {
            int rand_num = rand() % 100;
            if (rand_num < pg * 100)
            {
                PCB *pcb = (PCB*)malloc(sizeof(PCB));
                pcb->pid = total_thread_count; //TODO: available pid finder for threads will be used here
                pthread_create(&threads[total_thread_count], NULL, p_thread_func, (void*) pcb);
                pthread_join(threads[total_thread_count], NULL);
                current_thread_count++;
                total_thread_count++;
            }
        }
    }
    return NULL;
}

//TODO: Implement the cpu scheduler
void* cpu_scheduler(void *arg){}

//TODO: Implement the main function
int main(int argc, char *argv[]){
    if (argc != 16){
        printf("Invalid number of arguments\n");
        return 1;
    } else {
        if (argv[1] == "fcfs"){
            alg = FCFS;
        } else if (argv[1] == "sjf"){
            alg = SJF;
        } else if (argv[1] == "rr"){
            alg = RR;
        } else {
            printf("Invalid scheduling algorithm\n");
            return 1;
        }
        q = atoi(argv[2]);
        t1 = atoi(argv[3]);
        t2 = atoi(argv[4]);
        if (strcmp(argv[5], "uniform") != 0 && strcmp(argv[5], "exponential") != 0 && strcmp(argv[5], "fixed") != 0){
            printf("Invalid burst distribution argument\n");
            return 1;
        } else {
            burst_dist = argv[5];
        }
        min_burst = atoi(argv[6]);
        max_burst = atoi(argv[7]);
        burst_len = atoi(argv[8]);
        p0 = atof(argv[9]);
        p1 = atof(argv[10]);
        p2 = atof(argv[11]);
        if (p0 + p1 + p2 != 1){
            printf("Invalid p0, p1, p2 arguments\n");
            return 1;
        }
        pg = atof(argv[12]);
        maxp = atoi(argv[13]);
        allp = atoi(argv[14]);
        if (maxp > allp || maxp < 1 || maxp > 50 || allp < 1 || allp > 1000){
            printf("Invalid allp, maxp arguments\n");
            return 1;
        }
        if(argv[15] >= 1 && argv[15] <= 3){
            outmode = atoi(argv[15]);
        } else {
            printf("Invalid outmode argument\n");
            return 1;
        }
        threads = (pthread_t*)malloc(sizeof(pthread_t) * allp);
        initialize_list(pid_list);
        pthread_t process_generator; //Process generator thread
        pthread_t cpu_scheduler; //CPU scheduler thread
        pthread_create(&process_generator, NULL, generate_processes, NULL);
        pthread_join(process_generator, NULL);
        pthread_create(&cpu_scheduler, NULL, cpu_scheduler, NULL);
        pthread_join(cpu_scheduler, NULL);
        return 1;
    }
    
}

