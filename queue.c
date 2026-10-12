#include "queue.h"
#include <stdlib.h>

queue_t *queue_init()
{
    queue_t *q = malloc(sizeof(*q));
    if (q == NULL)
    {
        fprintf(stderr, "Error: memory allocation for queue failed\n");
        exit(1);
    }
    q->h = NULL;
    q->t = NULL;
    q->size = 0;
    return q;
}

void enqueue(queue_t *q, int val)
{
    q_node *qn = malloc(sizeof(*qn));
    if (qn == NULL)
    {
        fprintf(stderr, "Error: memory alllocation for qn failed\n");
        exit(1);
    }
    qn->next = NULL;
    qn->val = val;
    if (q->h == NULL)
    {
        q->t = qn;
        q->h = qn; 
    }
    else 
    {
        q->t->next = qn;
        q->t = qn;
    }
    q->size++;
}

int is_empty(queue_t *q)
{
    if (q == NULL)
    {
        fprintf(stderr, "Error: NULL queue\n");
        exit(2);
    }
    return q->h == NULL;
}

int dequeue(queue_t *q)
{
    if (is_empty(q))
    {
        fprintf(stderr, "Error: cannot pop from an empty queue\n");
        exit(3);
    }
    q_node *n = q->h;
    if (q->h == q->t)
    {
        q->h = NULL;
        q->t = NULL;
    }
    else 
    {
        q->h = q->h->next;
    }
    q->size--;

    int res = n->val;
    free(n);
    return res;
}

void queue_destroy(queue_t *q)
{
    if (q == NULL) return;
    while (q->h != NULL)
    {
        q_node *n = q->h;
        q->h = q->h->next;
        free(n);
    }
    q->t = NULL;
    q->size = 0;
    free(q);
}
