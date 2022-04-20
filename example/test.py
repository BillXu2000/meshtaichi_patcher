import taichi as ti
import meshtaichi_patcher

obj_name = "/media/hdd/model/scale/bunny/bunny0.obj"

ti.init()
mesh = ti.TetMesh()
meta = meshtaichi_patcher.mesh2meta(obj_name)
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
