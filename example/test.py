import taichi as ti
import meshtaichi_patcher as patcher

ti.init()
mesh = ti.TetMesh()
mesh.edges.link(mesh.verts)
patcher.meta_test("./bunny/bunny0.obj", ["ev"])
# meta = patcher.mesh2meta("./bunny0.obj", ["ev"])
# bunny = mesh.build(meta)

# @ti.kernel
# def ra():
#     sum = 0
#     for i in bunny.edges:
#         for j in i.verts:
#             sum += j.id
#     print(sum)
# ra()
