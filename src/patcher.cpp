#include "patcher.h"
#include <fstream>
#include <iostream>
#include <chrono>

int Patcher::get_size(int order) {
    return order ? relation[{order, 0}].size() : relation[{0, n_order - 1}].size();
}

void Patcher::set_relation(int from_end, int to_end, Csr &rel) {
    if (relation.find({from_end, to_end}) == relation.end()) {
        relation[{from_end, to_end}] = rel;
    }
}

namespace std {
    template<> struct hash<vector<int> > {
        std::size_t operator()(const vector<int> &f) const {
            std::size_t sum = 0;
            for (auto i: f) {
                sum = sum * 998244353 + i;
            }
            return sum;
        }  
    };
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
    unordered_map<vector<int>, int> ele_dict;
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
            if (subset.size() + k.size() - i < to_end + 1) return;
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

namespace {
    std::map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> times;
}

void Patcher::start_timer(std::string name) {
    if (debug) {
        times[name] = std::chrono::high_resolution_clock::now();
    }
}

void Patcher::print_timer(std::string name) {
    if (debug) {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - times[name];
        std::cout << name << " time: " << diff.count() << "\n";
    }
}

void Patcher::generate_elements() {
    start_timer("generate elements");
    using namespace std;
    // auto &rel = get_relation(0, n_order - 1);
    get_relation(0, n_order - 1);
    auto &rel = relation[{n_order - 1, 0}];
    for (int order = 1; order < n_order; order++) {
        if (relation.find({order, 0}) != relation.end()) {
            continue;
        }
        vector<int> cb;
        function<void(int, vector<int>)> fun = [&](int i, vector<int> ans) {
            if (ans.size() == order + 1) {
                cb.insert(cb.end(), ans.begin(), ans.end());
                return;
            }
            if (i == n_order) return;
            fun(i + 1, ans);
            ans.push_back(i);
            fun(i + 1, ans);
        };
        fun(0, {});
        vector<int> offset, value;
        vector<int> offset_ce, value_ce;
        // set<vector<int> > s;
        unordered_map<vector<int>, int> s;
        int m = 0;
        offset.push_back(0);
        offset_ce.push_back(0);
        for (int u = 0; u < rel.size(); u++) {
            vector<int> subset, k(rel[u].begin(), rel[u].end());
            sort(k.begin(), k.end());
            // function<void(int)> fun = [&](int i) {
            //     if (subset.size() == order + 1) {
            //         auto iter = s.find(subset);
            //         if (iter == s.end()) {
            //             value.insert(value.end(), subset.begin(), subset.end());
            //             offset.push_back(value.size());
            //             // s.insert(subset);
            //             // iter = s.insert(make_pair(subset, m));
            //             s.insert(make_pair(subset, m));
            //             m++;
            //         }
            //         // value_ce.push_back(iter->second);
            //         value_ce.push_back(s[subset]);
            //         return;
            //     }
            //     if (subset.size() + k.size() - i < order + 1 || subset.size() >= order + 1) return;
            //     fun(i + 1);
            //     subset.push_back(k[i]);
            //     fun(i + 1);
            //     subset.erase(subset.end() - 1);
            // };
            // fun(0);
            for (int i = 0; i < cb.size(); i += order + 1) {
                vector<int> subset(order + 1);
                for (int j = 0; j <= order; j++) {
                    subset[j] = k[cb[i + j]];
                }
                auto iter = s.find(subset);
                if (iter == s.end()) {
                    value.insert(value.end(), subset.begin(), subset.end());
                    offset.push_back(value.size());
                    s.insert(make_pair(subset, m));
                    m++;
                }
                value_ce.push_back(s[subset]);
            }
            offset_ce.push_back(value_ce.size());
        }
        relation[{order, 0}] = Csr(offset, value);
        relation[{n_order - 1, order}] = Csr(offset_ce, value_ce);
    }
    print_timer("generate elements");
}

void Patcher::patch() {
    start_timer("cpp patch");
    start_timer("cluster");
    using namespace std;
    // auto rel_cluster = get_relation(n_order - 1, 0).mul_unique(get_relation(0, n_order - 1)).remove_self_loop();
    auto rel_cluster = get_relation(n_order - 1, 0);
    auto cluster = Cluster();
    cluster.patch_size = patch_size;
    cluster.option = cluster_option;
    auto patch = cluster.run(rel_cluster);
    print_timer("cluster");
    start_timer("owned");
    for (int order = 0; order < n_order - 1; order++) {
        start_timer("relation order " + to_string(order));
        get_relation(n_order - 1, order);
        print_timer("relation order " + to_string(order));
    }
    for (int order = 0; order < n_order - 1; order++) {
        auto &rel = get_relation(n_order - 1, order);
        vector<int> color(get_size(order));
        int pro_cnt = 0;
        // start_timer(to_string(order) + " order");
        for (int p = 0; p < patch.size(); p++) {
            for (auto u: patch[p]) {
                for (auto v: rel[u]) {
                    color[v] = p;
                    pro_cnt++;
                }
            }
        }
        // print_timer(to_string(order) + " order");
        // vector<array<int, 2> > pairs;
        // for (int i = 0; i < color.size(); i++) {
        //     pairs.push_back({color[i], i});
        // }
        // owned[order] = Csr(pairs);
        owned[order] = Csr::from_color(color);
    }
    owned[n_order - 1] = patch;
    // Csr verts = patch.mul_unique(get_relation(n_order - 1, 0));
    if (all_patch_relation) {
        patch_relation.clear();
        for (int order = 0; order < n_order - 1; order++) {
            patch_relation.insert({n_order - 1, order});
        }
        patch_relation.insert({0, n_order - 1});
    }
    print_timer("owned");
    start_timer("ribbon");
    for (int order = n_order - 1; order >= 0; order--) {
        std::vector<int> off_new, val_new;
        off_new.push_back(0);
        vector<int> s(get_size(order));
        for (auto &i: s) {
            i = -1;
        }
        for (int p = 0; p < patch.size(); p++) {
            for (auto u: owned[order][p]) {
                s[u] = p;
                val_new.push_back(u);
            }
            auto add = [&](int w) {
                if (s[w] < p) {
                    s[w] = p;
                    val_new.push_back(w);
                }
            };
            auto char2order = [](char c) {
                if (c == 'v') return 0;
                if (c == 'e') return 1;
                if (c == 'f') return 2;
                if (c == 'c') return 3;
                return -1;
            };
            if (false) { // use optimized ribbon now
                // for (auto v: verts[p]) {
                //     auto &rel = get_relation(0, order);
                //     for (auto w: rel[v]) {
                //         add(w);
                //     }
                // }
            }
            else {
                for (auto u: patch[p]) {
                    auto &rel = get_relation(n_order - 1, order);
                    for (auto v: rel[u]) {
                        add(v);
                    }
                }
                for (auto i: patch_relation) {
                    if (i[1] != order) continue;
                    auto &rel = get_relation(i[0], i[1]);
                    if (i[0] <= i[1]) {
                        for (auto u: owned[i[0]][p]) {
                            for (auto v: rel[u]) {
                                add(v);
                            }
                        }
                    }
                    else {
                        for (auto u: total[i[0]][p]) {
                            for (auto v: rel[u]) {
                                add(v);
                            }
                        }
                    }
                }
            }
            off_new.push_back(val_new.size());
        }
        total[order] = Csr(off_new, val_new);
    }
    print_timer("ribbon");
    print_timer("cpp patch");
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
    auto &rel = get_relation(from_end, to_end);
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

void Patcher::write(std::string filename) {
    using namespace std;
    fstream out(filename, fstream::out | fstream::binary);
    auto write_int = [&](int i) {
        out.write(reinterpret_cast<char*>(&i), sizeof(i));
    };
    auto write_float = [&](float i) {
        out.write(reinterpret_cast<char*>(&i), sizeof(i));
    };
    auto write_vector = [&](vector<int> &arr) {
        write_int(arr.size());
        for (auto i: arr) {
            write_int(i);
        }
    };
    namespace py = pybind11;
    auto write_np = [&](py::array_t<int> &x) {
        auto arr = x.mutable_unchecked<1>();
        write_int(int(arr.shape(0)));
        for (py::ssize_t i = 0; i < arr.shape(0); i++) {
            write_int(arr(i));
        }
    };
    auto write_csr = [&](Csr &csr) {
        write_np(csr.offset);
        write_np(csr.value);
    };
    typedef array<int, 2> int2;
    auto write_map_int2_csr = [&](map<int2, Csr> &m) {
        write_int(m.size());
        for (auto &i: m) {
            write_int(i.first[0]);
            write_int(i.first[1]);
            write_csr(i.second);
        }
    };
    auto write_map_int_csr = [&](map<int, Csr> &m) {
        write_int(m.size());
        for (auto &i: m) {
            write_int(i.first);
            write_csr(i.second);
        }
    };
    auto write_map_int2_vec = [&](map<int2, vector<int>> &m) {
        write_int(m.size());
        for (auto &i: m) {
            write_int(i.first[0]);
            write_int(i.first[1]);
            write_vector(i.second);
        }
    };
    write_map_int2_csr(relation);
    write_map_int2_csr(relation_meta);
    write_map_int_csr(owned);
    write_map_int_csr(total);
    write_map_int2_vec(patch_offset);
    write_int(n_order);
    write_int(patch_size);
    {
        auto arr = position.mutable_unchecked<1>();
        write_int(int(arr.shape(0)));
        for (py::ssize_t i = 0; i < arr.shape(0); i++) {
            write_float(arr(i));
        }
    }
}

void Patcher::read(std::string filename) {
    using namespace std;
    fstream in(filename, fstream::in | fstream::binary);
    auto read_int = [&]() {
        int i;
        in.read(reinterpret_cast<char*>(&i), sizeof(i));
        return i;
    };
    auto read_float = [&]() {
        float i;
        in.read(reinterpret_cast<char*>(&i), sizeof(i));
        return i;
    };
    auto read_vector = [&]() {
        int n = read_int();
        vector<int> ans(n);
        for (auto &i: ans) {
            i = read_int();
        }
        return ans;
    };
    auto read_csr = [&]() {
        auto off = read_vector();
        auto val = read_vector();
        return Csr(off, val);
    };
    typedef array<int, 2> int2;
    auto read_map_int2_csr = [&]() {
        map<int2, Csr> m;
        int n = read_int();
        for (int i = 0; i < n; i++) {
            auto i0 = read_int();
            auto i1 = read_int();
            auto csr = read_csr();
            m[{i0, i1}] = csr;
        }
        return m;
    };
    auto read_map_int_csr = [&]() {
        map<int, Csr> m;
        int n = read_int();
        for (int i = 0; i < n; i++) {
            auto i0 = read_int();
            auto csr = read_csr();
            m[i0] = csr;
        }
        return m;
    };
    auto read_map_int2_vec = [&]() {
        map<int2, vector<int>> m;
        int n = read_int();
        for (int i = 0; i < n; i++) {
            auto i0 = read_int();
            auto i1 = read_int();
            auto vec = read_vector();
            m[{i0, i1}] = vec;
        }
        return m;
    };
    relation = read_map_int2_csr();
    relation_meta = read_map_int2_csr();
    owned = read_map_int_csr();
    total = read_map_int_csr();
    patch_offset = read_map_int2_vec();
    n_order = read_int();
    patch_size = read_int();
    namespace py = pybind11;
    {
        int n = read_int();
        vector<float> ans(n);
        for (auto &i: ans) {
            i = read_float();
        }
        position = py::array_t<float>(ans.size(), ans.data());
    }
}

Csr& Patcher::get_face() {
    auto i = relation.find({2, -1});
    if (i == relation.end()) {
        relation[{2, -1}] = Csr();
    }
    return relation[{2, -1}];
}

void Patcher::set_pos(pybind11::array_t<float> arr) {
    position = arr;
}

pybind11::array_t<float> Patcher::get_pos() {
    return position;
}

void Patcher::add_patch_relation(int u, int v) {
    patch_relation.insert({u, v});
}

void Patcher::add_all_patch_relation() {
    all_patch_relation = true;
}

pybind11::list Patcher::get_mapping(int order) {
    pybind11::list ans;
    auto &owned_tmp = owned[order].value;
    auto &total_tmp = total[order].value;
    auto own = owned_tmp.mutable_unchecked<1>();
    auto tot = total_tmp.mutable_unchecked<1>();
    std::vector<int> g2r(own.shape(0)), l2r(tot.shape(0));
    for (py::ssize_t i = 0; i < own.shape(0); i++) {
        g2r[own(i)] = i;
    }
    for (py::ssize_t i = 0; i < tot.shape(0); i++) {
        l2r[i] = g2r[tot(i)];
    }
    ans.append(pybind11::array_t<int>(g2r.size(), g2r.data()));
    ans.append(pybind11::array_t<int>(l2r.size(), l2r.data()));
    return ans;
}