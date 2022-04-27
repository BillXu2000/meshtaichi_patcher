import taichi as ti
import meshtaichi_patcher

# obj_name = "/media/hdd/model/scale/bunny/bunny0.obj"
obj_name = "/media/hdd/model/scale/bunny/bunny0.1.node"

ti.init()

meshes = []
for i in range(3):
    mesh = meshtaichi_patcher.load_mesh(obj_name)
    mesh[0] += i * 200 # mesh[0] is the positions of vertices
    meshes.append(mesh)


mesh = ti.TetMesh()
# meta = meshtaichi_patcher.mesh2meta(obj_name)
meta = meshtaichi_patcher.mesh2meta(meshes)
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

meta.patcher.export_obj('test.obj')