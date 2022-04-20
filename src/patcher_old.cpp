#include "patcher_old.h"

#include <set>
#include <cassert>
#include <sstream>

namespace MeshTaichi {

void Patcher::initialize(int _desired_size) {
  desired_size = _desired_size;
  num_seeds = (get_element_num() + desired_size - 1) / desired_size / 4;
  seeds.resize(get_element_num());
  std::fill(seeds.begin(), seeds.end(), 0);
  // Selecting random faces as the seeds for Lloyd’s algorithm
  std::set<int> candidate;
  const auto &R = get_relation();
  for (int i = 0; i < get_element_num(); ++i) {
    if (seeds[i] == 0) {
      candidate.insert(i);
      std::queue<int> qu;
      qu.push(i);
      seeds[i] = -1;
      while (!qu.empty()) {
        int p = qu.front();
        qu.pop();
        for (const auto &q : R[p]) {
          if (seeds[q] == 0) {
            seeds[q] = -1;
            qu.push(q);
          }
        }
      }
    }
  }

  std::vector<int> shuffle;
  for (int i = 0; i < get_element_num(); ++i) {
    if (candidate.find(i) == candidate.end()) {
      shuffle.emplace_back(i);
    }
  }

  std::random_shuffle(shuffle.begin(), shuffle.end());
  num_seeds = std::max(num_seeds, int(candidate.size()));
  int i = 0;
  for (const auto x : candidate) {
    seeds[x] = i;
    i += 1;
  }
  for (; i < num_seeds; ++i) {
    int j = shuffle[i - candidate.size()];
    seeds[j] = i;
  }
}

void Patcher::generate(int max_iteration, int add_per_iter) {
  int iter = 0;
  while (iter < max_iteration) {
    ++iter;
    //std::cout << iter << std::endl;
    patch_assign();
    int over_count = construct_patch();
    if (over_count == 0) break;
    if (iter < max_iteration) {
      update_seed(over_count);
      if ((iter > 0 && iter % add_per_iter == 0) || over_count < num_seeds * divide_threshold) {  // Accelerating the convergence
        add_seed();
      }
    }
  }

  seeds.clear();
  upd_vis.clear();
}

void Patcher::patch_assign() {
  std::queue<int> qu;
  for (int i = 0; i < get_element_num(); ++i) {
    if (seeds[i] >= 0 && lock.count(seeds[i]) == 0) {
      qu.push(i);
    }
  }

  const auto &R = get_relation();

  while (!qu.empty()) {
    int p = qu.front();
    qu.pop();
    for (const auto &q : R[p]) {
      if (seeds[q] == -1) {
        seeds[q] = seeds[p];
        qu.push(q);
      }
    }
  }
}

int Patcher::construct_patch() {
  // Represent patches in a compact format
  offset.resize(num_seeds + 1);
  value.resize(get_element_num());
  std::fill(offset.begin(), offset.end(), 0);
  
  max_size.resize(num_seeds);
  std::fill(max_size.begin(), max_size.end(), 0);

  for (const auto &s : seeds) {
    assert(s < num_seeds);
    ++offset[static_cast<int>(s)];
  }

  // Calc Statstics
  
  for (int i = 0; i < num_seeds; ++i) {
    max_size[i] = offset[i];
    //max_size[i] = 0;
  }

  for (int i = 1; i <= num_seeds; ++i) {
    offset[i] += offset[i - 1];
  }

  for (int i = 0; i < value.size(); ++i) {
    const auto &off = --offset[seeds[i]];
    value[off] = i;
  }

  // std::vector<std::set<int> > ribbon(num_seeds);
  // const auto &R = get_relation();
  // for (int i = 0; i < num_seeds; ++i) {
  //   int off_start = offset[i], off_end = (i < num_seeds - 1 ? offset[i+1] : upd_vis.size());
  //   for (int j = off_start; j < off_end; ++j) {
  //     const auto &p = value[j];
  //     for (const auto &q : R[p]) {
  //         if (seeds[q] != seeds[p] && ribbon[i].find(q) == ribbon[i].end()) {
  //           max_size[i]++;
  //           ribbon[i].insert(q);
  //       }
  //     }
  //   }
  // }
  // std::vector<std::set<int> > ribbon(num_seeds);
  // /*const auto &VR = mesh->make_rel(relation_by_orders(0, get_highest_order()));
  // const auto &RV = mesh->make_rel(relation_by_orders(get_highest_order(), 0));*/
  // int outer = int(main_relation) / 4;
  // //if (outer != get_highest_order()) {
  // if (true) {
  //   const auto &VR = mesh->make_rel(main_relation);
  //   const auto &RV = mesh->make_rel(relation_by_orders(get_highest_order(), int(main_relation) / 4));
  //   for (int i = 0; i < num_seeds; ++i) {
  //     if (lock.count(i)) continue;
  //     //int off_start = offset[i], off_end = (i < num_seeds - 1 ? offset[i+1] : upd_vis.size());
  //     int off_start = offset[i], off_end = offset[i+1];
  //     for (int j = off_start; j < off_end; ++j) {
  //       const auto &p = value[j];
  //       for (const auto &v : RV[p]) {
  //         for (const auto &q : VR[v]) {
  //           if (ribbon[i].count(q) == 0) {
  //             max_size[i]++;
  //             ribbon[i].insert(q);
  //           }
  //         }
  //       }
  //     }
  //   }
  // }
  // else {
  //   const auto &VR = mesh->make_rel(main_relation);
  //   for (int i = 0; i < num_seeds; ++i) {
  //     if (lock.count(i)) continue;
  //     //int off_start = offset[i], off_end = (i < num_seeds - 1 ? offset[i+1] : upd_vis.size());
  //     int off_start = offset[i], off_end = offset[i+1];
  //     for (int j = off_start; j < off_end; ++j) {
  //       const auto &p = value[j];
  //       for (const auto &q : VR[p]) {
  //         if (ribbon[i].count(q) == 0) {
  //           max_size[i]++;
  //           ribbon[i].insert(q);
  //         }
  //       }
  //     }
  //   }
  // }

  int max_sz = 0, over_count = 0;
  for (auto &sz : max_size) {
    if (sz > desired_size) over_count++;
    max_sz = std::max(max_sz, sz);
  }
  // std::cout << "num seed: " << num_seeds << " max size: " << max_sz << " over_count: " << over_count << std::endl;
  // printf("max_size[0] = %d, locksize = %d\n", max_size[0], lock.size());
  // printf("max_size[1] = %d, locksize = %d\n", max_size[1], lock.size());
  return over_count;
}

void Patcher::update_seed(int over_count) {
  upd_vis.resize(get_element_num());
  const auto &R = get_relation();
  std::queue<int> qu;
  for (int i = 0; i < num_seeds; ++i) {
    if (over_count < num_seeds * divide_threshold && max_size[i] <= desired_size) {
      lock.insert(i);
      continue;
    }
    int off_start = offset[i], off_end = (i < num_seeds - 1 ? offset[i+1] : upd_vis.size());
    for (int j = off_start; j < off_end; ++j) {
      const auto &p = value[j];
      bool on_boundary = false;
      for (const auto &q : R[p]) {
        if (seeds[q] != seeds[p]) {
          on_boundary = true;
          break;
        }
      }
      // constructing the patch boundary faces, 
      // i.e., faces in this patch incident to faces assigned to a different patch
      if (on_boundary) {
        qu.push(p);
        upd_vis[p] = 0;
      } else {
        upd_vis[p] = -1;
      }
    }

    int last_state = 0;
    while (!qu.empty()) {
      const auto &p = qu.front(); qu.pop();
      for (const auto &q : R[p]) {
        if (upd_vis[q] == -1 && seeds[q] == seeds[p]) { // marks the neighbor face as visited
          upd_vis[q] = upd_vis[p] + 1;
          last_state = upd_vis[q];
          qu.push(q);
        }
      }
    }
    std::vector<int> last_vis;
    for (int j = off_start; j < off_end; ++j) {
      const auto &p = value[j];
      if (upd_vis[p] == last_state) {
        last_vis.push_back(p);
      }
    }

    // pick a face randomly from the faces added in the final round
    int new_seed = 0;
    if (last_vis.size() > 0) 
      new_seed = last_vis[static_cast<int>(rand()) % last_vis.size()];
    else
      new_seed = value[off_start];
    for (int j = off_start; j < off_end; ++j) {
      const auto &p = value[j];
      seeds[p] = -1;
    }
    seeds[new_seed] = i; // update seed with the centroid
  }
}

void Patcher::add_seed() {
  int tmp_num_seeds = num_seeds;
  for (int i = 0; i < tmp_num_seeds; i++) {
    int off_start = offset[i], off_end = (i < tmp_num_seeds - 1 ? offset[i+1] : get_element_num());
    if (max_size[i] > desired_size) {
      int extra_seeds = (max_size[i] - 1) / desired_size;
      std::vector<int> shuffle;
      for (int j = off_start; j < off_end; ++j) {
        shuffle.push_back(value[j]);
      }

      std::random_shuffle(shuffle.begin(), shuffle.end());
      for (const auto &p : shuffle) {
        if (seeds[p] == -1) {
          seeds[p] = num_seeds++;
          --extra_seeds;
          //if (!extra_seeds) break;
          break;
        }
      }
    }
  }
}

void Patcher::build_patches(std::unordered_set<MeshElementType, MEHash> eles, 
                            std::unordered_set<MeshRelationType, MRHash> rels) {
  patches_info.resize(num_seeds);

  for (int i = 0; i < 4; ++i) {
    auto type = MeshElementType(i);
    if (eles.find(type) == eles.end() && i != get_highest_order()) {
      continue;
    }
    g2l.insert(std::make_pair(type, std::vector<int>(mesh->num_elements[type])));
    element_owner.insert(std::make_pair(type, std::vector<int>(mesh->num_elements[type])));
  }

  for (int patch_id = 0; patch_id < num_seeds; ++patch_id) {
    int off_start = offset[patch_id],
          off_end = (patch_id < num_seeds - 1 ? offset[patch_id + 1]
                                              : get_element_num());
    
    auto &pi = patches_info[patch_id];
    for (int i = get_highest_order(); i >= 0; --i) {
      auto type = MeshElementType(i);
      if (eles.find(type) == eles.end() && i != get_highest_order()) {
        continue;
      }
      pi.owned_elements.insert(std::make_pair(type, 0));
      pi.total_elements.insert(std::make_pair(type, 0));
      pi.l2g.insert(std::make_pair(type, std::vector<int>()));
      pi.ribbon_g2l.insert(std::make_pair(type, std::map<int, int>()));
    }

    for (int i = off_start; i < off_end; ++i) {
      for (int order = get_highest_order(); order >= 0; --order) {
        auto type = MeshElementType(order);
        if (eles.find(type) == eles.end() && order != get_highest_order()) {
          continue;
        }
        if (order == get_highest_order()) {
          auto id = value[i];
          element_owner[type][id] = patch_id;
        } else {
          for (auto id : mesh->make_rel(relation_by_orders(get_highest_order(), order))[value[i]]) {
            element_owner[type][id] = patch_id;
          }
        }
      }
    }
  }


  offset.clear();
  value.clear();

  for (int order = get_highest_order(); order >= 0; --order) {
    auto type = MeshElementType(order);
    if (eles.find(type) == eles.end() && order != get_highest_order()) {
      continue;
    }
    for (int id = 0; id < mesh->num_elements[type]; ++id) {
      auto &pi = patches_info[element_owner[type][id]];
      pi.owned_elements[type]++;
      pi.total_elements[type]++;
      pi.l2g[type].push_back(id);
      g2l[type][id] = pi.l2g[type].size() - 1;
    }
  }

  {
  auto type = MeshElementType(get_highest_order());
  const auto &HH = get_relation();
  for (int e1 = 0; e1 < mesh->num_elements[type]; ++e1) {
    for (const auto &e2 : HH[e1]) {
      const auto &p1 = element_owner[type][e1];
      const auto &p2 = element_owner[type][e2];

      if (p1 != p2) {
        auto ribboning = [&](int e1, int p1, int e2, int p2) {
          auto &pi = patches_info[p1];
          if (pi.ribbon_g2l[type].find(e2) == pi.ribbon_g2l[type].end()) {
            ++pi.total_elements[type];
            pi.l2g[type].push_back(e2);
            pi.ribbon_g2l[type].insert(std::make_pair(
              e2, pi.l2g[type].size() - 1));
          }
        };
        ribboning(e1, p1, e2, p2);
        ribboning(e2, p2, e1, p1);
      }
    }
  }
  }


  // construct ribbons

  std::vector<MeshRelationType> vec_rels;
  for (auto rel : rels) {
    vec_rels.push_back(rel);
  }

  std::sort(vec_rels.begin(), vec_rels.end(), [](MeshRelationType rel1, MeshRelationType rel2) {
    if (from_end_element_order(rel1) == from_end_element_order(rel2)) {
      return to_end_element_order(rel1) > to_end_element_order(rel2);
    }
    return from_end_element_order(rel1) > from_end_element_order(rel2);
  });

  for (auto rel : vec_rels) {
    auto from_type = MeshElementType(from_end_element_order(rel));
    auto to_type =  MeshElementType(to_end_element_order(rel));
    auto & XX = mesh->make_rel(rel);
    for (int p = 0; p < num_seeds; ++p) {
      auto &pi = patches_info[p];
      auto ele_range = int(from_type) <= int(to_type) ? pi.owned_elements[from_type] : pi.total_elements[from_type];
      // auto ele_range = pi.owned_elements[from_type];
      for (int i = 0; i < ele_range; ++i) {
        const auto i_g = pi.l2g[from_type][i];
        auto find_g2l = [&](int idx) {
          if (element_owner[to_type][idx] == p) {
            return g2l[to_type][idx];
          } else if (pi.ribbon_g2l[to_type].find(idx) != pi.ribbon_g2l[to_type].end()) {
            return pi.ribbon_g2l[to_type].find(idx)->second;
          } else {
            ++pi.total_elements[to_type];
            pi.l2g[to_type].push_back(idx);
            pi.ribbon_g2l[to_type].insert(std::make_pair(idx, pi.l2g[to_type].size() - 1));
            return static_cast<int>(pi.l2g[to_type].size() - 1);
          }
        };

        for (const auto &x : XX[i_g]) {
          const auto &x_l = find_g2l(x);
        }
      }
    }
  }

  for (auto rel : rels) {
    auto from_type = MeshElementType(from_end_element_order(rel));
    auto to_type =  MeshElementType(to_end_element_order(rel));
    auto & XX = mesh->make_rel(rel);
    std::vector<int> value, patch_offset, offset;
    for (int p = 0; p < num_seeds; ++p) {
      auto &pi = patches_info[p];
      if (int(from_type) <= int(to_type)) {
        patch_offset.push_back(value.size());
        offset.push_back(0);
      }

      auto ele_range = int(from_type) <= int(to_type) ? pi.owned_elements[from_type] : pi.total_elements[from_type];
      // auto ele_range = pi.owned_elements[from_type];
      for (int i = 0; i < ele_range; ++i) {
        const auto i_g = pi.l2g[from_type][i];
        auto find_g2l = [&](int idx) {
          if (element_owner[to_type][idx] == p) {
            return g2l[to_type][idx];
          } else if (pi.ribbon_g2l[to_type].find(idx) != pi.ribbon_g2l[to_type].end()) {
            return pi.ribbon_g2l[to_type].find(idx)->second;
          } else {
            assert(0);
            return -1;
          }
        };

        for (const auto &x : XX[i_g]) {
          const auto &x_l = find_g2l(x);
          assert(pi.l2g[to_type][x_l] == x);
          value.push_back(x_l);
        }
        if (int(from_type) <= int(to_type)) {
          offset.push_back(value.size() - patch_offset.back());
        }
      }
    }

    if (int(from_type) <= int(to_type)) {
      local_rels.insert(std::make_pair(rel, LocalRel(std::move(value), std::move(patch_offset), std::move(offset))));
    } else {
      local_rels.insert(std::make_pair(rel, LocalRel(std::move(value))));
    }
  }

  int total = 0, mm = 0;
  for (int p = 0; p < num_seeds; ++p) {
    auto &pi = patches_info[p];
    int val = pi.l2g[MeshElementType(3)].size();
    total += val;
    mm = std::max(mm, val);
  }
  //std::cout << "average: " << total / num_seeds << " max: "<< mm << std::endl;
  

  element_owner.clear();
  g2l.clear();

  for (auto ele : eles) {
    owned_offsets.insert(std::make_pair(ele, std::vector<int>({0})));
    total_offsets.insert(std::make_pair(ele, std::vector<int>({0})));
    l2g_mapping.insert(std::make_pair(ele, std::vector<int>()));
    l2r_mapping.insert(std::make_pair(ele, std::vector<int>()));
    g2r_mapping.insert(std::make_pair(ele, std::vector<int>()));
    max_num_per_patch.insert(std::make_pair(ele, 0));

    auto &owned = owned_offsets.find(ele)->second;
    auto &total = total_offsets.find(ele)->second;
    auto &l2g = l2g_mapping.find(ele)->second;
    auto &g2r = g2r_mapping.find(ele)->second;
    auto &l2r = l2r_mapping.find(ele)->second;
    auto &max_num = max_num_per_patch.find(ele)->second;

    std::vector<int> id;
    id.reserve(mesh->num_elements[ele]);
    g2r.resize(mesh->num_elements[ele]);
    for (const auto &pi : patches_info) {
      const auto &local_owned = pi.owned_elements.find(ele)->second;
      const auto &local_total = pi.total_elements.find(ele)->second;
      const auto &local_l2g = pi.l2g.find(ele)->second;
      owned.push_back(owned.back() + local_owned);
      total.push_back(total.back() + local_total);
      l2g.insert(l2g.end(), local_l2g.begin(), local_l2g.end());
      for (int i = 0; i < local_owned; ++i) {
        id.push_back(local_l2g[i]);
      }
      max_num = std::max(max_num, local_total);
    }

    const int align_size = 32;
    if (max_num % align_size != 0) {
      max_num += (align_size - max_num % align_size);
    }

    for (int i = 0; i < id.size(); ++i) {
      g2r[id[i]] = i;
    }

    l2r.resize(l2g.size());
    for (int i = 0; i < l2g.size(); ++i) {
      l2r[i] = g2r[l2g[i]];
    }
  }
  patches_info.clear();
}

std::string Patcher::export_json(std::unordered_set<MeshElementType, MEHash> eles, 
                          std::unordered_set<MeshRelationType, MRHash> rels) {
  //std::fstream out(filename, std::ios::out);
  std::stringstream out;
  bool comma_flag = false;
  out << "{\n";

  auto dump = [&](std::string name, const std::vector<int> &data) {
    out << "\"" << name << "\" : [";
    for (int i = 0; i < data.size(); ++i) {
      if (i) out << ",";
      out << data[i];
    }
    out << "]";
  };

  out << "\"num_patches\" : " << num_seeds << ",\n";

  // export attributes of elements
  out << "  \"elements\" : [ \n";
  for (const auto &ele : eles) {
    if (comma_flag) out << ",";
    comma_flag = true;
    out << "{";
    out << "\"order\" : " << int(ele) << ",\n";
    out << "\"num\" : " << mesh->num_elements[ele] << ",\n";
    out << "\"max_num_per_patch\" : " << max_num_per_patch[ele] << "\n";
    // dump("owned_offsets", owned_offsets[ele]); out << ",\n";
    // dump("total_offsets", total_offsets[ele]); out << ",\n";
    // dump("l2g_mapping", l2g_mapping[ele]); out << ",\n";
    // dump("g2r_mapping", g2r_mapping[ele]); out << ",\n";
    // dump("l2r_mapping", l2r_mapping[ele]); out << "\n";
    out << "}\n";
  }
  out << "  ], \n";

  // export local relation
  out << "  \"relations\" : [ \n";
  comma_flag = false;
  for (const auto &rel : rels) {
    if (comma_flag) out << ",";
    comma_flag = true;
    out << "{";
    out << "\"from_order\" : " << from_end_element_order(rel) << ",\n";
    out << "\"to_order\" : " << to_end_element_order(rel) << "\n";
    // auto local_rel = local_rels.find(rel)->second;
    // if (!local_rel.fixed) {
    //   dump("offset", local_rel.offset); out << ",\n";
    // }
    // dump("value", local_rel.value); out << "\n";
    out << "}\n";
  }
  out << "  ], \n";

  out << "\"attrs\" : {}\n";

  /*out << "  \"x\" : [ \n";
  comma_flag = false;
  for (const auto &vert : mesh->verts) {
    for (const auto &x : vert) {
      if (comma_flag) out << ",";
      comma_flag = true;
      out << x;
    }
  }
  out << "  ]}\n";*/

  out << "}";
  return out.str();
}

std::string Patcher::export_json() {
  return export_json(ex_eles, ex_rels);
}

void Patcher::run(Mesh *_mesh, int patch_size, std::vector<MeshElementType> eles, std::vector<MeshRelationType> rels) {
  clock_t start_time, tmp_time, end_time;
  start_time = clock();
  mesh = _mesh;
  if (mesh->topology == MeshTopology::Tetrahedron) {
    mesh->make_rel(MeshRelationType::CC);
  } else {
    mesh->make_rel(MeshRelationType::FF);
  }
  end_time = clock();
  // std::cout << "Making FF relation Time : " << (end_time - start_time) * 1.0 / CLOCKS_PER_SEC << "(s)\n";
  start_time = clock();
  initialize(patch_size);
  main_relation = rels[0];
  generate(1 << 20, 1);
  end_time = clock();
  //std::cout << "Iteration Time : " << (end_time - start_time) * 1.0 / CLOCKS_PER_SEC << "(s)\n";
  
  start_time = clock();
  std::unordered_set<MeshElementType, MEHash> _eles;
  std::unordered_set<MeshRelationType, MRHash> _rels;
  for (auto ele : eles) _eles.insert(ele);
  for (auto rel : rels) _rels.insert(rel);
  for (const auto &rel : _rels) {
    _eles.insert(MeshElementType(from_end_element_order(rel)));
    _eles.insert(MeshElementType(to_end_element_order(rel)));
  }
  build_patches(_eles, _rels);
  ex_eles = _eles;
  ex_rels = _rels;
  end_time = clock();
  //std::cout << "Build Patches Time : " << (end_time - start_time) * 1.0 / CLOCKS_PER_SEC << "(s)\n";
}
}
