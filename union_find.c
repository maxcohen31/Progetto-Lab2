#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "union_find.h"

/* Initialises the union-find data structure */
uf_t *init(int n)
{
    uf_t *uf = malloc(sizeof(uf_t)); 
    if (uf == NULL)
    {
        fprintf(stderr, "Error allocating memory for the union-find ds\n");
        return NULL;
    }
    uf->num_nodes = n;
    uf->parents = malloc(sizeof(int) * (n+1));
    uf->rank = malloc(sizeof(int) * (n+1));
    if (uf->parents == NULL || uf->rank == NULL)
    {
        fprintf(stderr, "Error allocating memory for parents or rank\n");
        free(uf->parents);
        free(uf->rank);
        return NULL;
    }

    /* Set to 0 the rank array */
    memset(uf->rank, 0, sizeof(int)*(n+1)); 

    /* Inits the parents array */
    for (int i = 0; i <= n; i++)
    {
        uf->parents[i] = i;
    }
   
    return uf;

}

/* Find operation */
int find_op(uf_t *union_find_t, int a)
{
    int root = a;
    while (union_find_t->parents[root] != root) root = union_find_t->parents[root];
    // Path compression
    while (union_find_t->parents[root] != root)
    {
        int next = union_find_t->parents[a];
        union_find_t->parents[a] = root;
        a = next;
    }
    return root;
}

/* Union operation */
void union_op(uf_t *union_find_t, int a, int b)
{
    int root_a = find_op(union_find_t, a);
    int root_b = find_op(union_find_t, b);

    if (root_a != root_b) /* root_a and root_b don't belong to the same set */
    {
        /* Attach root_a under root_b */
        if (union_find_t->rank[root_a] < union_find_t->rank[root_b])
        {
            union_find_t->parents[root_a] = root_b;
        }
        /* Attach root_b under root_a */
        else if (union_find_t->rank[root_a] > union_find_t->rank[root_b])
        {
            union_find_t->parents[root_b] = root_a;
        }
        else 
        {
            union_find_t->parents[root_b] = root_a;
            union_find_t->rank[root_a]++;
        }
    }
}

/* Free the memory */
void free_uf(uf_t *union_find_t)
{
    if (union_find_t == NULL)
    {
        fprintf(stderr, "uf_t is NULL: nothing to free\n");
        exit(1);
    }
    free(union_find_t->parents);
    free(union_find_t->rank);
    free(union_find_t);
}

