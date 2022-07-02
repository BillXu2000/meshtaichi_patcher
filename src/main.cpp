#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <vector>
#include <chrono>
#include "patcher_api.h"
#include "csr.h"
#include "cluster.h"
#include "patcher.h"

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

template <typename T>
py::array_t<T> read_tetgen(std::string fn) {
    using namespace std;
    fstream in(fn, fstream::in);
    int n, m;
    in >> n;
    if (fn.substr(fn.size() - 4) == "node") m = 3;
    if (fn.substr(fn.size() - 4) == "face") m = 3;
    if (fn.substr(fn.size() - 3) == "ele") m = 4;
    for (char ch = in.get(); ch != '\n'; ch = in.get());
    std::vector<T> arr(n * m);
    for (int i = 0; i < n; i++) {
        int _;
        in >> _;
        for (int j = 0; j < m; j++) {
            in >> arr[i * m + j];
        }
        for (char ch = in.get(); !in.eof() && ch != '\n'; ch = in.get());
    }
    return py::array(arr.size(), arr.data());
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
            if (name == "patch_offset") return vector2np(local_rel.patch_offset);
            if (name == "offset") return vector2np(local_rel.offset);
        })
        .def("get_mesh_x", [](MeshTaichi::Patcher *patcher){
            //return py::to_pyarray(patcher->mesh->verts);
            std::vector<float> ans;
            for (auto x: patcher->mesh->verts) {
                for (auto y: x) {
                    ans.push_back(y);
                }
            }
            return vector2np(ans);
        })
        ;
    

    m.def("add_index_test", [](py::array_t<int> x) {
        auto arr = x.mutable_unchecked<1>();
        for (py::ssize_t i = 0; i < arr.shape(0); i++) {
            arr(i) += i;
            printf("%d ", arr(i));
        }
        puts("");
    }, py::arg().noconvert());

    // m.def("get_csr", [](py::array_t<int> offset, py::array_t<int> value) {
    //     return Csr(offset, value);
    // }, py::arg().noconvert());

    py::class_<Csr>(m, "Csr_cpp")
        .def(py::init<py::array_t<int> &, py::array_t<int> &>())
        .def_readwrite("offset", &Csr::offset)
        .def_readwrite("value", &Csr::value)
        .def("transpose", &Csr::transpose)
        .def("mul", &Csr::mul)
        .def("mul_unique", &Csr::mul_unique)
        .def("remove_self_loop", &Csr::remove_self_loop)
        .def("from_numpy", &Csr::from_numpy)
        ;

    m.def("get_np_test", [](int n) {
        std::vector<int> a(n);
        for (int i = 0; i < n; i++) {
            a[i] = i;
        }
        return py::array_t<int>(a.size(), a.data());
    });

    // py::class_<Cluster>(m, "Cluster_cpp")
    //     .def(py::init<>())
    //     .def_readwrite("patch_size", &Cluster::patch_size)
    //     .def("run", &Cluster::run)
    //     .def("run_greedy", &Cluster::run_greedy)
    //     ;

    py::class_<Patcher>(m, "Patcher_cpp")
        .def(py::init<>())
        .def_readwrite("n_order", &Patcher::n_order)
        .def_readwrite("patch_size", &Patcher::patch_size)
        .def_readwrite("cluster_option", &Patcher::cluster_option)
        .def_readwrite("debug", &Patcher::debug)
        .def("get_size", &Patcher::get_size)
        .def("set_relation", &Patcher::set_relation)
        .def("get_relation", &Patcher::get_relation)
        .def("generate_elements", &Patcher::generate_elements)
        .def("patch", &Patcher::patch)
        .def("get_owned", &Patcher::get_owned)
        .def("get_total", &Patcher::get_total)
        .def("get_relation_meta", &Patcher::get_relation_meta)
        .def("get_patch_offset", &Patcher::get_patch_offset)
        .def("write", &Patcher::write)
        .def("read", &Patcher::read)
        .def("get_face", &Patcher::get_face)
        .def("set_pos", &Patcher::set_pos)
        .def("get_pos", &Patcher::get_pos)

        .def("add_patch_relation", &Patcher::add_patch_relation)
        .def("add_all_patch_relation", &Patcher::add_all_patch_relation)

        .def("get_mapping", &Patcher::get_mapping)
        ;
    
    m.def("read_tetgen", [](std::string fn) {
        py::list ans;
        if (fn.substr(fn.size() - 4) == "node") ans.append(read_tetgen<float>(fn));
        else ans.append(read_tetgen<int>(fn));
        return ans;
    });
}
