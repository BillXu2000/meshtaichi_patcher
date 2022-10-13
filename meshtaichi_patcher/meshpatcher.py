from meshtaichi_patcher_core import Patcher_cpp
from .relation import Relation
import numpy as np, random

class MeshPatcher:
    def __init__(self, mesh=None):
        self.patcher = Patcher_cpp()
        self.n_order = 0
        self.face = None
        self.position = None
        if mesh == None:
            return
        for i in mesh:
            if isinstance(i, int):
                self.n_order = max(self.n_order, i + 1)
                if i == 0:
                    self.position = mesh[i]
                else:
                    self.patcher.set_relation(i, 0, Relation(mesh[i]).csr)
            if i == 2:
                self.face = mesh[i]
        if 'face' in mesh and mesh['face'] is not None:
            self.face = mesh['face']
        if self.face is not None:
            self.patcher.set_relation(2, -1, Relation(self.face).csr)
        if self.position is not None:
            self.patcher.set_pos(self.position.astype(np.float32).reshape(-1))
        self.patcher.n_order = self.n_order
        self.patched = False
    
    def write(self, filename):
        self.patcher.write(filename)
    
    def read(self, filename):
        self.patcher.read(filename)
        self.n_order = self.patcher.n_order
        # self.face = Relation(self.patcher.get_face()).to_numpy()
        self.position = self.patcher.get_pos().reshape(-1, 3)
        self.patched = True
    
    def get_size(self, order):
        return self.patcher.get_size(order)

    def get_relation(self, from_end, to_end):
        return Relation(self.patcher.get_relation(from_end, to_end))
    
    def patch(self, max_order, patch_relation):
        if self.patched: return
        self.patcher.generate_elements()
        if max_order != -1:
            self.patcher.n_order = max_order
            self.n_order = max_order
        if patch_relation == 'all':
            self.patcher.add_all_patch_relation()
            # for i in range(self.n_order):
            #     for j in range(self.n_order):
            #         self.patcher.add_patch_relation(i, j)
        else:
            c2i = {'v': 0, 'e': 1, 'f': 2, 'c': 3}
            for relation in patch_relation:
                self.patcher.add_patch_relation(c2i[relation[0]], c2i[relation[1]])
        self.patcher.patch()
        self.patched = True
    
    def get_element_meta(self, order):
        ans = {}
        ans["order"] = order
        ans["num"] = self.get_size(order)
        ans["max_num_per_patch"] = max([len(i) for i in self.total[order]])
        ans["owned_offsets"], tmp = self.owned[order].offset, self.owned[order].value
        ans["total_offsets"], ans["l2g_mapping"] = self.total[order].offset, self.total[order].value
        # g2r = [0] * self.get_size(order)
        # for i, k in enumerate(tmp):
        #     g2r[k] = i
        # ans["g2r_mapping"] = np.array(g2r, dtype=np.int32)
        # l2r = []
        # for k in ans["l2g_mapping"]:
        #     l2r.append(g2r[k])
        # ans["l2r_mapping"] = np.array(l2r, dtype=np.int32)
        # mapping = self.patcher.get_mapping(order)
        # print(mapping[0] - ans["g2r_mapping"])
        # print(mapping[1] - ans["l2r_mapping"])
        mapping = self.patcher.get_mapping(order)
        ans["g2r_mapping"] = mapping[0]
        ans["l2r_mapping"] = mapping[1]
        return ans
    
    def get_relation_meta(self, from_end, to_end):
        ans = {}
        ans["from_order"] = from_end
        ans["to_order"] = to_end
        csr = self.patcher.get_relation_meta(from_end, to_end)
        ans["offset"], ans["value"] = csr.offset.astype(np.uint16), csr.value.astype(np.uint16)
        ans["patch_offset"] = self.patcher.get_patch_offset(from_end, to_end)
        return ans
    
    def get_meta(self, relations):
        self.owned = []
        self.total = []
        for order in range(self.n_order):
            self.owned.append(Relation(self.patcher.get_owned(order)))
            self.total.append(Relation(self.patcher.get_total(order)))
        c2i = {'V': 0, 'E': 1, 'F': 2, 'C': 3}
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
    
    def export_obj(self, filename, vm=None, face=False, color=True, binary=False):
        import pymeshlab
        if vm is None:
            vm = self.position
        if self.face is None:
            self.face = Relation(self.patcher.get_face()).to_numpy()
        if not color:
            ms = pymeshlab.MeshSet()
            ms.add_mesh(pymeshlab.Mesh(vertex_matrix=vm, face_matrix=self.face))
            ms.save_current_mesh(filename, binary=binary, save_vertex_quality=False)
            return
        if face: # export face color
            face_colors = np.zeros([self.get_size(2), 4])
            for p in self.owned[2]:
                color = [random.random() for i in range(3)]
                color += [1]
                for u in p:
                    face_colors[u] = color
            ms = pymeshlab.MeshSet()
            ms.add_mesh(pymeshlab.Mesh(vertex_matrix=vm, face_matrix=self.face, f_color_matrix=face_colors))
        else:
            vert_colors = np.zeros([self.get_size(0), 4])
            for p in self.owned[0]:
                color = [random.random() for i in range(3)]
                color += [1]
                for u in p:
                    vert_colors[u] = color
            ms = pymeshlab.MeshSet()
            ms.add_mesh(pymeshlab.Mesh(vertex_matrix=vm, face_matrix=self.face, v_color_matrix=vert_colors))
        if filename[-4:] == '.obj':
            ms.save_current_mesh(filename)
        else:
            ms.save_current_mesh(filename, binary=binary, save_vertex_quality=False)
    
    def face_matrix(self):
        return self.face
    
    def stats(self, filename=None):
        import matplotlib.pyplot as plt
        # order = self.n_order - 1
        fig, axs = plt.subplots(nrows=2, ncols=self.n_order, figsize=(4 * self.n_order, 8))
        ans = {'total_max': [], 'owned_max': [], 'owned_ratio': []}
        for order in range(self.n_order):
            rate = self.owned[order].total_size() / self.total[order].total_size()
            # axs[0, order].violinplot([len(i) for i in self.owned[order]], showmeans=True)
            tmp_total = [len(i) for i in self.total[order]]
            rg = [0, max(tmp_total)]
            axs[0, order].hist([len(i) for i in self.owned[order]], range=rg)
            axs[0, order].set_title(f"owned, order = {order}")
            # axs[0, order].set_ylim(bottom=0)
            # axs[1, order].violinplot([len(i) for i in self.total[order]], showmeans=True)
            axs[1, order].hist(tmp_total, range=rg)
            axs[1, order].set_title(f"total, owned ratio = {'%.2f' % rate}")
            # axs[1, order].set_ylim(bottom=0)
            # axs[2, order].bar(0, rate, label='ribbon rate')
            # axs[2, order].set_ylim(top=1)
            # axs[2, order].set_title(f"owned rate = {'%.2f' % rate}")
            ans['total_max'].append(max(tmp_total))
            ans['owned_max'].append(max([len(i) for i in self.owned[order]]))
            # ans['owned_ratio'].append(float('%.3f' % rate))
            ans['owned_ratio'].append(rate)
            
        # plt.show()
        # plt.savefig('/home/bx2k/transport/patcher.svg')
        if filename is not None: plt.savefig(filename)
        return ans
    
    def get_owned_rate(self, order):
        return self.owned[order].total_size() / self.total[order].total_size()