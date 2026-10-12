#ifndef __queue_h_
#define __queue_h_

#include <stdio.h>
#include <stdlib.h>

typedef struct q_node
{
    int val;
    struct q_node *next;
} q_node;

typedef struct queue_t
{
    q_node *h;
    q_node *t;
    int size;
} queue_t;

/* Initializes a queue */
queue_t *queue_init();

int is_empty(queue_t *q);

/* Inserts a node into the queue */
void enqueue(queue_t *q, int val);

/* Removes a node from the queue */
int dequeue(queue_t *q);

/* Free all the memory detained by the queue */
void queue_destroy(queue_t *q);

#endif
