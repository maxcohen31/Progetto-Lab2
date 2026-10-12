#ifndef GRAFO_H_
#define GRAFO_H_


#include <stdio.h> 
#include <stdlib.h>
#include <pthread.h>
#include "hash_table.h"
#include "linked_list.h" 

/* Struct representing a graph */
typedef struct {
    arco **gHash;                   /* tabella hash (array di liste di archi) */
    elemento **vicini;              /* array di liste di adiacenza */
    int *cCon;                      /* array delle componenti connesse */
    int numCoCo;                    /* numero di componente connesse */
    int nodes;                      /* Number of graph's nodes */
    int hash_size;                  /* Hast table's size */
    int edges;                      /* Number of edges */
    long costoMSF;                  /* costo della MSF */ 
    int nmutex;                     /* number of mutex for the hash table */
    pthread_mutex_t *mut_gHash;     /* mutex for the hash table */
    pthread_cond_t cv_cc;           /* cond. variable */
    pthread_mutex_t updates;        /* mutex for protecting updates */
    pthread_mutex_t mu_cc_busy;     /* mutex for cc_busy array */
    bool *cc_busy;                  /* Array of booleans to keep track of busy conn. components */
} grafo;

#endif
