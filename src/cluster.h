#pragma once
#include "csr.h"

struct Cluster {
    int patch_size = 256;

    Cluster() {}
    Csr run(Csr &graph);
};