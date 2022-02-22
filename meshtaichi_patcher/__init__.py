from meshtaichi_patcher_core import *
import json

def mesh2meta(filename, relations):
    import taichi as ti
    patcher = Patcher()
    patcher.run(filename, relations)
    data = json.loads(patcher.export_json())
    return ti.Mesh.json2meta(data)