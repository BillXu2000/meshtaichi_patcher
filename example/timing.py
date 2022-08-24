import taichi as ti
import meshtaichi_patcher, pymeshlab, time, sys, os

# mesh_name = "/media/hdd/model/scale/bunny/bunny9.ply"
# obj_name = "/media/hdd/model/scale/bunny/bunny0.1.node"
# obj_name = "/home/bx2k/models/bunny/bunny0.obj"
# mesh_name = "/home/bx2k/models/Armadillo_ascii.ply"
# mesh_name = '/media/hdd/model/data/Armadillo/tet/after/Armadillo_ascii.1.node'

ti.init()

def get_timing(mesh):
    start = time.time()
    meta = meshtaichi_patcher.mesh2meta(mesh, patch_size=1024, patch_relation=[], cache=True, refresh_cache=True)
    cold = time.time() - start
    start = time.time()
    meta = meshtaichi_patcher.mesh2meta(mesh, patch_size=1024, patch_relation=[], cache=True)
    warm = time.time() - start
    return cold, warm

for mesh in sys.argv[1:]:
    print(mesh, os.path.basename(mesh), *get_timing(mesh))