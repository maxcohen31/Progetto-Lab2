#ifndef _arco_h
#define _arco_h


#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/* Generic edge of the graph */
typedef struct arco
{
    int u;
    int v;
    int weight;
    bool msf;
    struct arco *next;
} arco;

#endif // !_arco_h
#define _arco_h
