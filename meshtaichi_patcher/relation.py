from meshtaichi_patcher_core import Csr_cpp
import numpy as np

class Relation:
    def __init__(self, matrix):
        if isinstance(matrix, Csr_cpp):
            csr = matrix
            self.csr = csr
            self.offset = csr.offset
            self.value = csr.value
            # print(self.offset, self.value)
        else:
            offset = [0] + [len(i) for i in matrix]
            for i in range(len(matrix)):
                offset[i + 1] += offset[i]
            self.offset = np.array(offset)
            value = []
            for i in matrix:
                for j in i:
                    value.append(j)
            self.value = np.array(value)
            self.csr = Csr_cpp(self.offset, self.value)
    
    def keys(self):
        return range(len(self.offset) - 1)
    
    def __len__(self):
        return len(self.offset) - 1

    def __getitem__(self, key):
        return self.value[self.offset[key]: self.offset[key + 1]]

    def transpose(self):
        return Relation(self.csr.transpose())

    def mul(self, relation):
        return Relation(self.csr.mul(relation.csr))
                    
    def remove_self_loop(self):
        return Relation(self.csr.remove_self_loop())