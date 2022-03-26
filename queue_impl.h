#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "scheduler_definitions.h"

//NODE DEFINITIONS AND METHODS

//Definition for the structure of a node in a queue
//build like a doubly linked list
typedef struct node{
    PCB process_data;
    struct node* next;
    struct node* prev;
}node;

//Method to create a new node
//requires a pcb parameter as data for the node
//returns a node pointer
node* create_node(PCB process_data){
    node* node = (struct node*) malloc(sizeof(struct node));
    node->next = NULL;
    node->prev = NULL;
    node->process_data = process_data;
    return node;
}

//Method to print a node
//requires a node pointer parameter
//returns void
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

//Method to check if the queue is empty or not
//requires a queue pointer parameter
//returns boolean
bool isEmpty(struct queue* queue){
    return queue->front == NULL;
}

//Method to create a new queue
//no parameter required
//returns a queue pointer
queue* create_queue(){
    queue* queue = (struct queue*) malloc(sizeof(struct queue));
    queue->front = NULL;
    queue->back = NULL;
}

//Method to create a new queue
//a node pointer parameter as the front node is required
//returns a queue pointer
queue* create_queue(node* front_node){
    queue* queue = (struct queue*) malloc(sizeof(struct queue));
    queue->front = front_node;
    queue->back = NULL;
}

//Method to create a new queue
//two node pointer parameters as the front and the back node is required
//returns a queue pointer
queue* create_queue(node* front_node, node* back_node){
    queue* queue = (struct queue*) malloc(sizeof(struct queue));
    queue->front = front_node;
    queue->back = back_node;
}



// TODO: Implement the enqueue method according to the scheduling algorithm parameter.
// TODO: Implement the dequeue method
// TODO: Implement the destroy node method
// TODO: Implement the destroy queue method
// FIXME: Fix the comments as Method:, Parameters:, Return:  



//Method to print a queue
//requires a queue pointer parameter
//returns void
void print_queue(queue* queue){
    if(!isEmpty(queue)){
        node* node_ptr = queue->front;
        while(node_ptr->next != NULL){
            print_node(node_ptr);
            node_ptr = node_ptr->next;
        }
    }
}






