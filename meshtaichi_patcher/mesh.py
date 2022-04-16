from meshtaichi_patcher_core import Patcher_cpp
from .relation import Relation
import numpy as np

class Mesh:
    def __init__(self, mesh):
        self.position = mesh.vertex_matrix()
        self.n_order = 3

        self.patcher = Patcher_cpp()
        self.patcher.n_order = self.n_order

        # self.relation = {(2, 0): Relation(mesh.face_matrix())}
        self.patcher.set_relation(2, 0, Relation(mesh.face_matrix()).csr)
        if len(mesh.edge_matrix()):
            # self.relation[(1, 0)] = Relation(mesh.edge_matrix())
            self.patcher.set_relation(1, 0, Relation(mesh.face_matrix()).csr)
        self.patcher.generate_elements()
        # self.generate_elements()
    
    def get_size(self, order):
        # return len(self.relation[(order, 0)]) if order > 0 else len(self.position)
        return self.patcher.get_size(order)
    
    def get_subsets(s):
        if len(s) == 0:
            return [[]]
        ans = Mesh.get_subsets(s[1:])
        n = len(ans)
        for i in range(n):
            ans.append([s[0]] + ans[i])
        return ans

    def generate_elements(self):
        for order in range(1, self.n_order):
            if (order, 0) in self.relation:
                continue
            relation = []
            ele_set = set()
            for i, u in enumerate(self.relation[(self.n_order - 1, 0)]):
                for v in Mesh.get_subsets(u):
                    v = tuple(sorted(v))
                    if len(v) != order + 1 or v in ele_set:
                        continue
                    ele_set.add(v)
                    relation.append(v)
            self.relation[(order, 0)] = Relation(np.array(relation))

    def get_relation(self, from_end, to_end):
        return Relation(self.patcher.get_relation(from_end, to_end))
        # if (from_end, to_end) in self.relation:
        #     return self.relation[(from_end, to_end)]
        # if from_end < to_end:
        #     self.relation[(from_end, to_end)] = self.get_relation(to_end, from_end).transpose()
        #     return self.relation[(from_end, to_end)]
        # if from_end == to_end == 0:
        #     self.relation[(from_end, to_end)] = self.get_relation(0, 1).mul(self.get_relation(1, 0)).remove_self_loop()
        #     return self.relation[(from_end, to_end)]
        # if from_end == to_end:
        #     self.relation[(from_end, to_end)] = self.get_relation(from_end, from_end - 1).mul(self.get_relation(from_end - 1, from_end)).remove_self_loop()
        #     return self.relation[(from_end, to_end)]
        # # from_end > to_end
        # ele_dict = {}
        # for i, u in enumerate(self.relation[(to_end, 0)]):
        #     ele_dict[tuple(sorted(u))] = i
        # relation = []
        # for i, u in enumerate(self.relation[(from_end, 0)]):
        #     relation.append([])
        #     for v in Mesh.get_subsets(u):
        #         v = tuple(sorted(v))
        #         if len(v) != to_end + 1:
        #             continue
        #         j = ele_dict[v]
        #         relation[i].append(j)
        # self.relation[(from_end, to_end)] = Relation(np.array(relation))
        # return self.relation[(from_end, to_end)]
    
    def patch(self, cluster):
        # self.patcher = Patcher_cpp()
        # for order in range(self.n_order - 1):
        #     self.get_relation(self.n_order - 1, order)
        # for order in range(self.n_order):
        #     self.get_relation(0, order)
        # for i, j in self.relation:
        #     self.patcher.set_relation(i, j, self.relation[(i, j)].csr)
        # self.patcher.n_order = self.n_order
        self.patcher.patch(Relation(cluster.patch).csr)
        self.owned = []
        self.total = []
        for order in range(self.n_order):
            self.owned.append(Relation(self.patcher.get_owned(order)))
            self.total.append(Relation(self.patcher.get_total(order)))
            # print(self.owned[-1].offset, self.owned[-1].value)
        # self.cluster = cluster
        # self.owned = []
        # for order in range(self.n_order - 1):
        #     color = np.zeros([self.get_size(order)], dtype=np.int32)
        #     relation = self.get_relation(self.n_order - 1, order)
        #     for u in range(self.get_size(self.n_order - 1)):
        #         for v in relation[u]:
        #             color[v] = cluster.color[u]
        #     owned = [[] for i in cluster.patch]
        #     for v, c in enumerate(color):
        #         owned[c].append(v)
        #     self.owned.append(Relation(owned))
        # owned = None
        # self.owned.append(Relation(cluster.patch))
        # self.total = []
        # for order in range(self.n_order):
        #     total = []
        #     for c, p in enumerate(cluster.patch):
        #         s = set(self.owned[order][c])
        #         rel_0 = self.get_relation(self.n_order - 1, 0)
        #         rel_1 = self.get_relation(0, order)
        #         l = []
        #         for u in p:
        #             for v in rel_0[u]:
        #                 for w in rel_1[v]:
        #                     if w not in s:
        #                         s.add(w)
        #                         l.append(w)
        #         total.append(list(self.owned[order][c]) + l)
        #     self.total.append(Relation(total))
    
    def get_element_meta(self, order):
        ans = {}
        ans["order"] = order
        ans["num"] = self.get_size(order)
        ans["max_num_per_patch"] = max([len(i) for i in self.total[order]])
        ans["owned_offsets"], tmp = self.owned[order].offset, self.owned[order].value
        ans["total_offsets"], ans["l2g_mapping"] = self.total[order].offset, self.total[order].value
        g2r = [0] * self.get_size(order)
        for i, k in enumerate(tmp):
            g2r[k] = i
        ans["g2r_mapping"] = np.array(g2r)
        l2r = []
        for k in ans["l2g_mapping"]:
            l2r.append(g2r[k])
        ans["l2r_mapping"] = np.array(l2r)
        return ans
    
    def get_relation_meta(self, from_end, to_end):
        ans = {}
        ans["from_order"] = from_end
        ans["to_order"] = to_end
        # self.patcher.set_relation(from_end, to_end, self.get_relation(from_end, to_end).csr)
        csr = self.patcher.get_relation_meta(from_end, to_end)
        ans["offset"], ans["value"] = csr.offset, csr.value
        ans["patch_offset"] = self.patcher.get_patch_offset(from_end, to_end)
        return ans
        # ans = {}
        # ans["from_order"] = from_end
        # ans["to_order"] = to_end
        # total_from = self.total[from_end] if from_end > to_end else self.owned[from_end]
        # total_to = self.total[to_end]
        # relation = self.get_relation(from_end, to_end)
        # offset = []
        # value = []
        # patch_off = [0]
        # for i in range(len(total_from)):
        #     d = {}
        #     for j, u in enumerate(total_to[i]):
        #         d[u] = j
        #     matrix = []
        #     for u in total_from[i]:
        #         l = []
        #         for v in relation[u]:
        #             l.append(d[v] if v in d else 0)
        #         matrix.append(l)
        #     rel = Relation(matrix)
        #     offset = offset + list(rel.offset)
        #     value = value + list(rel.value)
        #     patch_off.append(patch_off[-1] + rel.offset[-1])
        # ans["offset"], ans["value"] = np.array(offset), np.array(value)
        # ans["patch_offset"] = np.array(patch_off[:-1])
        # return ans
    
    def get_meta(self, relations):
        c2i = {'v': 0, 'e': 1, 'f': 2, 'c': 3}
        ans = {}
        # ans["num_patches"] = len(self.cluster.patch)
        ans["num_patches"] = len(self.owned[-1])
        ans["elements"] = []
        for order in range(self.n_order):
            ans["elements"].append(self.get_element_meta(order))
        ans["relations"] = []
        for relation in relations:
            if isinstance(relation, str):
                relation = (c2i[relation[0]], c2i[relation[1]])
            ans["relations"].append(self.get_relation_meta(*relation))
        ans["attrs"] = {'x': self.position.reshape(-1).astype(np.float32)}
        return ans

            