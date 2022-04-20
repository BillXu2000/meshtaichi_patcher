from meshtaichi_patcher_core import Patcher_cpp
from .relation import Relation
import numpy as np

class MeshPatcher:
    def __init__(self, mesh):
        self.position = mesh.vertex_matrix()
        self.n_order = 3

        self.patcher = Patcher_cpp()
        self.patcher.n_order = self.n_order

        self.patcher.set_relation(2, 0, Relation(mesh.face_matrix()).csr)
        if len(mesh.edge_matrix()):
            self.patcher.set_relation(1, 0, Relation(mesh.face_matrix()).csr)
        self.patcher.generate_elements()
    
    def get_size(self, order):
        return self.patcher.get_size(order)

    def get_relation(self, from_end, to_end):
        return Relation(self.patcher.get_relation(from_end, to_end))
    
    def patch(self, cluster):
        self.patcher.patch(Relation(cluster.patch).csr)
        self.owned = []
        self.total = []
        for order in range(self.n_order):
            self.owned.append(Relation(self.patcher.get_owned(order)))
            self.total.append(Relation(self.patcher.get_total(order)))
    
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
        csr = self.patcher.get_relation_meta(from_end, to_end)
        ans["offset"], ans["value"] = csr.offset, csr.value
        ans["patch_offset"] = self.patcher.get_patch_offset(from_end, to_end)
        return ans
    
    def get_meta(self, relations):
        c2i = {'v': 0, 'e': 1, 'f': 2, 'c': 3}
        ans = {}
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
        ans["patcher"] = self
        return ans

            