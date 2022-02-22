#pragma once

#include <algorithm>
#include <random>
#include <queue>
#include <fstream>
#include <ctime>
#include <set>
#include "patcher_mesh.h"

namespace MeshTaichi {

class Patcher {
 public:
  Mesh *mesh;
  int desired_size; // Every patch size should be less or equal to desired_size
  int num_seeds; // Total number of seeds/patches
  static float divide_threshold;
  std::vector<int> seeds; // Faces belong to which seed
  std::vector<int> upd_vis; // Visit state in update seeds stage
  
  // Patching result
  std::vector<int> offset; // The prefix sum of the patch size array
  std::vector<int> value;  //  stores the IDs of the patch’s faces
  std::vector<int> max_size;
  std::set<int> lock;
  MeshRelationType main_relation;

  Patcher(std::shared_ptr<Mesh> mesh_) {
    mesh = mesh_.get();
    if (mesh->topology == MeshTopology::Tetrahedron) {
      mesh->make_rel(MeshRelationType::CC);
    } else {
      mesh->make_rel(MeshRelationType::FF);
    }
  }

  void initialize(int _desired_size);
  void generate(int max_iteration, int add_per_iter);

  void patch_assign(); // Assigning vertices to the "nearest" seed to create partitions
  int construct_patch();
  void update_seed(int over_count); // Updating the partition’s seed with the partition's centroid
  void add_seed();

  const Mesh::GlobalRelation& get_relation() const {
    if (mesh->topology == MeshTopology::Tetrahedron) {
      return mesh->get_rel(MeshRelationType::CC);
    } else {
      return mesh->get_rel(MeshRelationType::FF);
    }
  }
  const size_t get_element_num() const {
    if (mesh->topology == MeshTopology::Tetrahedron) {
      return mesh->num_elements.find(MeshElementType::Cell)->second;
    } else {
      return mesh->num_elements.find(MeshElementType::Face)->second;
    }
  }

  const int get_highest_order() const {
    if (mesh->topology == MeshTopology::Tetrahedron) {
      return 3;
    } else {
      return 2;
    }
  }

  // Construct Patches
  std::unordered_map<MeshElementType, std::vector<int>> element_owner;
  std::unordered_map<MeshElementType, std::vector<int>> g2l;

  // Patch info
  // Relation info are from Tetmesh/Trimesh, but we should localize index
  struct LocalRel {
    LocalRel(std::vector<int> && value_, std::vector<int> &&offset_)
        : value(value_), offset(offset_) {
      fixed = false;
    }

    LocalRel(std::vector<int> &&value_) : value(value_) {
      fixed = true;
    }

    bool fixed;
    std::vector<int> value;
    std::vector<int> offset;
  };

  struct PatchInfo {
    std::unordered_map<MeshElementType, int> total_elements;
    std::unordered_map<MeshElementType, int> owned_elements;
    std::unordered_map<MeshElementType, std::vector<int>> l2g;
    mutable std::unordered_map<MeshElementType, std::map<int, int>> ribbon_g2l;
  };

  std::vector<PatchInfo> patches_info;

  void build_patches(std::unordered_set<MeshElementType> eles, 
                     std::unordered_set<MeshRelationType> rels);

  // flattened info
  std::unordered_map<MeshElementType, std::vector<int>> owned_offsets;
  std::unordered_map<MeshElementType, std::vector<int>> total_offsets;
  std::unordered_map<MeshElementType, std::vector<int>> l2g_mapping;
  std::unordered_map<MeshElementType, std::vector<int>> l2r_mapping;
  std::unordered_map<MeshElementType, std::vector<int>> g2r_mapping;
  std::unordered_map<MeshElementType, int> max_num_per_patch;
  std::unordered_map<MeshRelationType, LocalRel> local_rels;

  void export_json(std::string filename,
                   std::unordered_set<MeshElementType> eles, 
                   std::unordered_set<MeshRelationType> rels);

  static Patcher run(std::shared_ptr<Mesh> mesh, 
      int patch_size, 
      std::vector<MeshElementType> eles, 
      std::vector<MeshRelationType> rels,
      std::string filename) {
    clock_t start_time, end_time;
    Patcher patcher(mesh);
    patcher.initialize(patch_size);
    patcher.main_relation = rels[0];
    start_time = clock();
    patcher.generate(1 << 20, 1);
    end_time = clock();
    std::cout << "Patching Time : "
            << (end_time - start_time) * 1.0 / CLOCKS_PER_SEC << "(s)\n";
    
    std::unordered_set<MeshElementType> _eles;
    std::unordered_set<MeshRelationType> _rels;
    for (auto ele : eles) _eles.insert(ele);
    for (auto rel : rels) _rels.insert(rel);
    for (const auto &rel : _rels) {
      _eles.insert(MeshElementType(from_end_element_order(rel)));
      _eles.insert(MeshElementType(to_end_element_order(rel)));
    }
    start_time = clock();
    patcher.build_patches(_eles, _rels);
    end_time = clock();
    std::cout << "Build Patches Time : "
            << (end_time - start_time) * 1.0 / CLOCKS_PER_SEC << "(s)\n";
    
    patcher.export_json(filename, _eles, _rels);
    mesh->print();

    return patcher;
  }
};

}