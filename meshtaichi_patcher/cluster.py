# from meshtaichi_patcher_core import Cluster_cpp
# from . import relation
# import random
# import numpy as np

# class Cluster:
#     def __init__(self, relation, patch_size):
#         self.relation = relation
#         self.patch_size = patch_size
    
#     def run(self):
#         cluster = Cluster_cpp()
#         cluster.patch_size = self.patch_size
#         # ans = relation.Relation(cluster.run(self.relation.csr))
#         ans = relation.Relation(cluster.run_greedy(self.relation.csr))
#         self.patch = ans