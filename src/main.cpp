#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "patcher_api.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

int add(int i, int j) {
    return i + j;
}

namespace py = pybind11;

PYBIND11_MODULE(meshtaichi_patcher_core, m) {
    m.doc() = R"pbdoc(
        Pybind11 example plugin
        -----------------------

        .. currentmodule:: meshtaichi_patcher

        .. autosummary::
           :toctree: _generate

           add
           subtract
    )pbdoc";

    m.def("add", &add, R"pbdoc(
        Add two numbers

        Some other explanation about the add function.
    )pbdoc");

    m.def("subtract", [](int i, int j) { return i - j; }, R"pbdoc(
        Subtract two numbers

        Some other explanation about the subtract function.
    )pbdoc");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif

    m.def("run_mesh", &MeshTaichi::run_mesh);
    py::class_<MeshTaichi::Patcher>(m, "Patcher")
        .def(py::init<>())
        .def("run", [](MeshTaichi::Patcher *patcher, std::string mesh_name, std::vector<std::string> relations) {
            std::map<char, int> name2dim;
            name2dim['v'] = 0;
            name2dim['e'] = 1;
            name2dim['f'] = 2;
            name2dim['c'] = 3;
            using namespace MeshTaichi;
            using RT = MeshRelationType;
            using ET = MeshElementType;
            auto mesh = Mesh::load_mesh(mesh_name);
            std::vector<RT> rt_arr;
            std::vector<ET> et_arr;
            for (std::string str : relations) {
                rt_arr.push_back(RT(name2dim[str[0]] * 4 + name2dim[str[1]]));
                et_arr.push_back(ET(name2dim[str[0]]));
                et_arr.push_back(ET(name2dim[str[1]]));
            }
            int patch_size = 256;
            patcher->run(mesh.get(), patch_size, et_arr, rt_arr);
        });
}
