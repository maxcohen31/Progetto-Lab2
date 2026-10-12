#include "hash_table.h"
#include "linked_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* Inserts a new node by manteining the list sorted */
elemento *insert_ord(elemento *l, elemento *new_el, int (*cmp)(elemento*, elemento*))
{
    if (l == NULL) return new_el;
    else if (cmp(l, new_el) > 0)
    {
        /* new->id is smaller than l->id: new becomes the new head */
        new_el->next = l;
        return new_el;
    }
    else 
    {
        elemento *tmp = l;
        /* Find the first element in the list that has id greater than new */
        while (tmp->next && cmp(tmp->next, new_el) < 0)
        {
            tmp = tmp->next;
        }
        if (tmp->next == NULL)
        {
            /* Insertion at the end of the list */
            tmp->next = new_el;
            new_el->next = NULL;
        }
        else
        {
            /* Insertion of the new node before tmp */
            new_el->next = tmp->next;
            tmp->next = new_el;
        }
        return l;
    }
}

/* Compares two id */
int compare_id(elemento *a, elemento *b)
{
    return a->id - b->id;
}

elemento **get_neighbors(hash_t *ht, int n)
{
    elemento **neigh = malloc(sizeof(elemento*)*(n+1));
    if (neigh == NULL)
    {
        fprintf(stderr, "Memory allocation for 'neigh' failed\n");
        exit(6);
    }
    /* neigh's memory set to NULL */
    memset(neigh, 0, sizeof(elemento*)*(n+1)); 

    /* Loop over the hash table and get its elements */
    for (int i = 0; i < ht->size; i++)
    {
        arco *a = ht->link_l[i];
        while (a != NULL) 
        {
            /* Nieghbors of a->u */
            elemento *e1 = malloc(sizeof(elemento));
            if (e1 == NULL)
            {
                fprintf(stderr, "Memory allocation for 'e1' failed\n");
                exit(9);
            }
            e1->id = a->v;
            e1->msf = a->msf;
            e1->w = a->weight;
            e1->next = NULL;
            neigh[a->u] = insert_ord(neigh[a->u], e1 , compare_id);

            /* Neighbors of a->v */
            elemento *e2= malloc(sizeof(elemento));
            if (e2 == NULL)
            {
                fprintf(stderr, "Memory allocation for 'e2' failed\n");
                exit(9);
            }
            e2->id = a->u;
            e2->msf = a->msf;
            e2->w = a->weight;
            e2->next = NULL;
            neigh[a->v] = insert_ord(neigh[a->v], e2 , compare_id);
        
            a = a->next;

        }
    }
    return neigh;
}

void remove_element(elemento **l, int i)
{
    elemento *current = *l;
    elemento *p = NULL;
    while (current != NULL)
    {
        if (current->id == i)
        {
            if (p == NULL) *l = current->next;
            else p->next = current->next;
            free(current);
            return;
        }
        p = current;
        current = current->next;
    }
}

/* Prints out neighbors of the node n */
void print_neighbors(elemento **l, int n)
{
    elemento *c = l[n];
    if (c == NULL)
    {
        printf("Isolated node\n");
        return;
    }
    printf("Neighbors of node %d:\n", n);
    while (c != NULL) 
    {
        printf("Node %d -> W: %d; msf: %d\n", c->id, c->w, c->msf);
        c = c->next;
    }
}

