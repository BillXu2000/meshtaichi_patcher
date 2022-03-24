#pragma once

#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <tuple>
#include <algorithm>
#include <assert.h>

namespace MeshTaichi {

enum class MeshTopology { Triangle = 3, Tetrahedron = 4 };

enum class MeshElementType { Vertex = 0, Edge = 1, Face = 2, Cell = 3 };

std::string element_type_name(MeshElementType type);

enum class MeshRelationType {
  VV = 0, VE = 1, VF = 2, VC = 3,
  EV = 4, EE = 5, EF = 6, EC = 7,
  FV = 8, FE = 9, FF = 10, FC = 11,
  CV = 12, CE = 13, CF = 14, CC = 15,
};

std::string relation_type_name(MeshRelationType type);

enum class MeshElementReorderingType {
  NonReordering,
  Reordering,
};

enum class ConvType { l2g, l2r, g2r };

std::string conv_type_name(ConvType type);

int element_order(MeshElementType type);
int from_end_element_order(MeshRelationType rel);
int to_end_element_order(MeshRelationType rel);
MeshRelationType relation_by_orders(int from_order, int to_order);
MeshRelationType inverse_relation(MeshRelationType rel);

class MEHash {
public:
 std::size_t operator()(const MeshElementType& k) const
 {
   return int(k);
 }
};

class MRHash {
public:
 std::size_t operator()(const MeshRelationType& k) const
 {
   return int(k);
 }
};

class Mesh {
  public:
  using GlobalRelation = std::vector<std::vector<int>>;
  MeshTopology topology;
  std::unordered_map<MeshElementType, int, MEHash> num_elements;
  std::unordered_map<MeshRelationType, GlobalRelation, MRHash> global_rels;
  std::vector<std::vector<float>> verts;

  GlobalRelation& alloca_rel(MeshRelationType rel_type) {
    global_rels.insert(std::make_pair(rel_type, Mesh::GlobalRelation()));
    return global_rels.find(rel_type)->second;
  }

  const GlobalRelation& get_rel(MeshRelationType rel_type) {
    return global_rels.find(rel_type)->second;
  }

  GlobalRelation& make_rel(MeshRelationType rel_type) {
    if (global_rels.find(rel_type) != global_rels.end()) { // CV, FV, EV
      return global_rels.find(rel_type)->second;
    }

    auto &rel = alloca_rel(rel_type);
    auto from_end = from_end_element_order(rel_type);
    auto to_end = to_end_element_order(rel_type);

    if (from_end == 0 && to_end == 0) { // VV
      rel = mul(
        make_rel(MeshRelationType::VE),
        make_rel(MeshRelationType::EV),
        1, false
      );
    } else if (from_end > to_end) { // CF, CE, FE
      rel = mul(
        make_rel(relation_by_orders(from_end, 0)),
        make_rel(relation_by_orders(0, to_end)),
        to_end + 1, true
      );
    }  else if (from_end == to_end) { // CC, FF, EE
      rel = mul(
        make_rel(relation_by_orders(from_end, from_end - 1)),
        make_rel(relation_by_orders(from_end - 1, to_end)),
        1, false
      );
    } else { // FC, EC, EF, VE, VF, VC
      rel = inverse(make_rel(relation_by_orders(to_end, from_end)), 
        num_elements.find(MeshElementType(from_end))->second);
    }

    return rel;
  }

  Mesh(MeshTopology topology) {
    this->topology = topology;
  }

  void print() {
    for (auto &x : num_elements) {
      std::cout << "Num " << element_type_name(x.first) << ": " << x.second << std::endl;
    }

    for (auto &x : global_rels) {
      std::cout << relation_type_name(x.first) << " Size: " << x.second.size() << std::endl;
    }
  }

  static std::shared_ptr<Mesh> load_mesh(std::string filename); 

  private:
    GlobalRelation inverse(const GlobalRelation &rel, int size) const {
      GlobalRelation inv_rel(size);
      for (int i = 0; i < rel.size(); i++) {
        for (auto j : rel[i]) {
          inv_rel[j].push_back(i);
        }
      }
      return inv_rel;
    }

    GlobalRelation mul(const GlobalRelation &A, const GlobalRelation &B, int degree, bool diagonal) const {
      GlobalRelation C(A.size());
      std::unordered_map<int, int> degs;
      for (int i = 0; i < A.size(); i++) {
        degs.clear();
        for (auto j : A[i]) {
          for (auto k : B[j]) {
            auto ptr = degs.find(k);
            if (ptr == degs.end()) {
              degs.insert(std::make_pair(k, 1));
              ptr = degs.find(k);
            } else {
              ptr->second++;
            }

            if (ptr->second >= degree) {
              if (!diagonal && i == k) continue;
              C[i].push_back(k);
            }
          }
        }
      }
      return C;
    }
};

std::shared_ptr<Mesh> load_obj(std::string filename, bool shuffle=false);
std::shared_ptr<Mesh> load_tet(std::string filename, bool shuffle=false);


}
