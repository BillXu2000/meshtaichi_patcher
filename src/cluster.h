#pragma once
#include "csr.h"

struct Cluster {
    int patch_size = -1;
    std::string option;

    Cluster() {}
    Csr run(Csr &graph);
    Csr run_greedy(Csr &graph);
    Csr run_kmeans(Csr &graph);
    Csr run_unbound(Csr &graph);
};