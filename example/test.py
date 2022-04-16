import taichi as ti
import meshtaichi_patcher as patcher

obj_name = "./bunny/bunny7.obj"
# obj_name = "test.obj"
# relations = ['ev', 'vv', 'ee']
relations = ['fv', 'ev', 'vv', 've', 'vf']
# relations = ['vv']

ti.init()
mesh = ti.TetMesh()
mesh.edges.link(mesh.verts)
mesh.verts.link(mesh.verts)
meta = patcher.mesh2meta(obj_name, relations)
# meta = patcher.meta_test(obj_name, relations)
bunny = mesh.build(meta)

@ti.kernel
def ra():
    sum = 0
    for i in bunny.edges:
        # print('patch id', i.id, ti.mesh_patch_idx())
        for j in i.verts:
            # print('ev', i.id, j.id)
            sum += j.id
    print(sum)
    sum = 0
    for i in bunny.verts:
        # print(i.id, i.verts.size)
        for j in range(i.verts.size):
            sum += i.verts[j].id
        #     print(i.id, i.verts[j].id)
    print(sum)
ra()
