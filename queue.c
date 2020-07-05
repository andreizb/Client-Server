#include <stdio.h> 
#include <stdlib.h>
#include "queue.h"
  
struct QNode* newNode(void *k) 
{ 
    struct QNode* temp = (struct QNode*)malloc(sizeof(struct QNode)); 
    temp->key = k; 
    temp->next = NULL; 
    return temp; 
} 
  
struct Queue* createQueue() 
{ 
    struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue)); 
    q->front = q->rear = NULL; 
    return q; 
} 
  
void enQueue(struct Queue* q, void *k) 
{ 
    struct QNode* temp = newNode(k); 
  
    if (q->rear == NULL) { 
        q->front = q->rear = temp; 
        return; 
    } 
  
    q->rear->next = temp; 
    q->rear = temp; 
} 
  
void deQueue(struct Queue* q) 
{ 
    if (q->front == NULL) 
        return; 
  
    struct QNode* temp = q->front; 
  
    q->front = q->front->next; 
  
    if (q->front == NULL) 
        q->rear = NULL; 

    free(temp); 
}