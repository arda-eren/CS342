#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <pthread.h>
#include "queue_impl.h"

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
queue *ready_queue; //Ready queue
//queue *io1_queue; //IO queue for device 1
//queue *io2_queue; //IO queue for device 2
pthread_mutex_t mutex_rq; //Mutex for threads in ready queue
pthread_mutex_t mutex_io1; //Mutex for threads in io1 queue
pthread_mutex_t mutex_io2; //Mutex for threads in io2 queue
pthread_cond_t cond_rq; //Condition variable for threads in ready queue
//pthread_cond_t cond_thread; //Condition variable for threads
pthread_cond_t cond_io1; //Condition variable for threads in io1 queue
pthread_cond_t cond_io2; //Condition variable for threads in io2 queue
int current_thread_count; //Number of threads created
int threads_remaining; //Number of threads remaining
int current_time; //Current time
int cpu_thread_pid; //PID of the thread currently running on the CPU
int cpu_thread_count; //Number of threads running on the CPU
int io1_thread_count; //Number of threads running on device 1
int io2_thread_count; //Number of threads running on device 2

int min(int a, int b){
    if(a < b)
        return a;
    else
        return b;
}

void calculate_next_CPU_burst(PCB* pcb, scheduling_algorithm alg){
    if (alg == RR)
    {
        pcb->next_CPU_burst_length = min(pcb->next_CPU_burst_length, q);
    } else {
        if (strcmp(burst_dist, "uniform") == 0){
            pcb->next_CPU_burst_length = (rand() % (max_burst - min_burst + 1)) + min_burst;
        } else if (strcmp(burst_dist, "exponential") == 0){
            pcb->next_CPU_burst_length = (int)(-1 * log(1 - ((float)rand() / RAND_MAX)) * (max_burst - min_burst + 1)) + min_burst; 
        } else if (strcmp(burst_dist, "fixed") == 0){
            pcb->next_CPU_burst_length = burst_len;
        }
    }
}

void* pthread_func(void *arg){
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
    } else if (strcmp(burst_dist, "exponential") == 0){
        pcb->remaining_CPU_burst_length = (int)(-1 * log(1 - ((float)rand() / RAND_MAX)) * (max_burst - min_burst + 1)) + min_burst;
    } else if (strcmp(burst_dist, "fixed") == 0){
        pcb->remaining_CPU_burst_length = burst_len;
    }

    calculate_next_CPU_burst(pcb, alg);
    pthread_mutex_lock(&mutex_rq);
    enqueue(ready_queue, pcb, alg);
    pcb->last_ready_queue_enterance = current_time;
    printf("Thread with pid: %d at %d\n", pcb->pid, current_time);
    pthread_mutex_unlock(&mutex_rq);
    pthread_cond_broadcast(&cond_rq);
    
    while (pcb->process_state != TERMINATED){
        pthread_mutex_lock(&mutex_rq);
        while (cpu_thread_count > 0 && pcb->pid != cpu_thread_pid && cpu_thread_pid != -1){
            pthread_cond_wait(&cond_rq, &mutex_rq);
        }
        if (cpu_thread_pid == -1){
            pthread_mutex_unlock(&mutex_rq);
            pthread_cond_broadcast(&cond_rq);
            continue;
        }
        pthread_mutex_unlock(&mutex_rq);
        pthread_mutex_lock(&mutex_rq);
        node *holding_node = dequeue(ready_queue);
        PCB *temp_pcb = &(holding_node->process_data);
        temp_pcb->time_in_ready_queue = current_time - temp_pcb->last_ready_queue_enterance;
        temp_pcb->last_ready_queue_enterance = 0;
        cpu_thread_count++;
        pthread_mutex_unlock(&mutex_rq);
        pthread_cond_broadcast(&cond_rq);

        if (alg == RR)
        {
            temp_pcb->process_state = RUNNING;
            temp_pcb->remaining_CPU_burst_length = temp_pcb->next_CPU_burst_length;
            temp_pcb->burst_count = 0;
            calculate_next_CPU_burst(temp_pcb, alg);
            temp_pcb->total_time_in_cpu += temp_pcb->next_CPU_burst_length ;
            current_time += temp_pcb->next_CPU_burst_length;
            usleep(temp_pcb->next_CPU_burst_length * 1000);
            printf("Thread %d is running at time: %d\n", temp_pcb->pid, current_time);
            pthread_mutex_lock(&mutex_rq);
            if (temp_pcb->remaining_CPU_burst_length != 0){
                enqueue(ready_queue, temp_pcb, alg);
                temp_pcb->last_ready_queue_enterance = current_time;
            }
            cpu_thread_count--;
            pthread_mutex_unlock(&mutex_rq);
            pthread_cond_broadcast(&cond_rq);
        }
        else
        {
            temp_pcb->process_state = RUNNING;
            temp_pcb->remaining_CPU_burst_length = pcb->next_CPU_burst_length;
            temp_pcb->burst_count = 0;
            calculate_next_CPU_burst(temp_pcb, alg);
            temp_pcb->total_time_in_cpu += temp_pcb->next_CPU_burst_length;
            current_time += temp_pcb->next_CPU_burst_length;
            usleep(temp_pcb->next_CPU_burst_length * 1000);
            temp_pcb->remaining_CPU_burst_length = 0;
            printf("Thread %d is running at time: %d\n", temp_pcb->pid, current_time);
            cpu_thread_count--;
            pthread_cond_broadcast(&cond_rq);
        }

        int cpu_decision_prob = rand() % 100;
        if (cpu_decision_prob < p0 * 100)
        {
            pcb->process_state = TERMINATED;    
            temp_pcb->process_state = TERMINATED;
            pthread_cond_broadcast(&cond_rq);
        }
        else if (cpu_decision_prob < (p0 + p1) * 100)
        {
            pthread_mutex_lock(&mutex_io1);
            while (io1_thread_count > 0)
            {
                pthread_cond_wait(&cond_io1, &mutex_io1);
            }
            temp_pcb->process_state = USING_IO1;
            io1_thread_count++;
            printf("Thread %d is using IO1 at time: %d\n\n", temp_pcb->pid, current_time);
            usleep(t1 * 1000);
            temp_pcb->device1_io_counter++;
            current_time += t1;
            io1_thread_count--;
            printf("Thread %d finished using IO1 at time: %d\n", temp_pcb->pid, current_time);
            pthread_mutex_unlock(&mutex_io1);
            pthread_cond_signal(&cond_io1);
            pthread_mutex_lock(&mutex_rq);
            temp_pcb->process_state = READY;
            enqueue(ready_queue, temp_pcb, alg);
            temp_pcb->last_ready_queue_enterance = current_time;
            pthread_mutex_unlock(&mutex_rq);
            calculate_next_CPU_burst(temp_pcb, alg);
            pthread_cond_broadcast(&cond_rq);
        }
        else
        {
            pthread_mutex_lock(&mutex_io2);
            while (io2_thread_count > 0)
            {
                pthread_cond_wait(&cond_io2, &mutex_io2);
            }
            temp_pcb->process_state = USING_IO2;
            io1_thread_count++;
            printf("Thread %d is using IO2 at time: %d\n\n", temp_pcb->pid, current_time);
            usleep(t2 * 1000);
            temp_pcb->device2_io_counter++;
            current_time += t2;
            io2_thread_count--;
            printf("Thread %d finished using IO2 at time: %d\n", temp_pcb->pid, current_time);
            pthread_mutex_unlock(&mutex_io2);
            pthread_cond_signal(&cond_io2);
            pthread_mutex_lock(&mutex_rq);
            temp_pcb->process_state = READY;
            enqueue(ready_queue, temp_pcb, alg);
            temp_pcb->last_ready_queue_enterance = current_time;
            pthread_mutex_unlock(&mutex_rq);
            calculate_next_CPU_burst(temp_pcb, alg);
            pthread_cond_broadcast(&cond_rq);
        }   
    }
    threads_remaining--;
    current_thread_count--;
    printf("Thread %d is exiting at time: %d\n", pcb->pid, current_time);
    pthread_exit(NULL);
    return NULL;
}

//Function that will generate new processes
void* generate_processes(void *arg){
    int total_thread_count = 0;
    cpu_thread_pid = -1;
    if (maxp < 10)
    {
       for (int i = 0; i < maxp; i++)
        {
            PCB *pcb = (PCB*)malloc(sizeof(PCB));
            pcb->pid = i;
            pthread_create(&threads[i], NULL, pthread_func, (void*) pcb);
            current_thread_count++;
            total_thread_count++;
        }      
        
        while (total_thread_count < allp)
        {
            if (current_thread_count < maxp)
            {
                usleep(5000);
                current_time += 5;
                int rand_num = rand() % 100;
                if (rand_num < pg * 100)
                {
                    PCB *pcb = (PCB*)malloc(sizeof(PCB));
                    pcb->pid = total_thread_count; //TODO: available pid finder for threads will be used here
                    pthread_create(&threads[total_thread_count], NULL, pthread_func, (void*) pcb);
                    current_thread_count++;
                    total_thread_count++;
                }
            }
        }      
    } else{
        for (size_t i = 0; i < 10; i++)
        {
            PCB *pcb = (PCB*)malloc(sizeof(PCB));
            pcb->pid = i;
            pthread_create(&threads[i], NULL, pthread_func, (void*) pcb);
            current_thread_count++;
            total_thread_count++;
        }

        while (total_thread_count != allp)
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
                    pthread_create(&threads[total_thread_count], NULL, pthread_func, (void*) pcb);
                    current_thread_count++;
                    total_thread_count++;
                }
            }
        }
    }
    return NULL;
}

//TODO: Implement the cpu scheduler
void* cpu_scheduler(void *arg){
    while (1)
    {
        if(threads_remaining == 0)
        {
            break;
        } else {
            pthread_mutex_lock (&mutex_rq);
            while (cpu_thread_count > 0)
            {
                pthread_cond_wait(&cond_rq, &mutex_rq);
            }
             if (ready_queue->front != NULL)
            {
                cpu_thread_pid = ready_queue->front->process_data.pid;
            }
            else
            {
                cpu_thread_pid = -1;
            }
            pthread_mutex_unlock(&mutex_rq);
            pthread_cond_broadcast(&cond_rq);            
        }
    }
    return NULL;
}

//TODO: Implement the main function
int main(int argc, char *argv[]){
    if (argc != 16){
        printf("Invalid number of arguments\n");
        return 1;
    } 
    else {

        if (strcmp(argv[1], "FCFS") == 0){
            alg = FCFS;
        } else if (strcmp(argv[1], "SJF") == 0){
            alg = SJF;
        } else if (strcmp(argv[1], "RR") == 0){
            alg = RR;
        } else {
            printf("Invalid scheduling algorithm\n");
            return 1;
        }

        if (strcmp(argv[2], "INF"))
        {
            q = 1;
        } else {
            q = atoi(argv[2]);
        }
        
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

        if((int)argv[15] >= 1 || (int)argv[15] <= 3){
            outmode = atoi(argv[15]);
        } else {
            printf("Invalid outmode argument\n");
            return 1;
        }
        printf("Valid arguments\n");

        pthread_mutex_init(&mutex_rq, NULL);
        pthread_mutex_init(&mutex_io1, NULL);
        pthread_mutex_init(&mutex_io2, NULL);
        pthread_cond_init(&cond_rq, NULL);
        pthread_cond_init(&cond_io1, NULL);
        pthread_cond_init(&cond_io2, NULL);

        threads_remaining = allp;
        cpu_thread_count = 0;

        ready_queue = create_queue();
        threads = (pthread_t*)malloc(sizeof(pthread_t) * allp);
        pthread_t process_generator; //Process generator thread
        pthread_t scheduler; //CPU scheduler thread
        pthread_create(&process_generator, NULL, &generate_processes, NULL);
        pthread_join(process_generator, NULL);
        pthread_create(&scheduler, NULL, &cpu_scheduler, NULL);
        pthread_join(scheduler, NULL);

        for (int i = 0; i < maxp; i++)
        {
            pthread_join(threads[i], NULL);
        }
        print_queue(ready_queue);
        pthread_mutex_destroy(&mutex_rq);
        pthread_mutex_destroy(&mutex_io1);
        pthread_mutex_destroy(&mutex_io2);
        pthread_cond_destroy(&cond_rq);
        pthread_cond_destroy(&cond_io1);
        pthread_cond_destroy(&cond_io2);
        free(threads);
        free(ready_queue);
    }
    return 1;
}

