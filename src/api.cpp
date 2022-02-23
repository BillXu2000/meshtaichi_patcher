#include <iostream>
#include <string>
#include <map>
#include <vector>
#include "patcher_api.h"

namespace MeshTaichi {

using RT = MeshRelationType;
using ET = MeshElementType;

std::map<std::string, std::string> argparse(int argc, char* argv[]) {
  using namespace std;
  vector<string> argv_str;
  map<string, string> args;
  args["tet"] = "armadillo";
  args["patch_size"] = "1024";
  args["output"] = "/home/yc/Desktop/armadillo.json";
  args["divide_threshold"] = "0.5";
  args["extra"] = "";
  for (int i = 0; i < argc; i++) {
    argv_str.push_back(argv[i]);
  }
  for (int i = 1; i < argc; i++) {
    if (argv_str[i].substr(0, 2) == "--") {
      assert(argv_str[i].size() > 2 && i < argc - 1);
      args[argv_str[i].substr(2)] = argv_str[i + 1];
      i++;
    }
  }
  return args;
}

template <typename T>
std::vector<T> get_vector(int n) {
  std::vector<T> ans(n);
  for (int i = 0; i < n; i++) {
    ans[i] = T(n - i - 1);
  }
  return ans;
}

float Patcher::divide_threshold = 0.5;

int main_ra(int argc, char* argv[]) {
  auto args = argparse(argc, argv);
  Patcher::divide_threshold = stof(args["divide_threshold"]);
  if (args["single"].length() == 0 && args.count("shuffle")) {
    assert(0); // shuffle only works for single relation now
  }
  if (args["single"].length() > 0) {
    std::map<char, int> name2dim;
    name2dim['v'] = 0;
    name2dim['e'] = 1;
    name2dim['f'] = 2;
    name2dim['c'] = 3;
    int outer = name2dim[args["single"][0]], inner = name2dim[args["single"][1]];
    /*if (inner >= outer) Patcher::count_ribbon = true;
    else {
      Patcher::divide_threshold = 0;
    }*/
    std::shared_ptr<Mesh> mesh;
    if (args.count("obj")) {
      mesh = load_obj(args["obj"], args.count("shuffle"));
    }
    else {
      mesh = load_tet(args["tet"], args.count("shuffle"));
    }
    std::vector<RT> rt_arr({RT(outer * 4 + inner), RT(outer * 4)});
    //if (outer == 0) rt_arr.push_back(RT(inner * 4));
    //else rt_arr.push_back(RT(outer * 4));
    std::vector<ET> et_arr({ET(outer), ET(inner), ET::Vertex});
    for (int i = 0; i < args["extra"].size(); i += 2) {
      rt_arr.push_back(RT(name2dim[args["extra"][i]] * 4 + name2dim[args["extra"][i + 1]]));
      et_arr.push_back(ET(name2dim[args["extra"][i]]));
      et_arr.push_back(ET(name2dim[args["extra"][i + 1]]));
    }
    //Patcher::run(mesh, stoi(args["patch_size"]), et_arr, rt_arr, args["output"]);
  }
  else if (args.find("obj") == args.end()) {
    auto mesh = load_tet(args["tet"]);
    //Patcher::run(mesh, stoi(args["patch_size"]), {ET::Cell, ET::Edge, ET::Vertex}, {RT::CV, RT::EV, RT::VE, RT::VV}, args["output"]);
  }
  else if (args["relation"] == "fv") {
    auto mesh = load_obj(args["obj"]);
    //Patcher::run(mesh, stoi(args["patch_size"]), {ET::Face, ET::Vertex}, {RT::FV, RT::VF, RT::VV}, args["output"]);
  }
  else if (args["relation"] == "cv") {
    auto mesh = load_obj(args["obj"]);
    //Patcher::run(mesh, stoi(args["patch_size"]), {ET::Cell, ET::Vertex}, {RT::CV, RT::VC, RT::VV}, args["output"]);
  }
  else {
    auto mesh = load_obj(args["obj"]);
    //Patcher::run(mesh, stoi(args["patch_size"]), {ET::Edge, ET::Vertex}, {RT::EV, RT::VE, RT::VV}, args["output"]);
  }
  return 0;
}

std::string run_mesh(std::string mesh_name, std::vector<std::string> relations) {
  std::map<char, int> name2dim;
  name2dim['v'] = 0;
  name2dim['e'] = 1;
  name2dim['f'] = 2;
  name2dim['c'] = 3;
  std::shared_ptr<Mesh> mesh = Mesh::load_mesh(mesh_name);
  std::vector<RT> rt_arr;
  std::vector<ET> et_arr;
  for (std::string str : relations) {
    rt_arr.push_back(RT(name2dim[str[0]] * 4 + name2dim[str[1]]));
    et_arr.push_back(ET(name2dim[str[0]]));
    et_arr.push_back(ET(name2dim[str[1]]));
  }
  int patch_size = 256;
  Patcher patcher;
  patcher.run(mesh.get(), patch_size, et_arr, rt_arr);
  return patcher.export_json();
}

}