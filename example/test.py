import taichi as ti
import meshtaichi_patcher, pymeshlab

# obj_name = "/media/hdd/model/scale/bunny/bunny0.obj"
# obj_name = "/media/hdd/model/scale/bunny/bunny0.1.node"
obj_name = "/home/bx2k/models/bunny/bunny0.obj"

ti.init()

meshes = []
for i in range(3):
    mesh = meshtaichi_patcher.load_mesh_rawdata(obj_name)
    mesh[0] += i * 200 # mesh[0] is the positions of vertices
    meshes.append(mesh)


mesh = ti.TetMesh()
# meta = meshtaichi_patcher.mesh2meta(obj_name)
vec3f = ti.types.vector(3, ti.f32)
mesh.verts.place({'x' : vec3f}) 
# meta = meshtaichi_patcher.mesh2meta(meshes, patch_size=512, relation=['ff'], patch_relation=['ff'])
meta = meshtaichi_patcher.mesh2meta(meshes, patch_size=512, patch_relation=['fv', 'vf'])
bunny = mesh.build(meta)
patcher = meta.patcher
stats = patcher.stats('out.svg')
print(stats)
exit(0)


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
    sum_f = 0.0
    for i in bunny.verts:
        # print(i.id, i.verts.size)
        for j in range(i.verts.size):
            sum_f += i.verts[j].x.norm()
        #     print(i.id, i.verts[j].id)
    print(sum_f)
ra()

# meta.patcher.export_obj()
ms = pymeshlab.MeshSet()
ms.add_mesh(pymeshlab.Mesh(vertex_matrix=bunny.verts.x.to_numpy(), face_matrix=bunny.patcher.face))
ms.save_current_mesh('/home/bx2k/transport/test.obj', binary=False, save_vertex_quality=False)