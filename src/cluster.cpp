#include "cluster.h"
#include <algorithm>
#include <queue>
#include <set>

Csr Cluster::run(Csr &graph_cv) {
    // if (option == "kmeans") return run_kmeans(graph);
    // if (option == "greedy") return run_greedy(graph);
    if (option == "greedy") return run_greedy_cv(graph_cv);
    if (option == "unbound" || option == "kmeans") {
        Csr graph_vc = graph_cv.transpose();
        Csr graph_cc = graph_cv.mul_unique(graph_vc);
        return run_unbound(graph_cc);
    }
    std::cerr << option << ": option not valid!\n";
    assert(false);
    return Csr();
}

Csr Cluster::color2ans(std::vector<int> &color, Csr &graph) {
    using namespace std;
    int n = color.size();
    map<int, unordered_set<int> > totals;
    for (int u = 0; u < n; u++) {
        totals[color[u]].insert(u);
        for (auto v: graph[u]) {
            totals[color[u]].insert(v);
        }
    }
    typedef array<int, 2> int2;
    set<int2> colors;
    for (auto i: totals) {
        colors.insert({-int(i.second.size()), i.first});
    }
    map<int, int> color_map;
    for (int c = 0; !colors.empty(); c++) {
        int sum = 0;
        while(!colors.empty()) {
            auto i = colors.lower_bound({sum - patch_size, 0});
            if (i == colors.end()) break;
            if (sum == 0) sum += patch_size / 4;
            sum -= (*i)[0];
            color_map[(*i)[1]] = c;
            colors.erase(i);
        }
    }
    vector<array<int, 2> > pairs;
    for (int u = 0; u < n; u++) {
        pairs.push_back({color_map[color[u]], u});
    }
    return Csr(pairs);
}

Csr Cluster::color2ans_cv(std::vector<int> &color, Csr &graph, Csr &graph_inv, std::vector<int> &total_size) {
    using namespace std;
    int n = color.size();
    // map<int, unordered_set<int> > totals, owneds;
    // for (int u = 0; u < n; u++) {
    //     totals[color[u]].insert(u);
    //     for (auto w: graph[u]) {
    //         auto &owned = owneds[color[u]];
    //         if (owned.find(w) != owned.end()) continue;
    //         owned.insert(w);
    //         for (auto v: graph_inv[w]) {
    //             totals[color[u]].insert(v);
    //         }
    //     }
    // }
    typedef array<int, 2> int2;
    set<int2> colors;
    // for (auto i: totals) {
    //     // colors.insert({-int(i.second.size()), i.first});
    //     colors.insert({-int(i.second.size()), i.first});
    // }
    for (int i = 0; i < total_size.size(); i++) {
        colors.insert({-total_size[i], i});
    }
    map<int, int> color_map;
    for (int c = 0; !colors.empty(); c++) {
        int sum = 0;
        while(!colors.empty()) {
            auto i = colors.lower_bound({sum - patch_size, 0});
            if (i == colors.end()) break;
            if (sum == 0) sum += patch_size / 4;
            sum -= (*i)[0];
            color_map[(*i)[1]] = c;
            colors.erase(i);
        }
    }
    vector<array<int, 2> > pairs;
    for (int u = 0; u < n; u++) {
        pairs.push_back({color_map[color[u]], u});
    }
    // vector<array<int, 2> > pairs;
    // for (int u = 0; u < n; u++) {
    //     pairs.push_back({color[u], u});
    // }
    return Csr(pairs);
}

Csr Cluster::run_unbound(Csr &graph) {
    using namespace std;
    typedef array<int, 2> int2;
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
            seeds.insert(seeds.end(), con.begin(), con.begin() + con.size() / patch_size + 1);
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
    Csr owned_out;
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
        if (round == 20) {
            owned_out = owned;
            break;
        }
        std::vector<bool> visit(n);
        for (int i = 0; i < n; i++) {
            visit[i] = false;
        }
        for (int p = 0; p < seeds.size(); p++) {
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
        }
    }
    auto &owned = owned_out;
    for (int i = 0; i < owned.size(); i++) {
        priority_queue<int2> neighbors;
        vector<int> connect(n);
        vector<bool> visited(n);
        unordered_set<int> total;
        for (int i = 0; i < n; i++) {
            connect[i] = 0;
            visited[i] = false;
        }
        for (int u = 0; u < n; u++) {
            if (color[u] != i) continue;
            total.insert(u);
            for (auto v: graph[u]) {
                total.insert(v);
                connect[v]++;
                neighbors.push({connect[v], v});
            }
        }
        if (total.size() > patch_size) continue;
        while (!neighbors.empty()) {
            int u = neighbors.top()[1], k = neighbors.top()[0];
            neighbors.pop();
            if (color[u] == i || visited[u]) continue;
            visited[u] = true;
            int sum = total.size();
            for (auto v: graph[u]) {
                if (total.find(v) == total.end()) {
                    sum++;
                    if (sum > patch_size) break;
                }
            }
            if (sum > patch_size) continue;
            color[u] = i;
            for (auto v: graph[u]) {
                total.insert(v);
                connect[v]++;
                neighbors.push({connect[v], v});
            }
        }
    }
    {
        vector<array<int, 2> > pairs;
        for (int u = 0; u < n; u++) {
            pairs.push_back({color[u], u});
        }
        owned = Csr(pairs);
    }
    {
        int color_n = owned.size();
        vector<int> connect(n);
        for (int i = 0; i < owned.size(); i++) {
            int c = -1;
            for (auto u: owned[i]) {
                color[u] = -1;
            }
            while (true) {
                priority_queue<int2> neighbors;
                for (auto u: owned[i]) {
                    if (color[u] < 0) {
                        neighbors.push({0, u});
                        color[u] = -1;
                        connect[u] = 0;
                    }
                }
                if (neighbors.empty()) {
                    break;
                }
                if (c == -1) {
                    c = i;
                }
                else {
                    c = color_n;
                    color_n++;
                }
                unordered_set<int> total;
                while (!neighbors.empty()) {
                    int u = neighbors.top()[1], k = neighbors.top()[0];
                    neighbors.pop();
                    if (color[u] != -1) continue;
                    int sum = total.size();
                    for (auto v: graph[u]) {
                        if (total.find(v) == total.end()) {
                            sum++;
                            if (sum > patch_size) break;
                        }
                    }
                    if (sum > patch_size) {
                        color[u] = -2; // not appear again in this patch
                        continue;
                    }
                    color[u] = c;
                    for (auto v: graph[u]) {
                        total.insert(v);
                        if (color[v] == -1) {
                            connect[v]++;
                            neighbors.push({connect[v], v});
                        }
                    }
                }
            }
        }
    }
    {
        vector<array<int, 2> > pairs;
        for (int u = 0; u < n; u++) {
            pairs.push_back({color[u], u});
        }
        return Csr(pairs);
    }
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
    int round_n = 5;
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
        // printf("cnt = %d\n", cnt);
        if (ma <= patch_size) break;

        // update seeds
        std::vector<bool> visit(n);
        for (int i = 0; i < n; i++) {
            visit[i] = false;
        }
        for (int p = 0; p < owned.size(); p++) {
            // if (total[p].size() <= patch_size) continue;
            // if (cnt < patch_size / 5 && round % round_n == 0 && total[p].size() <= patch_size) {
            //     locked[p] = true;
            // }
            // if (locked[p]) continue;
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
    if (false) {// greedy after kmeans
        vector<array<int, 2> > pairs;
        for (int u = 0; u < n; u++) {
            pairs.push_back({color[u], u});
        }
        auto owned = Csr(pairs);
        auto total_tmp = owned.mul_unique(graph);
        vector<array<int, 2>> patches;
        for (int i = 0; i < total_tmp.size(); i++) {
            patches.push_back({total_tmp[i].size(), i});
        }
        sort(patches.begin(), patches.end());
        vector<bool> colored(n);
        for (int i = 0; i < n; i++) {
            colored[i] = false;
        }
        typedef array<int, 2> int2;
        for (auto p: patches) {
            int i = p[1];
            priority_queue<int2> neighbors;
            vector<int> connect(n);
            vector<bool> visited(n);
            unordered_set<int> total;
            for (int i = 0; i < n; i++) {
                connect[i] = 0;
                visited[i] = false;
            }
            for (int u = 0; u < n; u++) {
                if (color[u] != i) continue;
                total.insert(u);
                colored[u] = true;
                for (auto v: graph[u]) {
                    total.insert(v);
                    connect[v]++;
                    neighbors.push({connect[v], v});
                }
            }
            while (!neighbors.empty()) {
                int u = neighbors.top()[1], k = neighbors.top()[0];
                neighbors.pop();
                if (color[u] == i || visited[u] || colored[u]) continue;
                visited[u] = true;
                int sum = total.size();
                for (auto v: graph[u]) {
                    if (total.find(v) == total.end()) {
                        sum++;
                        if (sum > patch_size) break;
                    }
                }
                if (sum > patch_size) continue;
                color[u] = i;
                colored[u] = true;
                for (auto v: graph[u]) {
                    total.insert(v);
                    connect[v]++;
                    neighbors.push({connect[v], v});
                }
            }
        }
    }
    return color2ans(color, graph);
}

Csr Cluster::run_greedy_cv(Csr &graph) {
    using namespace std;
    assert(patch_size != -1); // patch_size has to be set before run cluster
    auto graph_inv = graph.transpose();
    typedef array<int, 2> int2;
    int n = graph.size();
    priority_queue<int2, vector<int2>> seeds;
    vector<int> color(n), degree(n);
    for (int i = 0; i < n; i++) {
        color[i] = -1;
        // degree[i] = graph[i].size();
        degree[i] = 0;
        seeds.push({degree[i], i});
    }
    vector<int> connect(n), visited(n), connect_tag(n), total_size;
    for (int i = 0; i < n; i++) {
        connect_tag[i] = -1;
        visited[i] = -1;
    }
    int color_n = 0;
    while (!seeds.empty()) {
        int seed = seeds.top()[1];
        seeds.pop();
        if (color[seed] != -1) continue;
        priority_queue<int2> neighbors;
        unordered_set<int> total, owned;
        neighbors.push({0, seed});
        while (!neighbors.empty()) {
            int u = neighbors.top()[1], k = neighbors.top()[0];
            neighbors.pop();
            if (color[u] == color_n || visited[u] == color_n) continue;
            visited[u] = color_n;
            unordered_set<int> tmp_total;
            int sum = total.size();
            for (auto w: graph[u]) {
                if (owned.find(w) != owned.end()) continue;
                for (auto v: graph_inv[w]) {
                    if (total.find(v) == total.end() && tmp_total.find(v) == tmp_total.end()) {
                        tmp_total.insert(v);
                        sum++;
                    }
                    if (sum > patch_size) break;
                }
                if (sum > patch_size) break;
            }
            if (sum > patch_size) continue;
            color[u] = color_n;
            for (auto w: graph[u]) {
                // if (owned.find(w) != owned.end()) continue;
                owned.insert(w);
                for (auto v: graph_inv[w]) {
                    total.insert(v);
                    if (color[v] == -1) {
                        if (connect_tag[v] != color_n) {
                            connect_tag[v] = color_n;
                            connect[v] = 0;
                        }
                        connect[v]++;
                        neighbors.push({connect[v], v});
                        // degree[v]--;
                        degree[v]++;
                        seeds.push({degree[v], v});
                    }
                }
            }
        }
        total_size.push_back(total.size());
        color_n++;
    }
    return color2ans_cv(color, graph, graph_inv, total_size);
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
        // degree[i] = graph[i].size();
        degree[i] = 0;
        seeds.push({degree[i], i});
    }
    vector<int> connect(n), visited(n), connect_tag(n);
    for (int i = 0; i < n; i++) {
        connect_tag[i] = -1;
        visited[i] = -1;
    }
    int color_n = 0;
    // for (int seed = 0; seed < n; seed++) {
    while (!seeds.empty()) {
        int seed = seeds.top()[1];
        seeds.pop();
        if (color[seed] != -1) continue;
        priority_queue<int2> neighbors;
        unordered_set<int> total;
        neighbors.push({0, seed});
        total.insert(seed);
        while (!neighbors.empty()) {
            int u = neighbors.top()[1], k = neighbors.top()[0];
            neighbors.pop();
            if (color[u] == color_n || visited[u] == color_n) continue;
            visited[u] = color_n;
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
                    if (connect_tag[v] != color_n) {
                        connect_tag[v] = color_n;
                        connect[v] = 0;
                    }
                    connect[v]++;
                    neighbors.push({connect[v], v});
                    // degree[v]--;
                    degree[v]++;
                    seeds.push({degree[v], v});
                }
            }
        }
        color_n++;
    }
    // for (int i = 0; i < color_n; i++) {
    //     priority_queue<int2> neighbors;
    //     vector<int> connect(n);
    //     vector<bool> visited(n);
    //     unordered_set<int> total;
    //     for (int i = 0; i < n; i++) {
    //         connect[i] = 0;
    //         visited[i] = false;
    //     }
    //     for (int u = 0; u < n; u++) {
    //         if (color[u] != i) continue;
    //         total.insert(u);
    //         for (auto v: graph[u]) {
    //             total.insert(v);
    //             connect[v]++;
    //             neighbors.push({connect[v], v});
    //         }
    //     }
    //     while (!neighbors.empty()) {
    //         int u = neighbors.top()[1], k = neighbors.top()[0];
    //         neighbors.pop();
    //         if (color[u] == i || visited[u]) continue;
    //         visited[u] = true;
    //         int sum = total.size();
    //         for (auto v: graph[u]) {
    //             if (total.find(v) == total.end()) {
    //                 sum++;
    //                 if (sum > patch_size) break;
    //             }
    //         }
    //         if (sum > patch_size) continue;
    //         color[u] = i;
    //         for (auto v: graph[u]) {
    //             total.insert(v);
    //             connect[v]++;
    //             neighbors.push({connect[v], v});
    //         }
    //     }
    // }
    return color2ans(color, graph);
}