import numpy as np

class Relation:
    def __init__(self, from_end, to_end, matrix):
        self.from_end = from_end
        self.to_end = to_end
        lens = [len(i) for i in matrix]
        if (max(lens) == min(lens)):
            matrix = np.array(matrix)
        self.matrix = matrix
    
    def keys(self):
        return list(range(len(self.matrix)))

    def __getitem__(self, key):
        return self.matrix[key]

    def transpose(self):
        m_t = []
        for u in self.keys():
            for v in self[u]:
                while len(m_t) <= v:
                    m_t.append([])
                m_t[v].append(u)
        return Relation(self.to_end, self.from_end, m_t)

    def mul(self, relation):
        matrix = []
        for u in self.keys():
            for v in self[u]:
                for w in relation[v]:
                    while len(matrix) <= u:
                        matrix.append([])
                    matrix[u].append(w)
        return Relation(self.from_end, relation.to_end, matrix)
                    
    def remove_self_loop(self):
        matrix = []
        for u in self.keys():
            l = []
            for v in self[u]:
                if u != v:
                    l.append(v)
            matrix.append(l)
        return Relation(self.from_end, self.to_end, matrix)