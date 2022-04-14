import numpy as np

class Relation:
    def __init__(self, matrix):
        offset = [0] + [len(i) for i in matrix]
        for i in range(len(matrix)):
            offset[i + 1] += offset[i]
        self.offset = np.array(offset)
        value = []
        for i in matrix:
            for j in i:
                value.append(j)
        self.value = np.array(value)
    
    def keys(self):
        return range(len(self.offset) - 1)
    
    def __len__(self):
        return len(self.offset) - 1

    def __getitem__(self, key):
        return self.value[self.offset[key]: self.offset[key + 1]]

    def transpose(self):
        m_t = []
        for u in self.keys():
            for v in self[u]:
                while len(m_t) <= v:
                    m_t.append([])
                m_t[v].append(u)
        return Relation(m_t)

    def mul(self, relation):
        matrix = []
        for u in self.keys():
            for v in self[u]:
                for w in relation[v]:
                    while len(matrix) <= u:
                        matrix.append([])
                    matrix[u].append(w)
        return Relation(matrix)
                    
    def remove_self_loop(self):
        matrix = []
        for u in self.keys():
            l = []
            for v in self[u]:
                if u != v:
                    l.append(v)
            matrix.append(l)
        return Relation(matrix)