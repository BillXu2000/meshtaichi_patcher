import taichi as ti
import numpy as np
import random
import meshtaichi_patcher as Patcher

ti.init(arch=ti.cuda, device_memory_GB=4.0, packed = True)

model_size = 0.05
meshes = []

def model_height(i): return 0.05 + (model_size / 2 + 0.008) * i
def transform(verts, scale, offset): 
  return verts / max(verts.max(0) - verts.min(0)) * scale + offset
def add_mesh(x, y, i):
    x -= model_size / 2
    y -= model_size / 2
    mesh = Patcher.load_mesh("/home/bx2k/models/armadillo/armadillo0.1.node")
    mesh[0] = transform(mesh[0], model_size, [x, y, model_height(i)])
    meshes.append(mesh)

# from bridson import poisson_disc_samples

# samples = poisson_disc_samples(width=1,height=1, r=0.075, radius=0.035)
# while len(samples) != 90: 
#   samples = poisson_disc_samples(width=1,height=1, r=0.075, radius=0.035)
# samples = np.array(samples)

for k in range(1):
  for i in range(90):
    # add_mesh(samples[i, 0], samples[i, 1], k)
    add_mesh(random.random(), random.random(), k)

meta = Patcher.mesh2meta(meshes, patch_size=1024, relations=["cv"], patch_relation=[])
Patcher.save_meta("./cluster", meta)