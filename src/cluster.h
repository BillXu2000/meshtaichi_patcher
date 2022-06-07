#pragma once
#include "csr.h"

struct Cluster {
    int patch_size = -1;
    std::string option;

    Cluster() {}
    Csr run(Csr &graph);
    Csr run_greedy(Csr &graph);
    Csr run_greedy_cv(Csr &graph);
    Csr run_kmeans(Csr &graph);
    Csr run_unbound(Csr &graph);

    Csr color2ans(std::vector<int>&, Csr&);
    Csr color2ans_cv(std::vector<int>&, Csr&, Csr&, std::vector<int>&);
};