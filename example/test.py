import taichi as ti
import meshtaichi_patcher as patcher

obj_name = "./bunny/bunny0.obj"
# obj_name = "test.obj"
# relations = ['ev', 'vv', 'ee']
relations = ['ev', 'vv']

ti.init()
mesh = ti.TetMesh()
mesh.edges.link(mesh.verts)
meta = patcher.meta_test(obj_name, relations)
# meta = patcher.mesh2meta(obj_name, relations)
bunny = mesh.build(meta)

@ti.kernel
def ra():
    sum = 0
    for i in bunny.edges:
        # print('patch id', i.id, ti.mesh_patch_idx())
        for j in i.verts:
            # print(i.id, j.id)
            sum += j.id
    print(sum)
    # sum = 0
    # for i in bunny.verts:
    #     # print('patch id', i.id, ti.mesh_patch_idx())
    #     for j in range(i.verts.size):
    #         # print(i.id, j.id)
    #         sum += i.verts[j].id
    # print(sum)
ra()
