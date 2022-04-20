#include "cluster.h"
#include <algorithm>
#include <queue>
#include <set>

Csr Cluster::run(Csr &graph) {
    using namespace std;
    int n = graph.size();
    vector<int> seeds, color;
    for (int i = 0; i < n; i++) {
        seeds.push_back(i);
    }
    srand(0);
    random_shuffle(seeds.begin(), seeds.end());
    seeds.erase(seeds.begin() + max(n / patch_size, 1), seeds.end());

    color.resize(n);
    while(true) {
        // coloring
        for (auto &i: color) {
            i = -1;
        }
        queue<int> q;
        vector<array<int, 2> > pairs;
        for (int i = 0; i < seeds.size(); i++) {
            int u = seeds[i];
            color[u] = i;
            pairs.push_back({i, u});
            q.push(u);
        }
        while (!q.empty()) {
            int u = q.front();
            q.pop();
            for (auto v : graph[u]) {
                if (color[v] == -1) {
                    color[v] = color[u];
                    pairs.push_back({color[v], v});
                    q.push(v);
                }
            }
        }
        auto patch = Csr(pairs);
        int ma = 0;
        for (int u = 0; u < patch.size(); u++) {
            ma = max(ma, patch[u].size());
        }
        if (ma <= patch_size) return patch;

        // update seeds
        std::vector<bool> visit(n);
        std::set<int> seeds_set;
        for (int p = 0; p < patch.size(); p++) {
            queue<int> q;
            for (auto u: patch[p]) {
                for (auto v: graph[u]) {
                    if (color[u] != color[v]) {
                        visit[u] = true;
                        q.push(u);
                        break;
                    }
                }
            }
            int last;
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
            seeds_set.insert(last);
            if (patch[p].size() > patch_size) {
                seeds_set.insert(*graph[last].begin());
            }
        }
        seeds.clear();
        for (auto seed: seeds_set) {
            seeds.push_back(seed);
        }
    }
}