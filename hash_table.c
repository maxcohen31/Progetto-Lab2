#include <complex.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "hash_table.h"
#include "grafo.h"

hash_t *hash_t_init(int size)
{
    hash_t *ht = malloc(sizeof(*ht));
    if (!ht) return NULL;

    ht->size = size;
    /* Allocates memory four our linked lists */
    ht->link_l = malloc(sizeof(arco*) * size);
    if (!ht->link_l)
    {
        free(ht); /* Frees the already allocated hash table */
        return NULL;
    }

    /* set each entries to NULL */
    for (int i = 0; i < size; i++) ht->link_l[i] = NULL;

    return ht;
}


unsigned int hash(int u, int v, int hs)
{
    return ((37 * (unsigned int)u) + (unsigned int)v) % hs;
}

void insert_gHash(arco **gHash, int ht_size, arco *a)
{
    /* Finds a spot to place the arc a */
    unsigned int bucket = hash(a->u, a->v, ht_size);     
    a->next = NULL; 
    arco *spot = gHash[bucket];

    /* Hash table is empty */
    if (spot == NULL)
    {
        gHash[bucket] = a;
        return;
    }
    
    /* find a free spot */
    arco *prev = NULL;
    while (spot != NULL)
    {
        if ((a->u == spot->u) && (a->v == spot->v))
        {
            /* Change the weight */
            spot->weight = a->weight;
            free(a);
            return;
        }
        
        prev = spot;
        spot = prev->next;
    }
    /* Arc inserted at the end of the list */ 
    prev->next = a; 
}

void insert(hash_t *ht, arco *a)
{
    insert_gHash(ht->link_l, ht->size, a);
} 

arco *find_gHash(arco **arco_array, int ht_size, int u, int v)
{
    unsigned int b = hash(u, v, ht_size);
    arco *found = arco_array[b];
    while (found != NULL) 
    {
        if ((found->u == u) && (found->v == v)) return found; 
        found = found->next;
    }
    return NULL;
}

arco *find(hash_t *ht, arco *a)
{
    if (ht == NULL) return NULL; /* Empty hash table */
    unsigned int spot = hash(a->u, a->v, ht->size);

    arco *match = ht->link_l[spot];

    while (match != NULL)
    {
        if ((match->u == a->u) && (match->v == a->v)) return match;
        match = match->next;
    }
    return match;
}

void free_ht(hash_t *ht)
{
    if (ht == NULL) return;
    free(ht);
}

arco *find_lock(grafo *g, int u, int v)
{
    int bucket = hash(u, v, g->hash_size);
    int mu_idx = bucket % g->nmutex;
     
    pthread_mutex_lock(&g->mut_gHash[mu_idx]);
    arco *found = g->gHash[bucket];
    while (found != NULL) 
    {
        if (found->u == u && found->v == v) break;
        found = found->next;
    }
    pthread_mutex_unlock(&g->mut_gHash[mu_idx]);
    return found;
}

void insert_lock(grafo *g, arco *a)
{
    int bucket = hash(a->u, a->v, g->hash_size);
    int mu_idx = bucket % g->nmutex;
    
    pthread_mutex_lock(&g->mut_gHash[mu_idx]); 
    a->next = NULL;
    if (g->gHash[bucket] == NULL) g->gHash[bucket] = a;
    else 
    {
        arco *curr = g->gHash[bucket];
        while (curr->next != NULL) {
            curr = curr->next;
    }
        curr->next = a;
    }
    pthread_mutex_unlock(&g->mut_gHash[mu_idx]); 
}

void remove_lock(grafo *g, int u, int v)
{
    int bucket = hash(u, v, g->hash_size);
    int mu_idx = bucket % g->nmutex;
     
    pthread_mutex_lock(&g->mut_gHash[mu_idx]);
    arco *curr = g->gHash[bucket];
    arco *prev = NULL;
    while (curr != NULL) 
    {
        if (curr->u == u && curr->v == v) 
        {
            if (prev == NULL) g->gHash[bucket] = curr->next;
            else prev->next = curr->next; 
            free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    } 
    pthread_mutex_unlock(&g->mut_gHash[mu_idx]);
}

/* ========= Utility ========= */
void print_hash_table(hash_t *ht)
{
    for (int i = 0; i < ht->size; i++)
    {
        arco *a = ht->link_l[i]; 
        printf("Slot[%2d]: \n", i);

        while (a != NULL)
        {
            printf("(%d, %d, %d)\n", a->u, a->v, a->weight);
            a = a->next;
        }
    }
    printf("\n");
}

int count_edges(hash_t *ht)
{
    int num_edges = 0;
    for (int i = 0; i < ht->size; i++)
    {
        arco *a = ht->link_l[i];
        while (a != NULL)
        {
            num_edges++;
            a = a->next;
        }
    }
    return num_edges;
}

/* Returns an array of arco struct and the number of edges found */
arco **edges_array(hash_t *ht, int num_edges, int *count)
{
    arco **edges = malloc(sizeof(arco*)*num_edges);
    if (edges == NULL)
    {
        fprintf(stderr, "Error allocating memory for edges_array\n");
        return NULL;
    }

    int arr_idx = 0;
    for (int i = 0; i < ht->size; i++)
    {
        arco *a = ht->link_l[i];
        while (a != NULL)
        {
            edges[arr_idx++] = a;
            a = a->next;
        }
    }
    *count = arr_idx; /* Edges found */
    return edges;
}
