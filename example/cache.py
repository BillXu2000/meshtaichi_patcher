import taichi as ti
import meshtaichi_patcher, pymeshlab

# obj_name = "/media/hdd/model/scale/bunny/bunny0.obj"
obj_name = "/media/hdd/model/scale/bunny/bunny0.1.node"

ti.init()

mesh = ti.TetMesh()
# meta = meshtaichi_patcher.mesh2meta(obj_name)
vec3f = ti.types.vector(3, ti.f32)
mesh.verts.place({'x' : vec3f}) 
meta = meshtaichi_patcher.mesh2meta(obj_name, patch_size=512) # use cache=True here to generate a metafile under ~/.patch_cache
meshtaichi_patcher.save_meta("./metafile", meta)
meta = meshtaichi_patcher.load_meta("./metafile")
bunny = mesh.build(meta)


@ti.kernel
def ra():
    sum = 0
    for i in bunny.edges:
        # print('patch id', i.id, ti.mesh_patch_idx())
        for j in i.verts:
            # print('ev', i.id, j.id)
            sum += j.x.norm()
    print(sum)
    sum = 0
    for i in bunny.verts:
        # print(i.id, i.verts.size)
        for j in range(i.verts.size):
            sum += i.verts[j].x.norm()
        #     print(i.id, i.verts[j].id)
    print(sum)
ra()

# meta.patcher.export_obj()
ms = pymeshlab.MeshSet()
ms.add_mesh(pymeshlab.Mesh(vertex_matrix=bunny.verts.x.to_numpy(), face_matrix=bunny.patcher.face))
ms.save_current_mesh('/home/bx2k/transport/test.obj', binary=False, save_vertex_quality=False)