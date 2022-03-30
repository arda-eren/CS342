#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "scheduler_defs.h"


//NODE DEFINITIONS AND METHODS

//Definition for the structure of a node in a queue
//build like a doubly linked list
typedef struct node{
    PCB process_data;
    struct node* next;
    struct node* prev;
}node;

//Method: create a new node
//Parameter: PCB
//Return: node pointer
node* create_node(PCB process_data){
    node* node = (struct node*) malloc(sizeof(struct node));
    node->next = NULL;
    node->prev = NULL;
    node->process_data = process_data;
    return node;
}

//Method: print a node to console
//Parameters: node pointer
//Return: void
void print_node(node* node){
    printf("%d", node->process_data);
}


//QUEUE DEFINITIONS AND METHODS

//Definition to hold dront and the back of the queue
//build like a doubly linked list
typedef struct queue{
    struct node* front;
    struct node* back;
}queue;

//Method: check if queue is empty
//Parameters: queue pointer
//Return: boolean
bool isEmpty(struct queue* queue){
    return queue->front == NULL;
}

//Method: create a new queue
//Parameters: -
//Return: queue pointer
queue* create_queue(){
    queue* queue = (struct queue*) malloc(sizeof(struct queue));
    queue->front = NULL;
    queue->back = NULL;
}

//Method: create a new queue
//Parameters: node pointer
//Return: queue pointer
queue* create_queue(struct node* front_node){
    queue* queue = (struct queue*) malloc(sizeof(struct queue));
    queue->front = front_node;
    queue->back = NULL;
}

//Method: create a new queue
//Parameters: node pointer, node pointer
//Return: queue pointer
queue* create_queue(node* front_node, node* back_node){
    queue* queue = (struct queue*) malloc(sizeof(struct queue));
    queue->front = front_node;
    queue->back = back_node;
}

//Method: enqueue the PCBs according to the sjf scheduling algorithm
//Parameters: queue pointer, PCB pointer
//Return: void
void sjf_enqueue(struct queue* queue, struct PCB* process_data){
        struct node* node = (struct node *)malloc(sizeof(struct node));
        node->process_data = *process_data;
        node->next = NULL;
        node->prev = NULL;

        if (isEmpty(queue)){
            queue->front = node;
            queue->back = node;
        }else{
            struct node* holder = queue->front;
            while (holder && holder->process_data.next_CPU_burst_length < process_data->next_CPU_burst_length){
                holder = holder->next;
            }
           
            if (holder == queue->front){
                node->next = queue->front;
                queue->front->prev = node;
                queue->front = node;
            }else if (holder == NULL){
                queue->back->next = node;
                node->prev = queue->back;
                queue->back = node;
            }else{
                node->next = holder;
                holder->prev->next = node;
                node->prev = holder->prev;
                holder->prev = node;
            }
        }
}

//Method: enqueue the PCBs according to the rr or fcfs scheduling algorithms
//Parameters: queue pointer, PCB pointer
//Return: void
void fifo_enqueue(struct queue* queue, struct PCB* process_data){
        struct node* node = (struct node *)malloc(sizeof(struct node));
        node->process_data = *process_data;
        node->next = NULL;
        node->prev = NULL;
        
        if (isEmpty(queue)){
            queue->front = node;
            queue->back = node;
        }else{
            struct node* holder = queue->back;
            holder->next = node;
            node->prev = queue->back;
            holder = node;
        }
}

//Method: enqueue according to the selected algorithm
//Parameters: queue pointer, PCB pointer, scheduling_algorithm
//Return: void
void enqueue(struct queue* queue, struct PCB* process_data, enum scheduling_algorithm algorithm){
    if(algorithm == SJF){
        sjf_enqueue(queue, process_data);
    }else{
        fifo_enqueue(queue, process_data);
    }
}

//Method: dequeue a node
//Parameters: node pointer
//Return: node pointer
node* dequeue(struct queue* queue){
    if(isEmpty(queue)){
        printf("Queue is empty\n");
        return NULL;
    }else{
        struct node *holder = queue->front;
        queue->front = queue->front->next;
        return holder;
    }
}

//Method: freeing the node memory
//Parameters: node pointer
//Return: void
void destroy_node(struct node* node){
    free(node);
}

//Method: freeing the queue memory
//Parameters: queue pointer
//Return: void
void destroy_queue(struct queue* queue){
    while(!isEmpty(queue)){
        destroy_node(dequeue(queue));
    }
    free(queue);
}

//Method: print a queue
//Parameters: queue pointer
//Return: void
void print_queue(queue* queue){
    if(!isEmpty(queue)){
        node* node_ptr = queue->front;
        while(node_ptr->next != NULL){
            print_node(node_ptr);
            node_ptr = node_ptr->next;
        }
    }
}






