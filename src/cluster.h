#pragma once
#include "csr.h"

struct Cluster {
    int patch_size = -1;

    Cluster() {}
    Csr run(Csr &graph);
    Csr run_greedy(Csr &graph);
};