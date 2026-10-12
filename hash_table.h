#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

#include <stdio.h>
#include "arco.h"
#include "grafo.h"

typedef struct hash_t
{
    arco **link_l;
    int size;
} hash_t;

/* Hash table initialization */
hash_t *hash_t_init(int size);

/* Hash function */
unsigned int hash(int u, int v, int hs);

/*Inserts an edge into gHash */
void insert_gHash(arco **gHash, int h_size, arco *a);

/* Inserts an edge into our hash table */
void insert(hash_t *ht, arco *a);

/* Find the given arco in g->gHash */
arco *find_gHash(arco **arco_array, int ht_size, int u, int v);

/* finds an edge */
arco *find(hash_t *ht, arco *a);

/* Same as find_gHash, insert_gHash and remove_gHash but used in a mutithreading context */
arco *find_lock(grafo *g, int u, int v);
void insert_lock(grafo *g, arco *a);
void remove_lock(grafo *g, int u, int v);

/* Frees the hash table's memory */
void free_ht(hash_t *ht);

/* Utility - prints to stdout the hash table */
void print_hash_table(hash_t *ht);

/* Utility - Returns the number of edges */
int count_edges(hash_t *ht);

/* Returns an array of pointers of arco structs */
arco **edges_array(hash_t *ht, int num_edges, int *count);

#endif
