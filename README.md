# meshtaichi patcher

generated from pybind/cmake_example.

## Prerequisites

* A compiler with C++11 support
* Pip 10+ or CMake >= 3.4 (or 3.8+ on Windows, which was the first version to support VS 2015)
* Ninja or Pip 10+
* Taichi


## Installation

```bash
git clone git@github.com:BillXu2000/meshtaichi_patcher.git
cd meshtaichi_patcher
git submodule update --init --recursive
python3 setup.py develop --user
```

## example

```python
import taichi as ti
import meshtaichi_patcher as patcher

ti.init()
mesh = ti.TetMesh()
mesh.edges.link(mesh.verts)
meta = patcher.mesh2meta("./bunny0.obj", ["ev"])
bunny = mesh.build(meta)

@ti.kernel
def ra():
    sum = 0
    for i in bunny.edges:
        for j in i.verts:
            sum += j.id
    print(sum)
ra()
```