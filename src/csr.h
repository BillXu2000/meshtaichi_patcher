#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <vector>

namespace py = pybind11;


struct Csr {
    py::array_t<int> offset, value;

    Csr(py::array_t<int> &_offset, py::array_t<int> &_value);

    Csr(std::vector<int> &_offset, std::vector<int> &_value);

    Csr(std::vector<std::array<int, 2>> pairs);

    Csr() {}


    struct Range {
        int *b, *e;
        Range(int* _b, int* _e);
        int* begin();
        int* end();
        int size();
        int operator [](int i);
    };

    Range operator[] (int i);

    int size();
    Csr transpose();
    Csr mul(Csr&);
    Csr remove_self_loop();
    void print();
};