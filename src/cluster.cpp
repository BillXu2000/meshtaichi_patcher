#include "cluster.h"
#include <algorithm>
#include <queue>
#include <set>

Csr Cluster::run(Csr &graph) {
    if (option == "kmeans") return run_kmeans(graph);
    if (option == "greedy") return run_greedy(graph);
    assert(false);
}

Csr Cluster::run_kmeans(Csr &graph) {
    using namespace std;
    assert(patch_size != -1); // patch_size has to be set before run cluster
    int n = graph.size();
    vector<int> seeds, color;
    color.resize(n);
    for (auto &i: color) {
        i = 0;
    }
    for (int u = 0; u < n; u++) {
        if (color[u] == 0) {
            queue<int> q;
            vector<int> con;
            q.push(u);
            color[u] = 1;
            while (!q.empty()) {
                int u = q.front();
                q.pop();
                con.push_back(u);
                for (auto v : graph[u]) {
                    if (color[v] == 0) {
                        color[v] = 1;
                        q.push(v);
                    }
                }
            }
            random_shuffle(con.begin(), con.end());
            seeds.insert(seeds.end(), con.begin(), con.begin() + con.size() / patch_size / 4 + 1);
        }
    }

    vector<bool> locked(n);
    color.resize(n);
    for (auto &i: color) {
        i = -1;
    }
    for (int i = 0; i < n; i++) {
        locked[i] = false;
    }
    int round_n = 1;
    for (int round = 1; true; round++) {
        // coloring
        queue<int> q;
        set<int> uni;
        for (int i = 0; i < seeds.size(); i++) {
            int u = seeds[i];
            if (color[u] == -1) {
                color[u] = i;
                q.push(u);
            }
            uni.insert(u);
        }
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            for (auto v : graph[u]) {
                if (color[v] == -1) {
                    color[v] = color[u];
                    q.push(v);
                }
            }
        }
        vector<array<int, 2> > pairs;
        for (int u = 0; u < n; u++) {
            pairs.push_back({color[u], u});
        }
        auto owned = Csr(pairs);
        auto total = owned.mul_unique(graph);
        int ma = 0, cnt = 0;
        for (int u = 0; u < owned.size(); u++) {
            // ma = max(ma, owned[u].size());
            ma = max(ma, total[u].size());
            if (total[u].size() > patch_size) cnt++;
        }
        if (ma <= patch_size) return owned;

        // update seeds
        std::vector<bool> visit(n);
        for (int i = 0; i < n; i++) {
            visit[i] = false;
        }
        for (int p = 0; p < owned.size(); p++) {
            // if (total[p].size() <= patch_size) continue;
            if (round % round_n == 0 && total[p].size() <= patch_size) {
                locked[p] = true;
            }
            if (locked[p]) continue;
            queue<int> q;
            for (auto u: owned[p]) {
                for (auto v: graph[u]) {
                    if (color[u] != color[v]) {
                        visit[u] = true;
                        q.push(u);
                        break;
                    }
                }
            }
            int last = seeds[p];
            while (!q.empty()) {
                int u = q.front();
                last = u;
                q.pop();
                for (auto v : graph[u]) {
                    if (color[v] == p && !visit[v]) {
                        visit[v] = true;
                        q.push(v);
                    }
                }
            }
            seeds[p] = last;
            if (round % round_n == 0) {
                for (auto v: graph[last]) {
                    if (color[v] == p && v != last) {
                        seeds.push_back(v);
                        locked.push_back(false);
                        break;
                    }
                }
            }
            for (auto u: owned[p]) {
                color[u] = -1;
            }
            // {
            //     int index = rand() % owned[p].size();
            //     int u = owned[p][index];
            //     if (u != seeds[p]) {
            //         seeds.push_back(u);
            //     }
            // }
        }
    }
}

Csr Cluster::run_greedy(Csr &graph) {
    using namespace std;
    assert(patch_size != -1); // patch_size has to be set before run cluster
    typedef array<int, 2> int2;
    int n = graph.size();
    // priority_queue<int2, vector<int2>, greater<int2>> seeds;
    priority_queue<int2, vector<int2>> seeds;
    vector<int> color(n), degree(n);
    for (int i = 0; i < n; i++) {
        color[i] = -1;
        degree[i] = graph[i].size();
        seeds.push({degree[i], i});
    }
    int color_n = 0;
    // for (int seed = 0; seed < n; seed++) {
    while (!seeds.empty()) {
        int seed = seeds.top()[1];
        seeds.pop();
        if (color[seed] != -1) continue;
        priority_queue<int2> neighbors;
        vector<int> connect(n);
        vector<bool> visited(n);
        unordered_set<int> total;
        neighbors.push({0, seed});
        for (int i = 0; i < n; i++) {
            connect[i] = 0;
            visited[i] = false;
        }
        while (!neighbors.empty()) {
            int u = neighbors.top()[1], k = neighbors.top()[0];
            neighbors.pop();
            if (color[u] == color_n || visited[u]) continue;
            visited[u] = true;
            int sum = total.size();
            for (auto v: graph[u]) {
                if (total.find(v) == total.end()) {
                    sum++;
                }
            }
            if (sum > patch_size) continue;
            color[u] = color_n;
            for (auto v: graph[u]) {
                total.insert(v);
                if (color[v] == -1) {
                    connect[v]++;
                    neighbors.push({connect[v], v});
                    degree[v]--;
                    seeds.push({degree[v], v});
                }
            }
        }
        color_n++;
    }
    vector<array<int, 2> > pairs;
    for (int u = 0; u < n; u++) {
        pairs.push_back({color[u], u});
    }
    return Csr(pairs);
}