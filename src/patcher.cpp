#include "patcher.h"

int Patcher::get_size(int order) {
    return order ? relation[{order, 0}].size() : relation[{0, n_order - 1}].size();
}

void Patcher::set_relation(int from_end, int to_end, Csr &rel) {
    if (relation.find({from_end, to_end}) == relation.end()) {
        relation[{from_end, to_end}] = rel;
    }
}

Csr& Patcher::get_relation(int from_end, int to_end) {
    using namespace std;
    array<int, 2> key{from_end, to_end};
    if (relation.find(key) != relation.end()) {
        return relation[key];
    }
    if (key[0] < key[1]) {
        relation[key] = get_relation(key[1], key[0]).transpose();
        return relation[key];
    }
    if (key[0] == key[1] && key[0] == 0) {
        relation[key] = get_relation(0, 1).mul(get_relation(1, 0)).remove_self_loop();
        return relation[key];
    }
    if (key[0] == key[1]) {
        relation[key] = get_relation(key[0], key[0] - 1).mul(get_relation(key[0] - 1, key[0])).remove_self_loop();
        return relation[key];
    }
    auto &to = relation[{to_end, 0}];
    map<vector<int>, int> ele_dict;
    for (int i = 0; i < to.size(); i++) {
        vector<int> k(to[i].begin(), to[i].end());
        sort(k.begin(), k.end());
        ele_dict[k] = i;
    }
    vector<int> offset, value;
    auto &from = relation[{from_end, 0}];
    offset.push_back(0);
    for (int u = 0; u < from.size(); u++) {
        vector<int> subset, k(from[u].begin(), from[u].end());
        sort(k.begin(), k.end());
        function<void(int)> fun = [&](int i) {
            if (subset.size() == to_end + 1) {
                value.push_back(ele_dict[subset]);
                return;
            }
            if (i >= k.size()) return;
            fun(i + 1);
            subset.push_back(k[i]);
            fun(i + 1);
            subset.erase(subset.end() - 1);
        };
        fun(0);
        offset.push_back(value.size());
    }
    relation[key] = Csr(offset, value);
    return relation[key];
}

void Patcher::generate_elements() {
    using namespace std;
    get_relation(0, n_order - 1);
    for (int order = 1; order < n_order; order++) {
        if (relation.find({order, 0}) != relation.end()) {
            continue;
        }
        auto &rel = relation[{n_order - 1, 0}];
        vector<int> offset, value;
        set<vector<int> > s;
        offset.push_back(0);
        for (int u = 0; u < rel.size(); u++) {
            vector<int> subset, k(rel[u].begin(), rel[u].end());
            sort(k.begin(), k.end());
            function<void(int)> fun = [&](int i) {
                if (subset.size() == order + 1 && s.find(subset) == s.end()) {
                    value.insert(value.end(), subset.begin(), subset.end());
                    offset.push_back(value.size());
                    s.insert(subset);
                    return;
                }
                if (i >= k.size()) return;
                fun(i + 1);
                subset.push_back(k[i]);
                fun(i + 1);
                subset.erase(subset.end() - 1);
            };
            fun(0);
        }
        relation[{order, 0}] = Csr(offset, value);
    }
}

void Patcher::patch(Csr &patch) {
    using namespace std;
    for (int order = 0; order < n_order - 1; order++) {
        auto &rel = get_relation(n_order - 1, order);
        vector<int> color(get_size(order));
        for (int p = 0; p < patch.size(); p++) {
            for (auto u: patch[p]) {
                for (auto v: rel[u]) {
                    color[v] = p;
                }
            }
        }
        vector<array<int, 2> > pairs;
        for (int i = 0; i < color.size(); i++) {
            pairs.push_back({color[i], i});
        }
        owned[order] = Csr(pairs);
    }
    owned[n_order - 1] = patch;
    for (int order = 0; order < n_order; order++) {
        std::vector<int> off_new, val_new;
        off_new.push_back(0);
        for (int p = 0; p < patch.size(); p++) {
            auto &rel_0 = get_relation(n_order - 1, 0);
            auto &rel_1 = get_relation(0, order);
            set<int> s;
            for (auto u: owned[order][p]) {
                s.insert(u);
                val_new.push_back(u);
            }
            for (auto u: patch[p]) {
                for (auto v: rel_0[u]) {
                    for (auto w: rel_1[v]) {
                        if (s.find(w) == s.end()) {
                            s.insert(w);
                            val_new.push_back(w);
                        }
                    }
                }
            }
            off_new.push_back(val_new.size());
        }
        total[order] = Csr(off_new, val_new);
    }
}

Csr& Patcher::get_owned(int order) {
    return owned[order];
}

Csr& Patcher::get_total(int order) {
    return total[order];
}

Csr &Patcher::get_relation_meta(int from_end, int to_end) {
    using namespace std;
    if (relation_meta.find({from_end, to_end}) != relation_meta.end()) {
        return relation_meta[{from_end, to_end}];
    }
    vector<int> d(get_size(to_end));
    auto &from = from_end > to_end ? total[from_end] : owned[from_end];
    auto &to = total[to_end];
    auto &rel = relation[{from_end, to_end}];
    vector<int> offset, value, patch_off;
    for (int p = 0; p < from.size(); p++) {
        for (int j = 0; j < to[p].size(); j++) {
            d[to[p][j]] = j;
        }
        offset.push_back(0);
        patch_off.push_back(value.size());
        int base = value.size();
        for (auto u: from[p]) {
            for (auto v: rel[u]) {
                value.push_back(d[v]);
            }
            offset.push_back(value.size() - base);
        }
    }
    relation_meta[{from_end, to_end}] = Csr(offset, value);
    patch_offset[{from_end, to_end}] = patch_off;
    return relation_meta[{from_end, to_end}];
}

py::array_t<int> Patcher::get_patch_offset(int from_end, int to_end) {
    auto &p = patch_offset[{from_end, to_end}];
    return py::array_t<int>(p.size(), p.data());
}