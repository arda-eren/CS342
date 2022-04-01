#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include "queue_impl.h"

struct timeval start_time, cur_time;
long int beginning_time;
struct queue* final_queue;
enum scheduling_algorithm alg; //Scheduling algorithm type

pthread_t *threads; //Array of threads
queue *ready_queue; //Ready queue
//queue *io1_queue; //IO queue for device 1
//queue *io2_queue; //IO queue for device 2

float p2; //Probability of terminating a process
float p1; //Probability of running thread going to IO device 1
float p0; //Probability of running thread going to IO device 2
float pg; //Probability of generating a new process

int q; //Time quantum

int t1; //Service time for IO device 1
int t2; //Service time for IO device 2

char *burst_dist; //Burst distribution type

int maxp; //Maximum number of process threads that can exit simultaneously
int allp; //Number of threads that will be created until simulation ends
int outmode; //Output mode

int burst_len; //Burst length
int max_burst; //Minimum burst length
int min_burst; //Maximum burst length

pthread_mutex_t mutex_clock;
pthread_mutex_t mutex_print;
pthread_mutex_t mutex_rq; //Mutex for threads in ready queue
pthread_mutex_t mutex_io1; //Mutex for threads in io1 queue
pthread_mutex_t mutex_io2; //Mutex for threads in io2 queue

int current_thread_count; //Number of threads created
int threads_remaining; //Number of threads remaining
int cpu_thread_pid; //PID of the thread currently running on the CPU

int cpu_thread_count; //Number of threads running on the CPU
int io2_thread_count; //Number of threads running on device 1
int io1_thread_count; //Number of threads running on device 2

pthread_cond_t cond_rq; //Condition variable for threads in ready queue
//pthread_cond_t cond_thread; //Condition variable for threads
pthread_cond_t cond_io1; //Condition variable for threads in io1 queue
pthread_cond_t cond_io2; //Condition variable for threads in io2 queue


int min(int a, int b){
    if(!(a < b)){
        return b;
    }else{
        return a;
    }
}

void calculate_next_CPU_burst(PCB* pcb, scheduling_algorithm alg){
    if (alg != RR){
        if (strcmp(burst_dist, "uniform") == 0){
            pcb->next_CPU_burst_length = (rand() % (max_burst - min_burst + 1)) + min_burst;
        } else if (strcmp(burst_dist, "exponential") == 0){
            pcb->next_CPU_burst_length = (int)(-1 * log(1 - ((float)rand() / RAND_MAX)) * (max_burst - min_burst + 1)) + min_burst; 
        } else if (strcmp(burst_dist, "fixed") == 0){
            pcb->next_CPU_burst_length = burst_len;
        }
    } else {
        if(pcb->remaining_CPU_burst_length != 0){
            pcb->next_CPU_burst_length = min(pcb->remaining_CPU_burst_length, q);    
        }else{
            pcb->remaining_CPU_burst_length = q;
            pcb->next_CPU_burst_length = min(pcb->remaining_CPU_burst_length, q);
        }
    }
}

void* pthread_func(void *arg){
    PCB *pcb = (PCB*)arg;

    if(outmode == 3){
        printf("Thread %d created\n", pcb->pid);
    }

    //pcb->start_time = cur_time;
    pcb->finish_time = 0;
    pcb->process_state = READY;
    pthread_mutex_lock(&mutex_clock);
    gettimeofday(&cur_time, NULL);
    pcb->arr_time = (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time);
    pthread_mutex_unlock(&mutex_clock);
    pcb->thread_id = pthread_self();
    pcb->burst_count = 0;
    pcb->device1_io_counter = 0;
    pcb->device2_io_counter = 0;
    pcb->total_time_in_cpu = 0;
    pcb->time_in_ready_queue = 0;
    
    pthread_mutex_lock(&mutex_clock);
    pcb->last_ready_queue_enterance = (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time);
    pthread_mutex_unlock(&mutex_clock);

    if (strcmp(burst_dist, "uniform") == 0){
        pcb->remaining_CPU_burst_length = rand() % (max_burst - min_burst + 1) + min_burst;
    } else if (strcmp(burst_dist, "exponential") == 0){
        pcb->remaining_CPU_burst_length = (int)(-1 * log(1 - ((float)rand() / RAND_MAX)) * (max_burst - min_burst + 1)) + min_burst;
    } else if (strcmp(burst_dist, "fixed") == 0){
        pcb->remaining_CPU_burst_length = burst_len;
    }

    calculate_next_CPU_burst(pcb, alg);
    pthread_mutex_lock(&mutex_rq);
    pthread_mutex_lock(&mutex_clock);
    gettimeofday(&cur_time, NULL);

    if(outmode == 3){
        printf("Thread %d is added to the ready queue at time: %ld\n", pcb->pid, ((cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time)));
    }

    pthread_mutex_unlock(&mutex_clock);
    enqueue(ready_queue, pcb, alg);
    printf("Thread with pid: %d enqueued at %d\n\n", pcb->pid, cur_time);
    pthread_mutex_unlock(&mutex_rq);
    pthread_cond_broadcast(&cond_rq);
    
    while (pcb->process_state != TERMINATED){
        pthread_mutex_lock(&mutex_rq);
        while ((cpu_thread_count > 0 || pcb->pid != cpu_thread_pid) && cpu_thread_pid != -1){
            pthread_cond_wait(&cond_rq, &mutex_rq);
        }
        if (cpu_thread_pid == -1){
            pthread_mutex_unlock(&mutex_rq);
            pthread_cond_broadcast(&cond_rq);
            continue;
        }
        pthread_mutex_unlock(&mutex_rq);
        pthread_mutex_lock(&mutex_rq);
        pthread_mutex_lock(&mutex_clock);
        gettimeofday(&cur_time, NULL);
        pthread_mutex_unlock(&mutex_clock);
        printf("Thread with pid: %d dequeued at %d\n\n", pcb->pid, cur_time);
        node *holding_node = dequeue(ready_queue);

        pthread_mutex_lock(&mutex_clock);
        if(outmode == 3){
        printf("Thread %d is selected for: %ld\n", pcb->pid, ((cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time)));
        }    
        pthread_mutex_unlock(&mutex_clock);

        PCB *temp_pcb = &(holding_node->process_data);
        //temp_pcb->time_in_ready_queue = cur_time - temp_pcb->last_ready_queue_enterance;
        //temp_pcb->last_ready_queue_enterance = 0;
        cpu_thread_count++;
        pthread_mutex_unlock(&mutex_rq);
        pthread_cond_broadcast(&cond_rq);

        if (alg == RR)
        {
            temp_pcb->process_state = RUNNING;
            temp_pcb->remaining_CPU_burst_length = temp_pcb->next_CPU_burst_length;
            
            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            temp_pcb->time_in_ready_queue += (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (temp_pcb->last_ready_queue_enterance);
            pthread_mutex_unlock(&mutex_clock);
            
            //temp_pcb->burst_count = 0;
            calculate_next_CPU_burst(temp_pcb, alg);
            temp_pcb->total_time_in_cpu += temp_pcb->next_CPU_burst_length ;

            pthread_mutex_lock(&mutex_clock);
            if (outmode == 3){
                printf("Thread %d is running at time: %ld\n", temp_pcb->pid, (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time));
            }
            if (outmode == 2){
                printf("%ld %d RUNNING\n", (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - beginning_time, temp_pcb->pid);
            }
            pthread_mutex_unlock(&mutex_clock);
            
            //cur_time += temp_pcb->next_CPU_burst_length;
            usleep(temp_pcb->next_CPU_burst_length * 1000);
            temp_pcb->remaining_CPU_burst_length -= temp_pcb->next_CPU_burst_length;
            printf("Thread %d is running at time: %d\n\n", temp_pcb->pid, cur_time);
            
            pthread_mutex_lock(&mutex_rq);
            if (temp_pcb->remaining_CPU_burst_length != 0){
                pthread_mutex_lock(&mutex_clock);
                gettimeofday(&cur_time, NULL);
                pcb->last_ready_queue_enterance = (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time);
                enqueue(ready_queue, temp_pcb, alg);
                
                if(outmode == 3){
                    printf("Thread %d is added to the ready queue at time: %ld\n", pcb->pid, ((cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time)));
                }
                pthread_mutex_unlock(&mutex_clock);
                pthread_cond_broadcast(&cond_rq);
                continue;
                
                //printf("Thread %d enqueued after running in CPU at %d\n\n", temp_pcb->pid, cur_time);
                //temp_pcb->last_ready_queue_enterance = cur_time;
            }else{ 
                if (outmode == 3){
                    printf("Q expired for thread %d at: %ld\n", temp_pcb->pid, (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - beginning_time);
                }
            }
            cpu_thread_count--;

            pthread_mutex_unlock(&mutex_rq);
            pthread_cond_broadcast(&cond_rq);
        }else{
            temp_pcb->process_state = RUNNING;
            
            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            temp_pcb->time_in_ready_queue += (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (temp_pcb->last_ready_queue_enterance);
            pthread_mutex_unlock(&mutex_clock);

            //temp_pcb->burst_count = 0;
            calculate_next_CPU_burst(temp_pcb, alg);
            temp_pcb->total_time_in_cpu += temp_pcb->next_CPU_burst_length;
            //cur_time += temp_pcb->next_CPU_burst_length;
            
            pthread_mutex_lock(&mutex_clock);
            if (outmode == 3){
                printf("Thread %d is running at time: %ld\n", temp_pcb->pid, (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time));
            }
            if (outmode == 2){
                printf("%ld %d RUNNING\n", (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - beginning_time, temp_pcb->pid);
            }
            usleep(temp_pcb->next_CPU_burst_length * 1000);
            pthread_mutex_unlock(&mutex_clock);

            temp_pcb->remaining_CPU_burst_length = 0;
            printf("Thread %d is running at time: %d\n\n", temp_pcb->pid, cur_time);
            cpu_thread_count--;
            
            pthread_cond_broadcast(&cond_rq);
        }

        int cpu_decision_prob = rand() % 100;
        if (cpu_decision_prob < p0 * 100)
        {
            pcb->process_state = TERMINATED;    
            temp_pcb->process_state = TERMINATED;
            
            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            temp_pcb->finish_time = (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - beginning_time;
            pthread_mutex_unlock(&mutex_clock);

            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            if (outmode == 3){
                printf("Thread %d is terminated at time: %ld\n", temp_pcb->pid, (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time));
            }
            pthread_mutex_unlock(&mutex_clock);
            enqueue(final_queue, temp_pcb, FCFS);
            pthread_cond_broadcast(&cond_rq);
        }else if (cpu_decision_prob <= (p0 + p1) * 100){
            pthread_mutex_lock(&mutex_io1);
            while (io1_thread_count > 0)
            {
                pthread_cond_wait(&cond_io1, &mutex_io1);
            }
            temp_pcb->process_state = USING_IO1;
            io1_thread_count++;

            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            if (outmode == 3){
                printf("Thread %d is using device 1 at time: %ld\n", temp_pcb->pid, (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time));
            }
            if (outmode == 2){
                printf("%ld %d USING DEVICE 1\n", (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - beginning_time, temp_pcb->pid);
            }
            pthread_mutex_unlock(&mutex_clock);

            printf("Thread %d is using IO1 at time: %d\n\n", temp_pcb->pid, cur_time);
            usleep(t1 * 1000);
            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            pthread_mutex_unlock(&mutex_clock);
            temp_pcb->device1_io_counter++;
            temp_pcb->burst_count++;
            io1_thread_count--;
            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            if (outmode == 3){
                printf("Thread %d finished using device 1 at time: %ld\n", temp_pcb->pid, (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time));
            }
            pthread_mutex_unlock(&mutex_clock);
            printf("Thread %d finished using IO1 at time: %d\n\n", temp_pcb->pid, cur_time);
            pthread_mutex_unlock(&mutex_io1);
            pthread_cond_signal(&cond_io1);
            pthread_mutex_lock(&mutex_rq);
            temp_pcb->process_state = READY;
            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            pcb->last_ready_queue_enterance = (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time);
            enqueue(ready_queue, temp_pcb, alg);
            pthread_cond_broadcast(&cond_rq);
            if(outmode == 3){
                printf("Thread %d is added to the ready queue at time: %ld\n", pcb->pid, ((cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time)));
            }
            pthread_mutex_unlock(&mutex_clock);
            printf("Thread %d enqueued after using IO1 at %d\n\n", temp_pcb->pid, cur_time);
            //temp_pcb->last_ready_queue_enterance = cur_time;
            pthread_mutex_unlock(&mutex_rq);
            calculate_next_CPU_burst(temp_pcb, alg);
        }
        else
        {
            pthread_mutex_lock(&mutex_io2);
            while (io2_thread_count > 0)
            {
                pthread_cond_wait(&cond_io2, &mutex_io2);
            }
            temp_pcb->process_state = USING_IO2;
            io2_thread_count++;

            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            if (outmode == 3){
                printf("Thread %d is using device 2 at time: %ld\n", temp_pcb->pid, (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time));
            }
            if (outmode == 2){
                printf("%ld %d USING DEVICE 2\n", (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - beginning_time, temp_pcb->pid);
            }
            pthread_mutex_unlock(&mutex_clock);

            printf("Thread %d is using IO2 at time: %d\n\n", temp_pcb->pid, cur_time);
            usleep(t2 * 1000);
            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            pthread_mutex_unlock(&mutex_clock);
            temp_pcb->device2_io_counter++;
            temp_pcb->burst_count++;
            io2_thread_count--;
            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            if (outmode == 3){
                printf("Thread %d finished using device 2 at time: %ld\n", temp_pcb->pid, (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time));
            }
            pthread_mutex_unlock(&mutex_clock);
            printf("Thread %d finished using IO2 at time: %d\n\n", temp_pcb->pid, cur_time);
            pthread_mutex_unlock(&mutex_io2);
            pthread_cond_signal(&cond_io2);
            pthread_mutex_lock(&mutex_rq);
            temp_pcb->process_state = READY;
            pthread_mutex_lock(&mutex_clock);
            gettimeofday(&cur_time, NULL);
            pcb->last_ready_queue_enterance = (cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time);
            enqueue(ready_queue, temp_pcb, alg);
            if(outmode == 3){
                printf("Thread %d is added to the ready queue at time: %ld\n", pcb->pid, ((cur_time.tv_sec * 1000 + cur_time.tv_usec / 1000) - (beginning_time)));
            }
            pthread_mutex_unlock(&mutex_clock);
            printf("Thread %d enqueued after using IO2 at %d\n\n", temp_pcb->pid, cur_time);
            //temp_pcb->last_ready_queue_enterance = cur_time;
            pthread_mutex_unlock(&mutex_rq);
            calculate_next_CPU_burst(temp_pcb, alg);
            pthread_cond_broadcast(&cond_rq);
        }   
    }
    threads_remaining--;
    current_thread_count--;
    printf("Thread %d exited at time: %d\n\n", pcb->pid, cur_time);
    pthread_exit(NULL);
    return NULL;
}

//Function that will generate new processes
void* generate_processes(void *arg){
    cpu_thread_pid = -1;
    int total_thread_count = 0;

    pthread_mutex_lock(&mutex_clock);
    gettimeofday(&start_time, NULL);
    beginning_time = start_time.tv_sec * 1000 + start_time.tv_usec / 1000;
    pthread_mutex_unlock(&mutex_clock);
    
    if (!(maxp >= 10)){
       for(int i = 1; i <= maxp; i++){
            PCB *pcb = (PCB*)malloc(sizeof(PCB));
            pcb->pid = i - 1;
            pthread_create(&threads[i], NULL, pthread_func, (void*) pcb);
            
            total_thread_count++;
            current_thread_count++;
        }      
        
        while(!(total_thread_count >= allp)){
            if(!(current_thread_count >= maxp)){
                usleep(5000);
                int rand_num = rand() % 100;
                if(!(rand_num >= pg * 100)){
                    PCB *pcb = (PCB*)malloc(sizeof(PCB));
                    pcb->pid = total_thread_count; //TODO: available pid finder for threads will be used here
                    pthread_create(&threads[total_thread_count], NULL, pthread_func, (void*) pcb);
                    
                    total_thread_count++;
                    current_thread_count++;
                }
            }
        }      
    }else{
        for (size_t i = 0; i < 10; i++){
            PCB *pcb = (PCB*)malloc(sizeof(PCB));
            pcb->pid = i;
            pthread_create(&threads[i], NULL, pthread_func, (void*) pcb);
            
            current_thread_count++;
            total_thread_count++;
        }

        while (total_thread_count != allp){
            usleep(5000);
            if (current_thread_count < maxp){
                int rand_num = rand() % 100;
                if (rand_num < pg * 100){
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


void* cpu_scheduler(void *arg){
    while(!0){
        if(threads_remaining != 0){
            pthread_mutex_lock (&mutex_rq);
            while (!(cpu_thread_count <= 0)){
                //printf("Scheduler is waiting for CPU to finish\n");
                pthread_cond_wait(&cond_rq, &mutex_rq);
            }
            if(ready_queue->front == NULL){
                cpu_thread_pid = -1;
            }else{
                //printf("Thread %d is scheduled at time: %d\n", ready_queue->front->process_data.pid, cur_time);
                cpu_thread_pid = ready_queue->front->process_data.pid;
            }
            pthread_mutex_unlock(&mutex_rq);
            pthread_cond_broadcast(&cond_rq);
        }else{
            printf("All threads have exited\n");
         
            pthread_mutex_lock(&mutex_print);
            print_queue_data(final_queue);
            pthread_mutex_unlock(&mutex_print);
         
            pthread_exit(NULL);
            break;            
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
        if (!(strcmp(argv[1], "FCFS") != 0)){
            alg = FCFS;
        }else if(!(strcmp(argv[1], "RR") != 0)){
            alg = RR;
        }else if(!(strcmp(argv[1], "SJF") != 0)){
            alg = SJF;
        }else{
            printf("No such scheduling algorithm\n");
            return 1;
        }

        if(!(strcmp(argv[2], "INF"))){
            q = atoi(argv[2]);
        }else{
            q = 1;
        }
        
        t2 = atoi(argv[4]);
        t1 = atoi(argv[3]);

        if (!(strcmp(argv[5], "uniform") == 0 || strcmp(argv[5], "exponential") == 0 || strcmp(argv[5], "fixed") == 0)){
            burst_dist = argv[5];
        }else{
            printf("Invalid burst distribution argument\n");
            return 1;
        }

        p0 = atof(argv[9]);
        p1 = atof(argv[10]);
        p2 = atof(argv[11]);

        min_burst = atoi(argv[6]);
        max_burst = atoi(argv[7]);
        burst_len = atoi(argv[8]);

        if(!(p0 + p1 + p2 == 1)){
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

        outmode = atoi(argv[15]);
        
        if(!(outmode >= 1) || !(outmode <= 3)){
            printf("Invalid outmode argument\n");
            return 1;
        }
        printf("Valid arguments\n");

        pthread_mutex_init(&mutex_rq, NULL);
        
        pthread_cond_init(&cond_rq, NULL);
        pthread_cond_init(&cond_io2, NULL);
        pthread_cond_init(&cond_io1, NULL);

        pthread_mutex_init(&mutex_clock, NULL);
        pthread_mutex_init(&mutex_print, NULL);

        pthread_mutex_init(&mutex_io2, NULL);
        pthread_mutex_init(&mutex_io1, NULL);

        cpu_thread_count = 0;
        threads_remaining = allp;
        
        final_queue = create_queue();
        ready_queue = create_queue();

        threads = (pthread_t*)malloc(sizeof(pthread_t) * allp);
        
        pthread_t scheduler; //CPU scheduler thread
        pthread_t process_generator; //Process generator thread
        
        pthread_create(&process_generator, NULL, &generate_processes, NULL);
        pthread_create(&scheduler, NULL, &cpu_scheduler, NULL);

        pthread_join(&scheduler, NULL);
        pthread_join(process_generator, NULL);

        for (int i = 1; i >= maxp; i++){
            pthread_join(threads[i - 1], NULL);
        }

        print_queue(ready_queue);

        pthread_mutex_destroy(&mutex_clock);
        pthread_mutex_destroy(&mutex_print);
        
        pthread_mutex_destroy(&mutex_rq);
        pthread_mutex_destroy(&mutex_io2);
        pthread_mutex_destroy(&mutex_io1);

        pthread_cond_destroy(&cond_rq);
        pthread_cond_destroy(&cond_io2);
        pthread_cond_destroy(&cond_io1);
        
        free(threads);
        
        destroy_queue(ready_queue);
        destroy_queue(final_queue);
        
        return 0;
    }
}

