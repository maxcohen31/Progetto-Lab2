
typedef struct uf_t 
{
    int *parents;
    int *rank;
    int num_nodes;
} uf_t;

/* Initialises an union-find data structure */
uf_t *init(int n); 

/* Find operation (recursive) - finds the root of the component x by using path compression strategy */
int find_op(uf_t *union_find_t, int a);

/* Iterative version of find operation */
// int find_op(uf_t *union_find_t, int a);

/* Union operation - merges two subsets a and b */
void union_op(uf_t *union_find_t, int a, int b);

/* Frees the union-find ds */
void free_uf(uf_t *union_find_t);
