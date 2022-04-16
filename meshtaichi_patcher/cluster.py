from meshtaichi_patcher_core import Cluster_cpp
from . import relation
import random
import numpy as np

class Cluster:
    def __init__(self, relation, patch_size):
        self.relation = relation
        self.patch_size = patch_size
        self.color = np.zeros([len(relation)], dtype=np.int32)
        self.visit = np.zeros([len(self.color)], dtype=np.int32)
        random.seed(0)

    def coloring(self, seeds):
        for i in range(len(self.color)):
            self.color[i] = -1
        queue = []
        self.patch = []
        for i, u in enumerate(seeds):
            self.color[u] = i
            self.patch.append([u])
            queue.append(u)
        for u in queue:
            for v in self.relation[u]:
                if self.color[v] == -1:
                    self.color[v] = self.color[u]
                    self.patch[self.color[v]].append(v)
                    queue.append(v)
    
    def update_seed(self):
        new_seeds = set()
        self.visit[:] = -1
        for p in self.patch:
            queue = []
            for u in p:
                for v in self.relation[u]:
                    if self.color[u] != self.color[v]:
                        self.visit[u] = 0
                        queue.append(u)
                        break
            for u in queue:
                for v in self.relation[u]:
                    if self.color[u] == self.color[v] and self.visit[v] == -1:
                        self.visit[v] = 0
                        queue.append(v)
            new_seeds.add(u)
            if len(p) > self.patch_size and len(self.relation[u]):
                new_seeds.add(self.relation[u][0])
        return list(new_seeds)
    
    def run(self):
        cluster = Cluster_cpp()
        ans = relation.Relation(cluster.run(self.relation.csr))
        # color = [0] * len(self.relation)
        # patch = []
        # for i in ans.keys():
        #     patch.append(ans[i])
        #     for j in ans[i]:
        #         color[j] = i
        # self.color = color
        self.patch = ans