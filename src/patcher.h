#pragma once
#include <map>
#include "csr.h"

struct Patcher {
    std::map<std::array<int, 2>, Csr> relation, relation_meta;
    std::map<int, Csr> owned, total;
    std::map<std::array<int, 2>, std::vector<int>> patch_offset;
    int n_order;

    Patcher() {}
    int get_size(int order);
    void set_relation(int from_end, int to_end, Csr &rel);
    Csr &get_relation(int from_end, int to_end);
    void generate_elements();
    void patch(Csr &patch);
    Csr &get_owned(int order);
    Csr &get_total(int order);
    Csr &get_relation_meta(int from_end, int to_end);
    py::array_t<int> get_patch_offset(int from_end, int to_end);
};