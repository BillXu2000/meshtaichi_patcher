import taichi as ti
import meshtaichi_patcher, pymeshlab

# obj_name = "/media/hdd/model/scale/bunny/bunny0.obj"
# obj_name = "/media/hdd/model/scale/bunny/bunny0.1.node"
# obj_name = "/home/bx2k/models/bunny/bunny0.1.node"
obj_name = "/home/bx2k/models/bunny/bunny0.obj"

ti.init()

mesh = ti.TetMesh()
# meta = meshtaichi_patcher.mesh2meta(obj_name)
vec3f = ti.types.vector(3, ti.f32)
mesh.verts.place({'x' : vec3f}) 
mesh.edges.place({'y' : vec3f}) 
meta = meshtaichi_patcher.mesh2meta(obj_name, patch_size=512)
bunny = mesh.build(meta)
meta.patcher.stats()