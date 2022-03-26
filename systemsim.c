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
burst_distribution_type burst_dist; //Burst distribution type
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

void* p_thread_func(void *arg){
}