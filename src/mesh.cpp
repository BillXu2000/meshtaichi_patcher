#include "patcher_mesh.h"

namespace MeshTaichi {

std::string element_type_name(MeshElementType type) {
  if (type == MeshElementType::Vertex)
    return "Vertex";
  else if (type == MeshElementType::Edge)
    return "Edge";
  else if (type == MeshElementType::Face)
    return "Face";
  else if (type == MeshElementType::Cell)
    return "Cell";
  else {
    assert(0);
    return "";
  }
}

std::string relation_type_name(MeshRelationType type) {
  return element_type_name(MeshElementType(from_end_element_order(type))) +
         "-" + element_type_name(MeshElementType(to_end_element_order(type)));
}

std::string conv_type_name(ConvType type) {
  if (type == ConvType::l2g)
    return "local to global";
  else if (type == ConvType::l2r)
    return "local to reordered";
  else if (type == ConvType::g2r) {
    return "global to reordered";
  } else {
    assert(0);
    return "";
  }
}

int element_order(MeshElementType type) {
  return int(type);
}

int from_end_element_order(MeshRelationType rel) {
  return int(rel) >> 0x2;
}

int to_end_element_order(MeshRelationType rel) {
  return int(rel) & 0x3;
}

MeshRelationType relation_by_orders(int from_order, int to_order) {
  return MeshRelationType((from_order << 2) | to_order);
}

MeshRelationType inverse_relation(MeshRelationType rel) {
  return relation_by_orders(to_end_element_order(rel),
                            from_end_element_order(rel));
}

using Edge = std::pair<int, int>;
using Face = std::tuple<int, int, int>;
using Cell = std::tuple<int, int, int, int>;

std::shared_ptr<Mesh> load_obj(std::string filename, bool shuffle) {
  FILE* fp = fopen(filename.c_str(), "r");
  if (fp == nullptr) {
    assert(0);
  }

  auto mesh = std::make_shared<Mesh>(MeshTopology::Triangle);

  std::vector<int> perm(0);
  if (shuffle) {
    FILE* fp = fopen(filename.c_str(), "r");
    char buf[256];
    int n = 0;
    while (fgets(buf, 256, fp)) {
      if (buf[0] == '#' || buf[0] == '\n') {
        continue;
      } else if (buf[0] == 'v' && buf[1] == ' ') {  // vertex info
        n++;
      } else if (buf[0] == 'f' && buf[1] == ' ') {
      } else {
      }
    }
    perm.resize(n);
    for (int i = 0; i < n; i++) {
      perm[i] = i;
    }
    srand(233);
    std::random_shuffle(perm.begin(), perm.end(), [](int i){return std::rand() % i;});
    mesh->verts.resize(n);
  }

  std::map<Edge, int> eg_mp;
  int num_edges = 0;
  auto &EV = mesh->alloca_rel(MeshRelationType::EV);
  auto add_edge = [&](int v0, int v1) {
    if (eg_mp.find(Edge(v0, v1)) == eg_mp.end() &&
        eg_mp.find(Edge(v1, v0)) == eg_mp.end()) {
      eg_mp.insert(std::pair<Edge, int>(Edge(v0, v1), num_edges));
      num_edges++;
      EV.push_back({v0, v1});
    }
  };

  int num_faces = 0;
  int num_verts = 0;
  auto &FV = mesh->alloca_rel(MeshRelationType::FV);


  char buf[256];
  char objbuf[3][256];
  auto s2i = [](char buf[]) {
    int ans = 0;
    for (int i = 0; '0' <= buf[i] && buf[i] <= '9'; i++) {
      ans = ans * 10 + buf[i] - '0';
    }
    return ans;
  };
  while (fgets(buf, 256, fp)) {
    if (buf[0] == '#' || buf[0] == '\n') {
      continue;
    } else if (buf[0] == 'v' && buf[1] == ' ') {  // vertex info
      float x, y, z;
      sscanf(buf + 2, "%f %f %f", &x, &y, &z);
      if (shuffle) {
        mesh->verts[perm[num_verts]] = {x, y, z};
      }
      else {
        mesh->verts.push_back({x, y, z});
      }
      num_verts++;
    } else if (buf[0] == 'f' && buf[1] == ' ') {
      int iv0, iv1, iv2, _0, _1, _2, _3, _4, _5;
      // sscanf(buf + 2, "%d/%d/%d %d/%d/%d %d/%d/%d", &iv0, &_0, &_1, &iv1, &_2, &_3, &iv2, &_4, &_5);
      //sscanf(buf + 2, "%d/%d/%d %d/%d/%d %d/%d/%d", &iv0, &_0, &_1, &iv1, &_2, &_3, &iv2, &_4, &_5);
      // sscanf(buf + 2, "%d %d %d", &iv0, &iv1, &iv2);
      //sscanf(buf + 2, "%d//%d %d//%d %d//%d", &iv0, &_1, &iv1, &_3, &iv2, &_5);
      sscanf(buf + 2, "%s %s %s", objbuf[0], objbuf[1], objbuf[2]);
      int v0 = s2i(objbuf[0]) - 1;
      int v1 = s2i(objbuf[1]) - 1;
      int v2 = s2i(objbuf[2]) - 1;
      if (shuffle) {
        v0 = perm[v0];
        v1 = perm[v1];
        v2 = perm[v2];
      }
      num_faces++;
      FV.push_back({v0, v1, v2});
      add_edge(v0, v1);
      add_edge(v1, v2);
      add_edge(v2, v0);
    } else {
    }
  }
  mesh->num_elements.insert(std::make_pair(MeshElementType::Vertex, num_verts));
  mesh->num_elements.insert(std::make_pair(MeshElementType::Edge, num_edges));
  mesh->num_elements.insert(std::make_pair(MeshElementType::Face, num_faces));
  fclose(fp);

  return mesh;
}

std::shared_ptr<Mesh> load_tet(std::string filename, bool shuffle) {
  if (filename.substr(filename.size() - 5) == ".node") {
    filename = filename.substr(0, filename.size() - 5);
  }
  FILE *node_fp = fopen((std::string(filename) + ".node").c_str(), "r");
  if (node_fp == nullptr) {
    assert(0);
  }

  int _0, _1, _2, _3, _4; // placeholder

  fscanf(node_fp, "%d %d %d %d\n", &_0, &_1, &_2, &_3);
  
  auto mesh = std::make_shared<Mesh>(MeshTopology::Tetrahedron);
  
  auto num_verts = _0;
  mesh->num_elements.insert(std::make_pair(MeshElementType::Vertex, num_verts));
  mesh->verts.resize(num_verts);
  std::vector<int> perm(num_verts);
  if (shuffle) {
    for (int i = 0; i < num_verts; i++) {
      perm[i] = i;
    }
    srand(233);
    std::random_shuffle(perm.begin(), perm.end(), [](int i){return std::rand() % i;});
  }
  for (int i = 0; i < num_verts; ++i) {
    float x, y, z;
    fscanf(node_fp, "%d %f %f %f", &_0, &x, &y, &z);
    if (shuffle) {
      _0 = perm[_0];
    }
    mesh->verts[_0] = {x, y, z};
  }
  fclose(node_fp);

  FILE *ele_fp = fopen((std::string(filename) + ".ele").c_str(), "r");
  if (ele_fp == nullptr) {
    assert(0);
  }

  int num_edges = 0;
  auto &EV = mesh->alloca_rel(MeshRelationType::EV);

  std::map<Edge, int> eg_mp;
  auto add_edge = [&](int v0, int v1) {
    if (eg_mp.find(Edge(v0, v1)) == eg_mp.end() &&
        eg_mp.find(Edge(v1, v0)) == eg_mp.end()) {
        eg_mp.insert(std::pair<Edge, int>(Edge(v0, v1), num_edges));
        num_edges++;
        EV.push_back({v0, v1});
      }
  };

  int num_faces = 0;
  auto &FV = mesh->alloca_rel(MeshRelationType::FV);

  std::map<Face, int> f_mp;
  auto add_face = [&](int v0, int v1, int v2) {
    if (f_mp.find(Face(v0, v1, v2)) == f_mp.end() &&
        f_mp.find(Face(v0, v2, v1)) == f_mp.end() &&
        f_mp.find(Face(v1, v0, v2)) == f_mp.end() &&
        f_mp.find(Face(v1, v2, v0)) == f_mp.end() &&
        f_mp.find(Face(v2, v0, v1)) == f_mp.end() &&
        f_mp.find(Face(v2, v1, v0)) == f_mp.end()) {
        f_mp.insert(std::pair<Face, int>(Face(v0, v1, v2), num_faces));
        num_faces++;
        FV.push_back({v0, v1, v2});
      }
  };

  fscanf(ele_fp, "%d %d %d", &_0, &_1, &_2);
  auto num_cells = _0;
  mesh->num_elements.insert(std::make_pair(MeshElementType::Cell, num_cells));
  auto &CV = mesh->alloca_rel(MeshRelationType::CV);
  CV.resize(num_cells);

  for (int i = 0; i < num_cells; ++i) {
    fscanf(ele_fp, "%d %d %d %d %d", &_0, &_1, &_2, &_3, &_4);
    if (shuffle) {
      _1 = perm[_1];
      _2 = perm[_2];
      _3 = perm[_3];
      _4 = perm[_4];
    }
    CV[_0] = {_1, _2, _3, _4};
    add_edge(_1, _2);
    add_edge(_1, _3);
    add_edge(_1, _4);
    add_edge(_2, _3);
    add_edge(_2, _4);
    add_edge(_3, _4);
    add_face(_2, _3, _4);
    add_face(_1, _3, _4);
    add_face(_1, _2, _4);
    add_face(_1, _2, _3);
  }
  fclose(ele_fp);

  mesh->num_elements.insert(std::make_pair(MeshElementType::Edge, num_edges));
  mesh->num_elements.insert(std::make_pair(MeshElementType::Face, num_faces));

  return mesh;
}

std::shared_ptr<Mesh> Mesh::load_mesh(std::string filename) {
  if (filename.substr(filename.size() - 3) == "obj") {
    return load_obj(filename, false);
  }
  else {
    return load_tet(filename, false);
  }
}

}