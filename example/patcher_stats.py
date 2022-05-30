import taichi as ti
import meshtaichi_patcher, pymeshlab, cProfile, pstats, io
from pstats import SortKey

# obj_name = "/media/hdd/model/scale/bunny/bunny0.obj"
# obj_name = "/media/hdd/model/scale/bunny/bunny0.1.node"
obj_name = "/home/bx2k/models/bunny/bunny3.1.node"
# obj_name = "/home/bx2k/models/bunny/bunny3.obj"

pr = cProfile.Profile()
pr.enable()

ti.init()

mesh = ti.TetMesh()
# meta = meshtaichi_patcher.mesh2meta(obj_name)
vec3f = ti.types.vector(3, ti.f32)
mesh.verts.place({'x' : vec3f}) 
mesh.edges.place({'y' : vec3f}) 
# meta = meshtaichi_patcher.mesh2meta(obj_name, patch_size=1024, cache=True, cluster_option='kmeans', refresh_cache=True)
meta = meshtaichi_patcher.mesh2meta(obj_name, patch_size=1024, debug=True)
bunny = mesh.build(meta)

pr.disable()
# pr.print_stats(sort=SortKey.CUMULATIVE)
s = io.StringIO()
sortby = SortKey.CUMULATIVE
ps = pstats.Stats(pr, stream=s).sort_stats(sortby)
ps.print_stats()
l = s.getvalue().split('\n')
print('\n'.join(l[:10]))

# meta.patcher.stats('/home/bx2k/transport/kmeans3.svg')
# meta.patcher.export_obj('/home/bx2k/transport/kmeans_old.obj')
# meta.patcher.stats('/home/bx2k/transport/greedy3_tet.svg')
# meta.patcher.export_obj('/home/bx2k/transport/greedy3.obj')
# meta.patcher.stats('/home/bx2k/transport/unbound3_tet.svg')
# meta.patcher.stats('/home/bx2k/transport/kmeans3_tet.svg')

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