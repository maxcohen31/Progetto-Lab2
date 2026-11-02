#!/usr/bin/env python3

from os import write
from sys import argv, stderr
import heapq
    
def build_graph(file):
    """
    Returns a dictionary, the list of edges and the total nodes of the graph
    """
    nodes = 0
    # edges -> [(neighbor, w)]
    edges = []
    # ht -> {(u,v): w}
    ht = {}
    with open(file) as f:
        for line in f:
            char = line.strip().split()
            if not char: continue
            if char[0] == 'c': continue
            elif char[0] == 'p':
                nodes = int(char[2]) + 1
                edges = [[] for _ in range(nodes)]
            elif char[0] == 'a':
                u = int(char[1])
                v = int(char[2])
                w = int(char[3])
                # key is a tuple -> immutable
                k = (min(u, v), max(u, v))
                ht[k] = w
                # Build the adjacency lists of u and v
                edges[u].append((v, w))
                edges[v].append((u, w))    
    return ht, edges, nodes

def aggiungi_arco(edges, ht, u, v, w):
    """
    Returns T if a edge is added. Otherwise returns F
    """
    k = (min(u, v), max(u, v))
    # edge already exists
    if k in ht: return False
    ht[k] = w
    edges[u].append((v, w))
    edges[v].append((u, w))
    return True

def cancella_arco(edges, ht, u, v):
    """
    Deletes an edge from the graph. Returns T if the given edge is removed, F otherwise
    """
    k = (min(u, v), max(u, v))
    if k not in ht: return False
    # removes the entry from ht
    del ht[k]
    
    # build the adjacency lists without the removed edge
    adj_u = []
    adj_v = []
    for (i, j) in edges[u]:
        if i != v:
            adj_u.append((i, j))
    edges[u] = adj_u
    for (i, j) in edges[v]:
        if i != u:
            adj_v.append((i, j))        
    edges[v] = adj_v
    return True

def prim(nodes, edges):
    """
    Prim's algorithm. Returns the MSF cost and the number of connected components
    """
    num_cc = 0
    msf_cost = 0
    visited = [False] * nodes

    for n in range(nodes):
        if visited[n]: continue
        num_cc += 1
        visited[n] = True
        
        hp = []
        for (neigh, weight) in edges[n]:
            heapq.heappush(hp, (weight, n, neigh))

        while len(hp) != 0:
            # pop w and destination node
            w, _, v = heapq.heappop(hp)
            if visited[v]: continue
            visited[v] = True
            msf_cost += w
            # loops over v's neighbors
            for (z, wz) in edges[v]:
                if not visited[z]:
                    heapq.heappush(hp, (wz, v, z))

    return num_cc, msf_cost 
    
def main():
    if len(argv) < 3:
        stderr.write("Usage: msf.py file_grafo file_archi")
        exit(1)
    ht, edg, nod = build_graph(argv[1])
    
    with open(argv[2]) as f:
        for l in f:
            op = l.strip().split()
            if not op: continue
            if op[0] == 'c': continue
            if op[0] == '+':
                u = int(op[1])
                v = int(op[2])
                w = int(op[3])
                if not aggiungi_arco(edg, ht, u, v, w):
                    print("Error aggiungi_arco", file=stderr)
            elif op[0] == '-':
                u = int(op[1])
                v = int(op[2])
                if not cancella_arco(edg, ht, u, v):
                    print("Error cancella_arco", file=stderr)
             
    num_cc, msf_cost = prim(nod, edg)
    n_edges = len(ht)
    print(f"{n_edges} {num_cc} {msf_cost}")
  
if __name__ == "__main__":
    main()
