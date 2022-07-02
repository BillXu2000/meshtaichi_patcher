#include "csr.h"
#include <vector>

Csr::Csr(py::array_t<int> &_offset, py::array_t<int> &_value): offset(_offset), value(_value) {}

Csr::Csr(std::vector<int> &_offset, std::vector<int> &_value): offset(py::array_t<int>(_offset.size(), _offset.data())), value(py::array_t<int>(_value.size(), _value.data())) {}

Csr::Csr(std::vector<std::array<int, 2>> pairs) {
    // sort(pairs.begin(), pairs.end());
    // std::vector<int> off_new, val_new;
    // for (int i = 0; i < pairs.size(); i++) {
    //     while (pairs[i][0] >= off_new.size()) {
    //         off_new.push_back(i);
    //     }
    //     val_new.push_back(pairs[i][1]);
    // }
    // off_new.push_back(pairs.size());
    // offset = py::array_t<int>(off_new.size(), off_new.data());
    // value = py::array_t<int>(val_new.size(), val_new.data());
    using namespace std;
    int m = 0;
    for (auto i: pairs) {
        m = max(i[0] + 1, m);
    }
    vector<int> cnt(m), off(m + 1), val(pairs.size());
    for (auto i: pairs) {
        cnt[i[0]]++;
    }
    off[0] = off[1] = 0;
    for (int i = 0; i < m - 1; i++) {
        off[i + 2] = off[i + 1] + cnt[i];
        // printf("i = %d off = %d\n", i, off[i]);
    }
    for (auto i: pairs) {
        val[off[i[0] + 1]] = i[1];
        off[i[0] + 1]++;
    }
    offset = py::array_t<int>(off.size(), off.data());
    value = py::array_t<int>(val.size(), val.data());
}

Csr Csr::from_numpy(py::array_t<int> &values) {
    using namespace std;
    auto arr = values.mutable_unchecked<2>();
    int n = arr.shape(0), m = arr.shape(1);
    vector<int> off(n + 1), val(n * m);
    for (int i = 0; i < n; i++) {
        off[i] = i * m;
        for (int j = 0; j < m; j++) {
            val[i * m + j] = arr(i, j);
        }
    }
    off[n] = n * m;
    return Csr(off, val);
}

Csr::Range::Range(int* _b, int* _e): b(_b), e(_e) {}

Csr Csr::from_color(std::vector<int> &c) {
    using namespace std;
    int m = 0;
    for (auto i: c) {
        m = max(i + 1, m);
    }
    vector<int> cnt(m), off(m + 1), val(c.size());
    for (auto i: c) {
        cnt[i]++;
    }
    off[0] = off[1] = 0;
    for (int i = 0; i < m - 1; i++) {
        off[i + 2] = off[i + 1] + cnt[i];
        // printf("i = %d off = %d\n", i, off[i]);
    }
    for (int i = 0; i < c.size(); i++) {
        val[off[c[i] + 1]] = i;
        off[c[i] + 1]++;
    }
    // offset = py::array_t<int>(off.size(), off.data());
    // value = py::array_t<int>(val.size(), val.data());
    // std::vector<std::array<int, 2> > pairs;
    // for (int u = 0; u < c.size(); u++) {
    //     pairs.push_back({c[u], u});
    // }
    // return Csr(pairs);
    return Csr(off, val);
}

int* Csr::Range::begin() {
    return b;
}

int* Csr::Range::end() {
    return e;
}

int Csr::Range::size() {
    return e - b;
}

int Csr::Range::operator [] (int i) {
    return b[i];
}

Csr::Range Csr::operator [] (int i) {
    return Range(value.mutable_data(0) + *offset.data(i), value.mutable_data(0) + *offset.data(i + 1));
}

int Csr::size() {
    return offset.shape(0) - 1;
}

Csr Csr::transpose() {
    using namespace std;
    vector<array<int, 2> > pairs;
    for (int i = 0; i < size(); i++) {
        for (auto j : (*this)[i]) {
            pairs.push_back({j, i});
        }
    }
    return Csr(pairs);
}

Csr Csr::mul(Csr &b) {
    using namespace std;
    vector<int> off_new, val_new;
    off_new.push_back(0);
    for (int u = 0; u < size(); u++) {
        for (auto v: (*this)[u]) {
            for (auto w: b[v]) {
                val_new.push_back(w);
            }
        }
        off_new.push_back(val_new.size());
    }
    return Csr(off_new, val_new);
}

Csr Csr::mul_unique(Csr &b) {
    using namespace std;
    vector<int> off_new, val_new;
    off_new.push_back(0);
    for (int u = 0; u < size(); u++) {
        unordered_set<int> s;
        for (auto v: (*this)[u]) {
            for (auto w: b[v]) {
                if (s.find(w) == s.end()) {
                    val_new.push_back(w);
                    s.insert(w);
                }
            }
        }
        off_new.push_back(val_new.size());
    }
    return Csr(off_new, val_new);
}

Csr Csr::remove_self_loop() {
    using namespace std;
    vector<int> off_new, val_new;
    off_new.push_back(0);
    for (int u = 0; u < size(); u++) {
        for (auto v: (*this)[u]) {
            if (u != v) {
                val_new.push_back(v);
            }
        }
        off_new.push_back(val_new.size());
    }
    return Csr(off_new, val_new);
}

void Csr::print() {
    for (int u = 0; u < size(); u++) {
        for (auto v: (*this)[u]) {
            printf("(%d, %d) ", u, v);
        }
        printf("\n");
    }
}