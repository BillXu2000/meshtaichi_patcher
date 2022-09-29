import taichi as ti
import meshtaichi_patcher, pymeshlab

# obj_name = "/media/hdd/model/scale/bunny/bunny0.obj"
# obj_name = "/media/hdd/model/scale/bunny/bunny0.1.node"
# obj_name = "/home/bx2k/models/bunny/bunny0.obj"
# mesh_name = "/home/bx2k/models/Armadillo_ascii.ply"
mesh_name = '/media/hdd/model/data/Armadillo/tet/after/Armadillo_ascii.1.node'

ti.init()

def get_stats(cluster='greedy', patch_size=512):
    mesh = ti.TetMesh()
    # meta = meshtaichi_patcher.mesh2meta(obj_name, cluster_option=cluster, patch_size=512, patch_relation=['fv', 'vf'])
    meta = meshtaichi_patcher.mesh2meta(mesh_name, cluster_option=cluster, patch_size=patch_size, patch_relation=['ev', 've'], cache=True)
    # bunny = mesh.build(meta)
    patcher = meta.patcher
    # stats = patcher.stats(cluster + '.svg')
    stats = patcher.stats()
    # patcher.export_obj(cluster + '.obj', face=True)
    return stats

for cluster in ['greedy', 'kmeans']:
    sizes = list(range(512, 1024, 128)) + list(range(1024, 3100, 512))
    for patch_size in sizes:
        stats = get_stats(cluster=cluster, patch_size=patch_size)
        ribbon_ratio = 1 - stats['owned_ratio'][0]
        print(cluster, patch_size, ribbon_ratio)
