from .relation import Relation
import numpy as np

class Mesh:
    def __init__(self, mesh):
        self.position = mesh.vertex_matrix()
        self.n_order = 3
        self.relation = {(2, 0): Relation(2, 0, mesh.face_matrix())}
        if len(mesh.edge_matrix()):
            self.relation[(1, 0)] = Relation(1, 0, mesh.edge_matrix())
        self.generate_elements()
    
    def get_size(self, order):
        return len(self.relation[(order, 0)].matrix) if order > 0 else len(self.position)
    
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
            self.relation[(order, 0)] = Relation(order, 0, np.array(relation))

    def get_relation(self, from_end, to_end):
        if (from_end, to_end) in self.relation:
            return self.relation[(from_end, to_end)]
        if from_end < to_end:
            self.relation[(from_end, to_end)] = self.get_relation(to_end, from_end).transpose()
            return self.relation[(from_end, to_end)]
        if from_end == to_end == 0:
            self.relation[(from_end, to_end)] = self.get_relation(0, 1).mul(self.get_relation(1, 0)).remove_self_loop()
            return self.relation[(from_end, to_end)]
        if from_end == to_end:
            self.relation[(from_end, to_end)] = self.get_relation(from_end, from_end - 1).mul(self.get_relation(from_end - 1, from_end)).remove_self_loop()
            return self.relation[(from_end, to_end)]
        # from_end > to_end
        ele_dict = {}
        for i, u in enumerate(self.relation[(to_end, 0)]):
            ele_dict[tuple(sorted(u))] = i
        relation = []
        for i, u in enumerate(self.relation[(from_end, 0)]):
            relation.append([])
            for v in Mesh.get_subsets(u):
                v = tuple(sorted(v))
                if len(v) != to_end + 1:
                    continue
                j = ele_dict[v]
                relation[i].append(j)
        self.relation[(from_end, to_end)] = Relation(from_end, to_end, np.array(relation))
        return self.relation[(from_end, to_end)]
    
    def patch(self, cluster):
        self.owned = []
        for order in range(self.n_order - 1):
            color = np.zeros([self.get_size(order)], dtype=np.int32)
            relation = self.get_relation(self.n_order - 1, order)
            for u in range(self.get_size(self.n_order - 1)):
                for v in relation[u]:
                    color[v] = cluster.color[u]
            self.owned.append([[] for i in cluster.patch])
            owned = self.owned[-1]
            for v, c in enumerate(color):
                owned[c].append(v)
            print(order, [len(i) for i in owned])
        owned = None
        self.owned.append(cluster.patch)
        self.total = []
        for order in range(self.n_order):
            total = []
            for c, p in enumerate(cluster.patch):
                s = set(self.owned[order][c])
                rel_0 = self.get_relation(self.n_order - 1, 0)
                rel_1 = self.get_relation(0, order)
                l = []
                for u in p:
                    for v in rel_0[u]:
                        for w in rel_1[v]:
                            if w not in s:
                                s.add(w)
                                l.append(w)
                total.append(self.owned[order][c] + l)
            self.total.append(total)
            print(order, [len(i) for i in total])
