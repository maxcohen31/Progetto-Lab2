#define _GNU_SOURCE
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "union_find.h"
#include "hash_table.h"
#include "grafo.h"
#include "abr.h"
#include "queue.h"

#define QUI __LINE__, __FILE__ 
#define MIN_THREAD 3
#define DEFAULT_HASHSIZE 100000
#define DEFAULT_MUTEX 1000
#define BUFF_SIZE 1024

typedef enum valid_insertion 
{
    ILLEGAL = -1,
    NOT_IN_MSF = 0,
    IN_MSF
} insertion;

typedef enum valid_deletion
{
    NO_EXISTENCE = -1,
    DELETED = 0,
    NOT_DELETED
} deletion;

/* ########## Structs ########## */

/* struct representing a single operation */
typedef struct 
{
    char op;
    int u;
    int v;
    int w;
} thread_op;

typedef struct 
{
    thread_op buff[BUFF_SIZE];
    int h;
    int t;
    int counter;
    int size;
    pthread_mutex_t mu;
    pthread_cond_t empty;
    pthread_cond_t full;
} buf_t;

/* Struct shared amongst threads */
typedef struct 
{
    grafo *g;
    buf_t *buf;
} thread_arg;

/* ######## Prototypes ######## */

bool is_prime(int n);
int make_prime(int hash_size);
void lock_bucket(grafo *g, int u, int v);
void unlock_bucket(grafo *g, int u, int v);
void lock_cc(grafo *g, int u, int v, int *lock_a, int *lock_b);
void unlock_cc(grafo *g, int u, int v);
void buffer_init(buf_t *buff);
void buffer_destroy(buf_t *buff);
void buff_put(buf_t *buf, thread_op op);
thread_op buff_get(buf_t *buf);
void *consumer_thread(void *arg);
int compare_edges_weight(const void *a, const void *b);
uf_t *kruskal(hash_t *ht, int num_nodes, int num_edges);
long msf_cost(hash_t *ht);
int *connectend_components(uf_t *uf, int n);
int count_cc(uf_t *uf, int n);
grafo *build_graph(hash_t *ht, int nodes, int mux);
void print_graph(grafo *g);
void free_graph(grafo *gr);
void stats(grafo *g);
void set_msf_neighbors(elemento *l, int idx, bool new_val);
bool find_min(node *abr, node* root_small, grafo*g, int *min_u, int *min_v, int *min_w, bool *found);
int aggiungi_arco(grafo *g, int u, int v, int w);
int cancella_arco(grafo *g, int u, int v);

/* ############################ */

/* ########## Main ########## */
int main(int argc, char **argv)
{
    int c;
    int thread_num = MIN_THREAD;
    int h_size = DEFAULT_HASHSIZE;
    int nmutex = DEFAULT_MUTEX;

    while ((c = getopt(argc, argv, "t:H:M:")) != -1)
    {
        switch (c) 
        {
            case 't':
                thread_num = atoi(optarg);
                break;
            case 'H':
                h_size = make_prime(atoi(optarg));
                break;
            case 'M':
                nmutex = atoi(optarg);
                break;  
            case '?':
                fprintf(stderr, "Argument not recognized\n");
                break;
            default:
                break; 
        }
    }
    /* Check if there are more than two arguments */
    if (argc - optind < 2)
    {
        fprintf(stderr, "Usage:\t%s file_grafo file_archi -t [n_threads] -H [hashsize] -M [nmutex]\nUsage: (optional arguments): msf.out file_grafo file_archi\n", argv[0]);
        exit(1);
    }
    FILE *f = fopen(argv[optind], "r");
    if (f == NULL)
    {
        fprintf(stderr, "Error[Line: %d File: %s] opening file\n", QUI);
        exit(2);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    int num_node = 0;
    int num_edge = 0;
    hash_t *ht = NULL;

    while ((read = getline(&line, &len, f)) != -1)
    {
        /* Get the number of nodes and arcs */
        if (line[0] == 'p')
        {

            int e = sscanf(line, "p sp %d %d", &num_node, &num_edge);
            if (e != 2) 
            {
                fprintf(stderr, "Error [Line: %d - File: %s]: line format not valid!\n", QUI);
                exit(3);
            } 
            /* Initializes the hast table */
            ht = hash_t_init(h_size);
        } 
        if (line[0] == 'a')
        {
            int u;
            int v;
            int w;
            int e = sscanf(line, "a %d %d %d", &u, &v, &w);
            if (e != 3) 
            {
                fprintf(stderr, "Error[Line: %d - File: %s]: line format not valid\n", QUI);
                exit(4);
            }
            /* Building the hash table */
            arco *arc = malloc(sizeof(arco));
            if (arc == NULL)
            {
                fprintf(stderr, "Error[Line: %d - File: %s]: memory allocation for arc failed!\n", QUI);
                exit(5);
            }
            arc->u = u;
            arc->v = v;
            arc->weight = w;
            arc->msf = false;
            insert(ht, arc);
        }
    }

    fclose(f);

    grafo *g = build_graph(ht, num_node, nmutex);
    free_ht(ht);

    /* ########## Multithreading section ########## */
    pthread_t *th_arr = malloc(sizeof(*th_arr) * thread_num);
    thread_arg *arg = malloc(sizeof(*arg) * thread_num);
    if (!th_arr || !arg)
    {
        fprintf(stderr, "Error[Line: %d - File: %s]: allocation for th_arr or arg arrays failed!\n", QUI);
        exit(6);
    }
    buf_t shared_buf;
    buffer_init(&shared_buf);
    for (int i = 0; i < thread_num; i++)
    {
        arg[i].g = g;
        arg[i].buf = &shared_buf;
    }
    
    for (int i = 0; i < thread_num; i++) 
    {
        if (pthread_create(&th_arr[i], NULL, consumer_thread, &arg[i]) != 0)
        {
            fprintf(stderr, "Error[Line: %d - File: %s]: Creation of threads failed\n", QUI);                
            exit(7);
        }
    }
    
    FILE *file_op = fopen(argv[optind + 1], "r");
    if (file_op == NULL)
    {
        fprintf(stderr, "Error[Line: %d - File: %s]: failed to open file_op\n", QUI);
        exit(8);
    }

    while ((read = getline(&line, &len, file_op)) != -1)
    {
        thread_op op;
        if (line[0] == '+')
        {
            int u;
            int v;
            int w;
            int e = sscanf(line, " + %d %d %d", &u, &v, &w);
            if (e != 3) continue;
            op.op = '+';
            op.u = u;
            op.v = v;
            op.w = w;
            buff_put(&shared_buf, op);
        }
        else if (line[0] == '-')
        {
            int u;
            int v;
            int e = sscanf(line, "- %d %d", &u, &v);
            if (e != 2) continue;
            op.op = '-';
            op.u = u;
            op.v = v;
            op.w = 0;
            buff_put(&shared_buf, op);
        }
    }

    fclose(file_op);
    free(line);

    for (int i = 0; i < thread_num; i++)
    {
        thread_op quit;
        quit.u = 0;
        quit.v = 0;
        quit.w = 0;
        quit.op = 'E';
        buff_put(&shared_buf, quit);
    }

    for (int i = 0; i < thread_num; i++)
    {
        if (pthread_join(th_arr[i], NULL) != 0)
        {
            fprintf(stderr, "Error[Line: %d - File: %s]: Thread join failed!\n", QUI);
            exit(9);
        }

    }

    printf("Operazioni terminate\n");
    stats(g);
    buffer_destroy(&shared_buf);
    free(th_arr);
    free(arg);
    free_graph(g);
    return 0;
}
/* ########## Main's end ########## */

bool is_prime(int n)
{
    if (n < 2) return false;
    if (n % 2 == 0) return n == 2;
    for (int i = 3; i <= n/i; i++)
    {
        if (n % i == 0) return false;
    }
    return true;
}

int make_prime(int hash_size)
{
    if (is_prime(hash_size)) return hash_size;
    int p = hash_size;
    while (!(is_prime(p))) p--;
    return p;
}

/* Locks the connected components for u and v. Returns the ids of the locked components */
void lock_cc(grafo *g, int u, int v, int *lock_a, int *lock_b)
{
    pthread_mutex_lock(&g->mu_cc_busy);
    /* Waits on cv_cc until the components of either u or v is already taken */
    while (g->cc_busy[g->cCon[u]] || g->cc_busy[g->cCon[v]]) pthread_cond_wait(&g->cv_cc, &g->mu_cc_busy); 
    int a = g->cCon[u];
    int b = g->cCon[v];
    /* Get the first component */
    g->cc_busy[a] = true;
    if (a != b) g->cc_busy[b] = true;  
    *lock_a = a;
    *lock_b = b;
    pthread_mutex_unlock(&g->mu_cc_busy);
}

/* Unlocks the connected components */
void unlock_cc(grafo *g, int lock_a, int lock_b)
{
    pthread_mutex_lock(&g->mu_cc_busy);    
    g->cc_busy[lock_a] = false;
    if (lock_a != lock_b) g->cc_busy[lock_b] = false; 
    pthread_cond_broadcast(&g->cv_cc);
    pthread_mutex_unlock(&g->mu_cc_busy);
}
/* Initializes the buffer shared amongst threads */
void buffer_init(buf_t *buff)
{
    buff->counter = 0;
    buff->h = 0;
    buff->t = 0;
    pthread_cond_init(&buff->full, NULL);
    pthread_cond_init(&buff->empty, NULL);
    pthread_mutex_init(&buff->mu, NULL);
}
/* Destroy the buffer */
void buffer_destroy(buf_t *buff)
{
    pthread_cond_destroy(&buff->empty);
    pthread_cond_destroy(&buff->full);
    pthread_mutex_destroy(&buff->mu);
}
/* Inserts into the shared buffer the operation to perform */
void buff_put(buf_t *buf, thread_op op)
{
    pthread_mutex_lock(&buf->mu);
    while (buf->counter == BUFF_SIZE) pthread_cond_wait(&buf->full, &buf->mu);
    buf->buff[buf->t] = op; 
    buf->t = (buf->t + 1) % BUFF_SIZE;
    buf->counter++;
    pthread_cond_signal(&buf->empty);
    pthread_mutex_unlock(&buf->mu);
}
/* Returns the operation from the buffer */
thread_op buff_get(buf_t *buf)
{
    pthread_mutex_lock(&buf->mu);
    while (buf->counter == 0) pthread_cond_wait(&buf->empty, &buf->mu);
    thread_op op = buf->buff[buf->h];
    buf->h = (buf->h + 1) % BUFF_SIZE;
    buf->counter--;
    pthread_cond_signal(&buf->full);
    pthread_mutex_unlock(&buf->mu);
    return op;
}
/* Consumer thread - It calls aggiungi_arco or cancella_arco depending on the operation got from the buffer */
void *consumer_thread(void *arg)
{
    thread_arg *a =  (thread_arg*)arg;
    grafo *g = a->g;
    /* Result of either aggiungi_arco or cancella_arco */
    int r; 

    while (true)
    {
        /* Getting the operation to perform */
        thread_op o = buff_get(a->buf);        
        /* Execution stopped */
        if (o.op == 'E') break;

        int first_comp;
        int snd_comp; 
        if (o.u > g->nodes || o.v > g->nodes)
        {
            if (o.op == '+') printf("%c %d %d %d 0\n", o.op, o.u, o.v, o.w);
            else printf("%c %d %d 0\n", o.op, o.u, o.v);
            continue;
        }
        /* Locks the components */
        lock_cc(g, o.u, o.v, &first_comp, &snd_comp);
        if (o.op == '+') r = aggiungi_arco(g, o.u, o.v, o.w);
        else if (o.op == '-') r = cancella_arco(g, o.u, o.v);
        /* Operation unknown */
        else 
        {
            unlock_cc(g, first_comp, snd_comp);
            continue; 
        }
        unlock_cc(g, first_comp, snd_comp);

        if (r >= 0) /* Valid operation */
        {
            if (o.op == '+') printf("%c %d %d %d %d %d %ld\n", o.op, o.u, o.v, o.w, g->edges, g->numCoCo, g->costoMSF);
            else printf("%c %d %d %d %d %ld\n", o.op, o.u, o.v, g->edges, g->numCoCo, g->costoMSF);
        }
        else /* Operation not valid */
        {
            if (o.op == '+') printf("%c %d %d %d 0\n", o.op, o.u, o.v, o.w);
            else printf("%c %d %d 0\n", o.op, o.u, o.v);
        } 
    }
    return NULL;
}

/* #################################### */

/* Utility function for qsort */
int compare_edges_weight(const void *a, const void *b)
{
    if ((*(arco**)a)->weight < (*(arco**)b)->weight) return -1;
    if ((*(arco**)a)->weight > (*(arco**)b)->weight) return 1;
    return 0;
}

/* ########## Kruskal's algorithm ########## */
uf_t *kruskal(hash_t *ht, int num_nodes, int num_edges)
{
    int edge_count;
    /* Gets the array of pointers to edges and the total amount */
    arco **e_arr = edges_array(ht, num_edges, &edge_count);
    if (e_arr == NULL)
    {
        fprintf(stderr, "Error[Line: %d - File: %s]: Memory allocation for e_arr failed\n", QUI);
        exit(1);
    }
    /* Sort our array of edges */
    qsort(e_arr, edge_count, sizeof(arco*), compare_edges_weight);
    
    uf_t *uf = init(num_nodes);
    if (uf == NULL)
    {
        fprintf(stderr, "Error[Line: %d - File: %s]: Memory allocation for Union-Find failed\n", QUI);
        free(e_arr);
        exit(2);
    }

    for (int i = 0; i < edge_count; i++)
    {
        arco *a = e_arr[i];
        /* Find the roots of u and v */
        int r_u = find_op(uf, a->u);
        int r_v = find_op(uf, a->v);
        if (r_u != r_v)
        {
            /* Join the two components */
            union_op(uf, r_u, r_v);
            /* The edge belongs to the MSF */
            a->msf = true;
        }
    } 
    free(e_arr);
    return uf;
}

/* Returns the cost of the MSF */
long msf_cost(hash_t *ht)
{
    long c = 0;
    for (int i = 0; i < ht->size; i++)
    {
        arco *a = ht->link_l[i];
        while (a != NULL)
        {
            if (a->msf) c += a->weight;
            a = a->next;
        }
    }
    return c;
}

/* Builds the connected components array */
int *connectend_components(uf_t *uf, int n)
{
    /* Array for storing connected components */
    int *conn_comp = malloc(sizeof(int) * (n+1));
    /* Array for storing minimal roots */
    int *roots = malloc(sizeof(int) * (n+1));
    if (conn_comp == NULL || roots == NULL)
    {
        fprintf(stderr, "Error[Line: %d - File: %s]Memory allocation failed for conn_comp or roots\n", QUI);
        exit(1);
    }

    for (int i = 0; i <= n; i++) roots[i] = INT_MAX; 
    for (int i = 0; i <= n; i++)
    {
        int r = find_op(uf, i);
        if (i < roots[r]) roots[r] = i;
    }

    for (int i = 0; i <= n; i++)
    {
        conn_comp[i] = roots[find_op(uf, i)];
    }
    free(roots);
    return conn_comp;
}

/* Counts connected components */
int count_cc(uf_t *uf, int n)
{
    int cc_count = 0;
    int *r = malloc(sizeof(int)*(n+1));
    if (r == NULL) 
    {
        fprintf(stderr, "Error[Line: %d - File: %s]: Memory allocation for r failed!\n", QUI);
        exit(1);
    }
    /* Set to -1 the  */
    for (int i = 0; i <= n; i++) r[i] = -1;
    for (int i = 0; i <= n; i++)
    {
        int val = find_op(uf, i);
        if (r[val] == -1)
        {
            r[val] = val;
            cc_count++;
        }
    }
    free(r);
    return cc_count;
}

/* Builds an undirected graph */
grafo *build_graph(hash_t *ht, int nodes, int mux)
{
    grafo *gr = malloc(sizeof(*gr));
    if (gr == NULL)
    {
        fprintf(stderr, "Error[Line: %d - File: %s]: Memory allocation for graph failed!\n", QUI);
        exit(10);
    }
   
    int tot_edges = count_edges(ht);
    uf_t *uf = kruskal(ht, nodes, tot_edges);
    gr->costoMSF = msf_cost(ht);
    gr->edges = tot_edges;
    gr->vicini = get_neighbors(ht, nodes);
    gr->cCon = connectend_components(uf, nodes);
    gr->numCoCo = count_cc(uf, nodes);
    gr->gHash = ht->link_l;
    gr->nodes = nodes;
    gr->hash_size = ht->size;
    gr->nmutex = mux;

    gr->mut_gHash = malloc(sizeof(pthread_mutex_t) * mux);
    gr->cc_busy = malloc(sizeof(bool) * (nodes + 1)); 
    if (!gr->mut_gHash || !gr->cc_busy)
    {
        fprintf(stderr, "Error[Line: %d - File: %s]: memory allocation failed for mut_gHash or cc_busy\n", QUI);
        exit(10);
    }
    for (int i = 0; i < mux; i++) pthread_mutex_init(&gr->mut_gHash[i], NULL);
    for (int i = 0; i <= nodes; i++) gr->cc_busy[i] = false;
    pthread_cond_init(&gr->cv_cc, NULL);
    pthread_mutex_init(&gr->mu_cc_busy, NULL);
    pthread_mutex_init(&gr->updates, NULL);

    free_uf(uf);
    return gr;
}

void print_graph(grafo *g)
{
    printf("###########################\n");
    printf("----- EDGES -----\n");
    for (int i = 0; i < g->hash_size; i++)
    {
        arco *e = g->gHash[i];
        while (e != NULL)
        {
            printf("(%d, %d, %d, msf=%d)\n", e->u, e->v, e->weight, e->msf);
            e = e->next;
        }
    }
    printf("----- NEIGHBORS -----\n");
    for (int i = 0; i <= g->nodes; i++)
    {
        elemento *el = g->vicini[i];
        while (el != NULL)
        {
            printf("[%d] -> (w=%d, msf=%d)\n", el->id, el->w, el->msf);
            el = el->next;
        }
    }
    printf("---- CONNECTED COMPONENT -----\n");
    for (int i = 0; i < g->nodes; i++) printf("Conn. Comp [%d] -> %d\n", i, g->cCon[i]);
    printf("###########################\n");
}

/* Free graph's memory */
void free_graph(grafo *gr)
{
    if (gr == NULL) return;
    for (int i = 0; i < gr->nmutex; i++) pthread_mutex_destroy(&gr->mut_gHash[i]);

    pthread_mutex_destroy(&gr->mu_cc_busy);
    pthread_cond_destroy(&gr->cv_cc);
    pthread_mutex_destroy(&gr->updates);
    free(gr->mut_gHash);

    /* Freeing gHash */
    for (int i = 0; i < gr->hash_size; i++)
    {
        arco *a = gr->gHash[i];
        while (a != NULL)
        {
            arco *nxt = a->next;
            free(a);
            a = nxt;
        }
    }
    free(gr->gHash);
    
    /* Freeing neighbors */
    for (int i = 0; i <= gr->nodes; i++)
    {
        elemento *e = gr->vicini[i];
        while (e != NULL)
        {
            elemento *nxt = e->next;
            free(e);
            e = nxt;
        }
    }
    free(gr->vicini);
    free(gr->cc_busy);
    free(gr->cCon);
    free(gr);
   
}

void stats(grafo *g)
{
    int occupied_pos = 0;
    int max_len = 0;
    int edges = 0;
    int num_cc = 0;
    long msf_cst = 0;
    float avg_len = 0;

    for (int i = 0; i < g->hash_size; i++)
    {
        int l = 0; 
        arco *a = g->gHash[i];
        while (a != NULL)
        {
            l++;
            edges++;
            if (a->msf) msf_cst += a->weight;
            a = a->next;
        }
        if (l > 0)
        {
            occupied_pos++;
            if (l > max_len) max_len = l;
        }
    }

    /* Counting connected components */
    int *cc = calloc(g->nodes+1, sizeof(int));
    for (int i = 0; i < g->nodes; i++)
    {
        int c = g->cCon[i];
        if (cc[c] == 0)
        {
            cc[c] = 1;
            num_cc++;
        }
    }
    free(cc);
    avg_len = (occupied_pos > 0) ? (double)edges/occupied_pos : 0.0;
    
    printf("Numero posizioni non vuote: %d\n", occupied_pos);
    printf("Lunghezza media liste: %f\n", avg_len);
    printf("Lunghezza massima liste: %d\n", max_len);
    printf("%d %d %ld\n", edges, num_cc, msf_cst);
 
}
/* Loops over the neighbors of l and set msf fild to new_val */
void set_msf_neighbors(elemento *l, int idx, bool new_val)
{
    elemento *h = l;
    while (h != NULL)
    {
        if (h->id == idx) h->msf = new_val;
        h = h->next;
    }
}

/* Adds an edge to the graph */
int aggiungi_arco(grafo *g, int u, int v, int w)
{
    /* Edge already exists */
    int fu = u < v ? u : v;
    int fv = u < v ? v : u;
    arco *e_found = find_lock(g, fu, fv);
    /* Edge not found */
    if (e_found != NULL) return ILLEGAL;

    /* u and v belong to different components */
    if (g->cCon[u] != g->cCon[v])
    {
        arco *a = malloc(sizeof(*a));
        if (a == NULL)
        {
            fprintf(stderr, "Error[Line: %d - File: %s]: Memory allocation failed\n", QUI);
            exit(1);
        }
        a->u = u;
        a->v = v;
        a->weight = w;
        a->msf = true;
        /* Insert the new edge to the MSF */
        insert_lock(g, a);

        /* Building vicini[u] and vicini[v] adjacency lists */
        elemento *a1 = malloc(sizeof(*a1));
        if (a1 == NULL) 
        {
            fprintf(stderr, "Error[Line: %d - File :%ss]: Memory allocation for a1 in aggiungi_arco failed\n", QUI);
            exit(2);
        }
        a1->id = v;
        a1->msf = true;
        a1->w = w;
        a1->next = NULL;

        elemento *a2 = malloc(sizeof(*a2));
        if (a2 == NULL) 
        {
            fprintf(stderr, "Error [Line: %d - File :%ss]: Memory allocation for a2 in aggiungi_arco\n", QUI);
            exit(3);
        }
        a2->id = u;
        a2->msf = true;
        a2->w = w;
        a2->next = NULL;
        g->vicini[u] = insert_ord(g->vicini[u], a1, compare_id);
        g->vicini[v] = insert_ord(g->vicini[v], a2, compare_id);

        /* Updating cCon: the new component's id is the smallest node's id of the two components */
        int id_u = g->cCon[u];
        int id_v = g->cCon[v];
        int new_id = id_u < id_v ? id_u : id_v;
        int old_id = id_u < id_v ? id_v : id_u;
        for (int i = 0; i <= g->nodes; i++)
        {
            if (old_id == g->cCon[i]) g->cCon[i] = new_id; 
        }
        /* Updating the graph stats */
        pthread_mutex_lock(&g->updates);
        g->edges++;
        g->numCoCo--;
        g->costoMSF += w;
        pthread_mutex_unlock(&g->updates);
        return IN_MSF;
    }
    else /* u and v belong to the same component */
    {
        bool *visited = malloc(sizeof(bool) * (g->nodes + 1)); 
        int *parents = malloc(sizeof(int) * (g->nodes + 1));
        int *pred_weight = malloc(sizeof(int) * (g->nodes +1));
        int *q = malloc(sizeof(int) * (g->nodes + 1));
        if (!visited || !parents || !pred_weight || !q)
        {
            fprintf(stderr, "Error [Line: %d - File :%s]: Memory allocation for visited, parents, q failed!\n", QUI);
            exit(4);
        }
        int h = 0;
        int t = 0;
        for (int i = 0; i <= g->nodes; i++) visited[i] = false;
        for (int i = 0; i <= g->nodes; i++) parents[i] = -1;

        /* Performs a BFS from u traversing only MSF edges */
        q[h] = u;
        t++;
        visited[u] = true;
        while (h != t)
        {
            int curr = q[h];
            h++;
            elemento *e = g->vicini[curr];
            while (e != NULL)
            {
                if (e->msf && !visited[e->id])
                {
                    visited[e->id] = true;
                    parents[e->id] = curr;
                    pred_weight[e->id] = e->w;
                    q[t] = e->id;
                    t++;
                }
                e = e->next;
            }
        }
        /* Searching for the unique path from u to v and find the edge with the largest weight */
        int max_w = INT_MIN;
        int i = v;
        int max_edge_u = -1;
        int max_edge_v = -1;
        while (i != u)
        {
            if (pred_weight[i] > max_w)
            {
                max_w = pred_weight[i];
                max_edge_u = parents[i];
                max_edge_v = i;
            }
            i = parents[i];
        }

        int res; /* Result */
        if (max_w > w)
        {
            arco *new_a = malloc(sizeof(*new_a));
            if (new_a == NULL)
            {
                fprintf(stderr, "Error [Line: %d - File: %s]: Memory allocation for new_a in aggiungi_Arco failed!\n", QUI);
                exit(5);
            }
            /* Finding the edge */
            int fu = (max_edge_u < max_edge_v) ? max_edge_u : max_edge_v;
            int fv = (max_edge_u < max_edge_v) ? max_edge_v : max_edge_u;
            arco *a_found = find_lock(g, fu, fv);
            /* Remove the old edge from the MSF. Still in the graph though */
            a_found->msf = false;
            
            set_msf_neighbors(g->vicini[fu], fv, false);
            set_msf_neighbors(g->vicini[fv], fu, false);

            /* Inserting the new edge in the hash table */
            new_a->u = u;      
            new_a->v = v;      
            new_a->weight = w;
            new_a->msf = true;

            insert_lock(g, new_a);

            elemento *a1 = malloc(sizeof(*a1));
            if (a1 == NULL) 
            { 
                fprintf(stderr, "Error[Line: %d - File: %s]: Memory allocation for a1 in aggiungi_arco failed\n", QUI); 
                exit(6); 
            }
            a1->id = v;
            a1->msf = true;
            a1->w = w;
            a1->next = NULL;
            
            elemento *a2 = malloc(sizeof(*a2));
            if (a2 == NULL)
            { 
                fprintf(stderr, "Error [Line: %d - File: %s]: Memory allocation for a1 in aggiungi_arco failed\n", QUI); 
                exit(7);
            }
            a2->id = u;
            a2->msf = true;
            a2->w = w;
            a2->next = NULL;
            
            /* Updating vicini adjacency list for u and v */
            g->vicini[u] = insert_ord(g->vicini[u], a1, compare_id);
            g->vicini[v] = insert_ord(g->vicini[v], a2, compare_id);
            
            pthread_mutex_lock(&g->updates);
            g->costoMSF += w - max_w;
            g->edges++;
            pthread_mutex_unlock(&g->updates);
            res = IN_MSF;

        }
        else /* max_w <= w */ 
        {
            arco *new_a = malloc(sizeof(*new_a));
            if (new_a == NULL)
            {
                fprintf(stderr, "Error [Line: %d - File: %s]: Memory allocation for new_a in aggiungi_Arco failed!\n", QUI);
                exit(8);
            }
            new_a->u = u;
            new_a->v = v;
            new_a->msf = false;
            new_a->weight = w;
            new_a->next = NULL;

            /* Insert the new edge into the hash table but it doesn't belong to MSF */
            insert_lock(g, new_a);
            
            elemento *a1 = malloc(sizeof(*a1));
            if (a1 == NULL) 
            { 
                fprintf(stderr, "Error [Line: %d - File: %s]: Memory allocation for a1 in aggiungi_Arco failed\n", QUI); 
                exit(9); 
            }
            a1->id = v;
            a1->msf = false;
            a1->w = w;
            a1->next = NULL;
            
            elemento *a2 = malloc(sizeof(*a2));
            if (a2 == NULL)
            { 
                fprintf(stderr, "Error [Line: %d - File: %s]: Memory allocation for a1 in aggiungi_Arco failed\n", QUI); 
                exit(10);
            }
            a2->id = u;
            a2->msf = false;
            a2->w = w;
            a2->next = NULL;
            
            /* Updates vicini adjacency lists */
            g->vicini[u] = insert_ord(g->vicini[u], a1, compare_id);
            g->vicini[v] = insert_ord(g->vicini[v], a2, compare_id);
            pthread_mutex_lock(&g->updates);
            g->edges++;
            pthread_mutex_unlock(&g->updates);
            res = NOT_IN_MSF;
        }
        free(parents);
        free(pred_weight);
        free(visited);
        free(q);

        return res;
    } 
}

/* Utility - finds the min edge between Lu and Lv components. Returns true if an edge is found */
bool find_min(node *abr, node *root_small, grafo *g, int *min_u, int *min_v, int *min_w, bool *found)
{
    if (abr == NULL) return *found;
    /* Visit left subtree */
    find_min(abr->left, root_small, g, min_u, min_v, min_w, found);
    /* Takes original id */
    int orig_u = unshuffle(abr->val_node);
    elemento *el = g->vicini[orig_u];
    while (el != NULL)
    {
        /* Check whether the edge cross the cut
         * - does not belong to MSF 
         * - connects to a node outside the smallest components 
         */
        if (!el->msf && !abr_search(root_small, el->id))
        {
            /* Found an edge */ 
            if (!(*found) || el->w < *min_w)
            {
                /* Update edge with minimum weight found */
                *min_u = orig_u;
                *min_v = el->id;
                *min_w = el->w;
                *found = true;
            }
        }
        el = el->next; 
        }  
    /* Visit right subtree */
    find_min(abr->right, root_small, g, min_u, min_v, min_w, found);
    return *found;
}
/* Removes an edge from the MSF */
int cancella_arco(grafo *g, int u, int v)
{
    int fu = (u < v) ? u : v;
    int fv = (u < v) ? v : u;
    arco *edge = find_lock(g, fu, fv);
    if (edge == NULL) return NO_EXISTENCE;
    
    /* Edge is not into MSF */
    if (!edge->msf)
    {
        remove_element(&g->vicini[fu], fv);
        remove_element(&g->vicini[fv], fu);
        remove_lock(g, fu, fv);
        pthread_mutex_lock(&g->updates);
        g->edges--;
        pthread_mutex_unlock(&g->updates);
        return DELETED;
    }
    /* Edge is in the MSF */
    else
    {
        int old_w = edge->weight;
        int old_cc = g->cCon[fu]; /* Used by find_min */

        /* Remove the edge from the hash table and adjacency lists */
        remove_lock(g, fu, fv);
        remove_element(&g->vicini[fu], fv);
        remove_element(&g->vicini[fv], fu);
        
        int min_u = fu;
        int min_v = fv;

        /* BFS queues */
        queue_t *qu = queue_init();
        queue_t *qv = queue_init();

        int initial_size = 10;
        int cap_u = initial_size;
        int cap_v = initial_size;
        int inserted_u = 1;
        int inserted_v = 1;
        /* Keeping track of the nodes of both components */
        int *node_u = malloc(sizeof(int) * initial_size);
        int *node_v = malloc(sizeof(int) * initial_size);
        if (!node_u || !node_v) 
        {
            fprintf(stderr, "Error[Line: %d - File: %s]: memory allocation failed\n", QUI);
            exit(10);
        }

        node_u[0] = fu;
        node_v[0] = fv;

        enqueue(qu, fu);
        enqueue(qu, -1); /* fu's parent */
        enqueue(qv, fv);
        enqueue(qv, -1); /* fv's parent */
        /* Alternating BFS */
        while (!is_empty(qu) && !is_empty(qv))
        {
            int current_u = dequeue(qu);
            int parent_u = dequeue(qu);
            if (current_u < min_u) min_u = current_u;

            elemento *ele = g->vicini[current_u];
            while (ele != NULL)
            {
                if (ele->msf && ele->id != parent_u)
                {
                    /* Check node_u capacity */
                    if (inserted_u == cap_u)
                    {
                        int new_cap_u = cap_u * 2;
                        node_u = realloc(node_u, sizeof(int)*new_cap_u);
                        if (node_u == NULL)
                        {
                            fprintf(stderr, "Error[Line: %d - File: %s]: memory allocation failed\n", QUI);
                            exit(11);
                        }
                        cap_u = new_cap_u;
                    }
                    node_u[inserted_u++] = ele->id;
                    enqueue(qu, ele->id);
                    enqueue(qu, current_u);
                }
                /* Keep exploring vicini u */
                ele = ele->next;
            }
            int current_v = dequeue(qv);
            int parent_v = dequeue(qv);
            if (current_v < min_v) min_v = current_v;

            ele = g->vicini[current_v];
            while (ele != NULL)
            {
                if (ele->msf && ele->id != parent_v)
                {
                    /* Check node_u capacity */
                    if (inserted_v == cap_v)
                    {
                        int new_cap_v = cap_v * 2;
                        node_v = realloc(node_v, new_cap_v*sizeof(int));
                        if (node_v == NULL)
                        {
                            fprintf(stderr, "Error[Line: %d - File: %s]: memory allocation failed\n", QUI);
                            exit(11);
                        }
                        cap_v = new_cap_v;
                    }
                    node_v[inserted_v++] = ele->id;
                    enqueue(qv, ele->id);
                    enqueue(qv, current_v);
                }
                /* Keep exploring vicini v */
                ele = ele->next;
            }
        }
        bool small_comp = is_empty(qu);
        int small_size;
        int largest_size;
        int *nodes_smallest_comp;
        int *nodes_largest_comp;
        int min_small_comp;
        int min_largest_comp;
        int largest_capacity;
        queue_t *largest_queue;
        /* Selecting the smalleer component then builds the abr */
        if (small_comp) 
        {
            nodes_smallest_comp = node_u;
            nodes_largest_comp = node_v;
            small_size = inserted_u;
            largest_size = inserted_v;
            min_small_comp = min_u;
            min_largest_comp = min_v;
            largest_queue = qv;
            largest_capacity = cap_v;
        }
        else 
        {
            nodes_smallest_comp = node_v;
            nodes_largest_comp = node_u;
            small_size = inserted_v;
            largest_size = inserted_u;
            min_small_comp = min_v;
            min_largest_comp = min_u;
            largest_queue = qu;
            largest_capacity = cap_u;
        }

        /* Building the smaller abr */
        node *smallest_abr = NULL;
        for (int i = 0; i < small_size; i++) smallest_abr = abr_insert(smallest_abr, nodes_smallest_comp[i]);
 
        int new_u = -1;
        int new_v = -1;
        int new_w = INT_MAX;
        bool found = false;
        bool is_found = find_min(smallest_abr, smallest_abr, g, &new_u, &new_v, &new_w, &found);
        /* Retrieve the replacement edge from the hash table */
        arco *min_edge = NULL;
        if (is_found) 
        {
            /* Normalize edges */
            int norm_a = new_u < new_v ? new_u : new_v;
            int norm_b = new_u < new_v ? new_v : new_u;
            min_edge = find_lock(g, norm_a, norm_b);
        }

        /* Getting the minimum edge */
        if (is_found)
        {
            /* Normalizing edges */
            int norm_a = (new_u < new_v) ? new_u : new_v;
            int norm_b = (new_u < new_v) ? new_v : new_u;
            min_edge = find_lock(g, norm_a, norm_b);
        }
        if (min_edge == NULL) /* Edge not found: splitting components */
        {
            bool find_min_id = abr_search(smallest_abr, old_cc);
            /* If the original minimum belongs to the smallest component
             * the BFS of the largest component must be completed */
            if (find_min_id)
            {
                while (!is_empty(largest_queue))
                {
                    int cur = dequeue(largest_queue);
                    int p = dequeue(largest_queue);
                    if (cur < min_largest_comp) min_largest_comp = cur;
                    elemento *e = g->vicini[cur];
                    while (e != NULL)
                    {
                        if (e->msf && p != e->id)
                        {
                            /* Check for capacity */
                            if (largest_size == largest_capacity)
                            {
                                int new_capacity = 2*largest_capacity;
                                nodes_largest_comp = realloc(nodes_largest_comp, new_capacity*sizeof(int)); 
                                if (small_comp) node_v = nodes_largest_comp;
                                else node_u = nodes_largest_comp;
                                if (nodes_largest_comp == NULL)
                                {
                                    fprintf(stderr, "Error[Line: %d - File: %s]: memory allocation failed\n", QUI);
                                    exit(12);
                                }
                                largest_capacity = new_capacity;
                            }
                            nodes_largest_comp[largest_size++] = e->id;
                            enqueue(largest_queue, e->id);
                            enqueue(largest_queue, cur);
                        }
                        e = e->next;
                    }
                }
            }
            /* Updating the id of the smaller component */
            for (int i = 0; i < small_size; i++) g->cCon[nodes_smallest_comp[i]] = min_small_comp;
            /* Updating the id of the largest component if necessary */
            if (find_min_id)
            {
                for (int i = 0; i < largest_size; i++) g->cCon[nodes_largest_comp[i]] = min_largest_comp;
            }
        }
        else /* Edge found: replacement edge does exist */
        {
            int bucket_min = hash(min_edge->u, min_edge->v, g->hash_size);
            int mu_idx_min = bucket_min % g->nmutex;
            pthread_mutex_lock(&g->mut_gHash[mu_idx_min]);
            min_edge->msf = true;
            pthread_mutex_unlock(&g->mut_gHash[mu_idx_min]);
            
            /* Updates adjacency lists */
            set_msf_neighbors(g->vicini[min_edge->u], min_edge->v, true);
            set_msf_neighbors(g->vicini[min_edge->v], min_edge->u, true);

        }
        pthread_mutex_lock(&g->updates);
        g->edges--;
        if (min_edge == NULL)
        {
            g->numCoCo++;
            g->costoMSF -= old_w;
        }
        else g->costoMSF = g->costoMSF + (min_edge->weight - old_w);
        pthread_mutex_unlock(&g->updates);
        free_abr(smallest_abr);
        queue_destroy(qu);
        queue_destroy(qv);
        free(node_u);
        free(node_v);
    }
    return DELETED;
}
