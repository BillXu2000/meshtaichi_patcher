#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <vector>
#include <chrono>
#include "patcher_api.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

int add(int i, int j) {
    return i + j;
}

namespace py = pybind11;

template<class T>
py::array_t<T> vector2np(const std::vector<T> &a) {
    return py::array_t<T>(a.size(), a.data());
}

template <typename value_type>
inline py::array_t<value_type> as_pyarray(std::vector<value_type>&& seq) {
    using Sequence = std::vector<value_type>;
    // Move entire object to heap (Ensure is moveable!). Memory handled via Python capsule
    Sequence* seq_ptr = new Sequence(std::move(seq));
    auto capsule = py::capsule(seq_ptr, [](void* p) { delete reinterpret_cast<Sequence*>(p); });
    return py::array(seq_ptr->size(),  // shape of array
                     seq_ptr->data(),  // c-style contiguous strides for Sequence
                     capsule           // numpy array references this parent
    );
}

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
            using namespace std;
            auto start = std::chrono::high_resolution_clock::now();
            auto mesh = Mesh::load_mesh(mesh_name);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> diff = end - start;
            //std::cout << "Mesh Loading Time : " << diff.count() << "(s)\n";
            static vector<shared_ptr<Mesh> > mesh_ptrs;
            mesh_ptrs.push_back(mesh);
            std::vector<RT> rt_arr;
            std::vector<ET> et_arr;
            for (std::string str : relations) {
                rt_arr.push_back(RT(name2dim[str[0]] * 4 + name2dim[str[1]]));
                et_arr.push_back(ET(name2dim[str[0]]));
                et_arr.push_back(ET(name2dim[str[1]]));
            }
            int patch_size = 256;
            patcher->run(mesh.get(), patch_size, et_arr, rt_arr);
        })
        .def("export_json", [](MeshTaichi::Patcher *patcher) {
            std::string json = patcher->export_json();
            return json;
        })
        .def("get_element_arr", [](MeshTaichi::Patcher *patcher, std::string name, int order){
            using ET = MeshTaichi::MeshElementType;
            using arr = std::unordered_map<ET, std::vector<int>, MeshTaichi::MEHash>;
            std::map<std::string, arr*> ma;
            ma["owned_offsets"] = &(patcher->owned_offsets);
            ma["total_offsets"] = &(patcher->total_offsets);
            ma["l2g_mapping"] = &(patcher->l2g_mapping);
            ma["l2r_mapping"] = &(patcher->l2r_mapping);
            ma["g2r_mapping"] = &(patcher->g2r_mapping);
            //return (*(ma[name]))[ET(order)];
            return vector2np((*(ma[name]))[ET(order)]);
        })
        .def("get_relation_arr", [](MeshTaichi::Patcher *patcher, std::string name, int order){
            using RT = MeshTaichi::MeshRelationType;
            auto local_rel = patcher->local_rels.find(RT(order))->second;
            if (name == "value") return vector2np(local_rel.value);
            if (name == "offset") return vector2np(local_rel.offset);
        })
        .def("get_mesh_x", [](MeshTaichi::Patcher *patcher){
            //return py::to_pyarray(patcher->mesh->verts);
            std::vector<int> ans;
            for (auto x: patcher->mesh->verts) {
                for (auto y: x) {
                    ans.push_back(y);
                }
            }
            return vector2np(ans);
        })
        ;
}
