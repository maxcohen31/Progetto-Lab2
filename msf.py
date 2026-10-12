#!/usr/bin/python3

from sys import argv, stderr
import heapq
    
def build_graph(file):
    nodes = 0
    edges = []
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
                k = (min(u, v), max(u, v))
                ht[k] = w
                edges[u].append((v, w))
                edges[v].append((u, w))    
    return ht, edges, nodes

def aggiungi_arco(edges, ht, u, v, w):
    if u < 0 or v < 0 or u >= len(edges) or v >= len(edges):
        return False
    k = (min(u, v), max(u, v))
    if k in ht: return False
    ht[k] = w
    edges[u].append((v, w))
    edges[v].append((u, w))
    return True

def cancella_arco(edges, ht, u, v):
    if u < 0 or v < 0 or u >= len(edges) or v >= len(edges):
        return False
    k = (min(u, v), max(u, v))
    if k not in ht:
        stderr.write("Error: missing edge")
    if k not in ht: return False
    del ht[k]

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

def prim(nodes, edges):
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
            w, _, v = heapq.heappop(hp)
            if visited[v]: continue
            visited[v] = True
            msf_cost += w
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
                aggiungi_arco(edg, ht, u, v, w)
            elif op[0] == '-':
                u = int(op[1])
                v = int(op[2])
                cancella_arco(edg, ht, u, v)
             
    num_cc, msf_cost = prim(nod, edg)
    n_edges = len(ht)
    print(f"{n_edges} {num_cc} {msf_cost}")
  
if __name__ == "__main__":
    main()
