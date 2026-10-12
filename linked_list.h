#ifndef LINKED_LIST_H_
#define LINKED_LIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct hash_t hash_t;

typedef struct elemento {
  int id;   // indice del nodo       
  int w;    // peso 
  bool msf; // true sse questo arco appartiene alla MSF
  struct elemento *next;
} elemento;

/* Insert 'new' as the head. Returns the head of the list */
elemento *insert_ord(elemento *l, elemento *new_el, int (*cmp)(elemento*, elemento*));

/* Returns the smallest id */
int compare_id(elemento *a, elemento *b);

/* Returns neighbors of node n */
elemento **get_neighbors(hash_t *ht, int n);

/* Remove an element from a list */
void remove_element(elemento **l, int i);

/* Given a node n, the function returns its neighbors */
void print_neighbors(elemento **l, int n);

#endif
