import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.IOException;
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.HashMap;

public class Msf {
    public static class Arco implements Comparable<Arco> {
        int u;
        int v;
        int w;
        boolean msf;

        Arco (int u, int v, int w) {
            this.u = u;
            this.v = v;
            this.w = w;
            this.msf = false;
        }

        @Override
        public boolean equals(Object obj) {
            if (obj == null) return false;
            if (!(obj instanceof Arco)) return false;
            Arco a = (Arco)obj;
            return (this.u == a.u) && (this.v == a.v) && (this.w == a.w); 
        }

        @Override
        public int compareTo(Arco other) {
            return Integer.compare(this.w, other.w);
        }

        @Override
        public String toString() {
            return "(" + u + ", " + v + ", " + w + ")";
        }
    }

    public static long hash(int u, int v) {
        return ((long)(u + v)) * (u + v + 1) / 2 + v;
    }

    public static int find(int[] parents, int comp) {
        if (parents[comp] == comp) return comp;
        return parents[comp] = find(parents, parents[comp]);
    }

    public static void union(int[] parents, int rank[], int u, int v) {
        int ra = find(parents, u);
        int rb= find(parents, v);

        if (ra != rb) {
            if (rank[ra] < rank[rb]) parents[ra] = rb;
            else if (rank[ra] > rank[rb]) parents[rb]= ra;
            else {
                parents[rb] = ra;
                rank[ra]++;
            }
        }
    }

    public static void main(String[] args) {
        if (args.length < 2) {
            System.err.println("Usage: java Msf file_grafo file_operazioni\n");
            System.exit(1);
        }
        
        // Edges number
        int num_edges = 0;
        int nodes = 0;
        // Number of connected components
        int num_cc;
        // MSF cost
        long msf_cost = 0;
        
        // Hash table for our graph
        HashMap<Long, Arco> g = new HashMap<>();
        // List of MST edges
        ArrayList<Arco> edges = new ArrayList<>();
        try {
            BufferedReader br = new BufferedReader(new FileReader(args[0]));
            String line;
            while ((line = br.readLine()) != null) {
                if (line.length() == 0 || line.charAt(0)== 'c') continue;
                String[] lines = line.trim().split("\\s+");
                if (lines[0].equals("p")) {
                    nodes = Integer.parseInt(lines[2]) + 1;
                    num_edges = Integer.parseInt(lines[3]);
                }
                if (lines[0].equals("a")) {
                    int u = Integer.parseInt(lines[1]);
                    int v = Integer.parseInt(lines[2]);
                    int w = Integer.parseInt(lines[3]);
                    if (u > v) {
                        int tmp = u;
                        u = v;
                        v = tmp;
                    }
                    Arco a = new Arco(u, v, w);
                    g.put(Msf.hash(u, v), a);
                    edges.add(a);
                }
            }
            br.close();
        } 
        catch(IOException e) {
            System.err.println("Error " + e + e.getMessage() + "\n");
            System.exit(2);
        } 
        // Print hash table 
        // g.forEach((h, val) -> System.out.println(h + ": " + val));

        num_cc = nodes;
        // Reading operations file
        try {
            BufferedReader br = new BufferedReader(new FileReader(args[1]));
            String line;
            while ((line = br.readLine()) != null) {
                if (line.length() == 0) continue;
                String[] lines = line.trim().split("\\s+");
                if (lines[0].equals("c")) continue;
                if (lines[0].equals("+")) {
                    int u = Integer.parseInt(lines[1]);
                    int v = Integer.parseInt(lines[2]);
                    int w = Integer.parseInt(lines[3]);
                    if (u > v) {
                        int tmp = u;
                        u = v;
                        v = tmp;
                    }
                    Arco read_edge = new Arco(u, v, w);
                    long key = hash(u, v);
                    Arco e = g.get(key);
                    if (e != null && e.equals(read_edge)) {
                        // Found an already existing edge
                        System.err.println("Edge already exists\n");
                        System.exit(3);
                    } 
                    // Insert the edge into the graph
                    g.put(key, read_edge);
                    edges.add(read_edge);
                    num_edges++;
                }
                if (lines[0].equals("-")) {
                    int u = Integer.parseInt(lines[1]);
                    int v = Integer.parseInt(lines[2]);
                    if (u > v) {
                        int tmp = u;
                        u = v;
                        v = tmp;
                    }
                    long key = hash(u, v);
                    Arco to_be_removed_edge = g.remove(key);
                    if (to_be_removed_edge == null) {
                        System.err.println("Edge to be removed does not exist\n");
                        System.exit(4);
                    }
                    edges.remove(to_be_removed_edge);
                    num_edges--;
                }
            }
            br.close();
        } catch (IOException e) {
            System.err.println("Error: " + e);
            e.printStackTrace();
            System.exit(2);
        }

        edges.sort(Arco::compareTo); // sort edges array
        // keep tracks of the parents
        int[] parents = new int[nodes];
        int[] rank = new int[nodes];
        for (int i = 0; i < nodes; i++) {
            parents[i] = i;
            rank[i] = 0;
        }
       
        for (int i = 0; i < edges.size(); i++) {
            int e1 = find(parents, edges.get(i).u);
            int e2 = find(parents, edges.get(i).v);
            int w = edges.get(i).w;
            if (e1 != e2) {
                // edges lay in different components
                union(parents, rank, e1, e2);
                msf_cost += w;
                edges.get(i).msf = true;
                num_cc--;
            }
        }
        System.out.printf("%d %d %d\n", num_edges, num_cc, msf_cost);
    }
}  

