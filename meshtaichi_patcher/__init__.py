from meshtaichi_patcher_core import *
import json, time, numpy as np

def mesh2meta(filename, relations):
    import taichi as ti
    patcher = Patcher()
    start = time.time()
    patcher.run(filename, relations)
    print('run time:', time.time() - start)
    data = json.loads(patcher.export_json())
    element_arrs = ["l2g_mapping", "l2r_mapping", "g2r_mapping", "owned_offsets", "total_offsets"]
    for element in data["elements"]:
        for arr in element_arrs:
            element[arr] = patcher.get_element_arr(arr, element["order"])
    for relation in data["relations"]:
        from_order = relation["from_order"]
        to_order = relation["to_order"]
        relation["value"] = patcher.get_relation_arr("value", from_order * 4 + to_order)
        if from_order <= to_order:
            relation["offset"] = patcher.get_relation_arr("offset", from_order * 4 + to_order)
    data["attrs"]["x"] = patcher.get_mesh_x().reshape(-1)
    start = time.time()
    meta = ti.Mesh.json2meta(data)
    print('json2meta time:', time.time() - start)
    return meta