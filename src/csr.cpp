#include "csr.h"
#include <vector>

Csr::Csr(py::array_t<int> &_offset, py::array_t<int> &_value): offset(_offset), value(_value) {}

Csr::Csr(std::vector<int> &_offset, std::vector<int> &_value): offset(py::array_t<int>(_offset.size(), _offset.data())), value(py::array_t<int>(_value.size(), _value.data())) {}

Csr::Csr(std::vector<std::array<int, 2>> pairs) {
    sort(pairs.begin(), pairs.end());
    std::vector<int> off_new, val_new;
    for (int i = 0; i < pairs.size(); i++) {
        while (pairs[i][0] >= off_new.size()) {
            off_new.push_back(i);
        }
        val_new.push_back(pairs[i][1]);
    }
    off_new.push_back(pairs.size());
    offset = py::array_t<int>(off_new.size(), off_new.data());
    value = py::array_t<int>(val_new.size(), val_new.data());
}

Csr::Range::Range(int* _b, int* _e): b(_b), e(_e) {}

int* Csr::Range::begin() {
    return b;
}

int* Csr::Range::end() {
    return e;
}

int Csr::Range::size() {
    return e - b;
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