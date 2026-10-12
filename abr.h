#ifndef ABR_H_
#define ABR_H_

#include <stdio.h>
#include <stdlib.h>

typedef struct node 
{
    int val_node;
    struct node *left;
    struct node *right;
} node;

/* Inserts a node into the abr */
node *abr_insert(node *root, int val);
node *abr_insert_ex(node *root, int val, int *inserted);
/* Searches a node within the abr */
int abr_search(node *root, int val);

/* Free the abr's memory */
void free_abr(node *root);

/* Get rid of regolarities hidden into the data */
int shuffle(int n);

int unshuffle(int n);

#endif 

