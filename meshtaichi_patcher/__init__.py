from meshtaichi_patcher_core import read_tetgen
import json, time, numpy as np
import taichi as ti
import pprint, re, os, subprocess, hashlib
from . import meshpatcher, relation
from os import path

def load_mesh(meshes, relations=[], 
              patch_size=256, 
              cache=False, 
              cluster_option="greedy", 
              max_order=-1, 
              refresh_cache=False, 
              patch_relation="all", 
              debug=False) -> ti.MeshInstance:
    """Create a triangle mesh (a set of vert/edge/face elements) \
       or a tetrahedron mesh (a set of vert/edge/face/cell elements), \
       and initialize its connectivity.

        Args:
            meshes: a string of model file patch, e.g., `"bunny.obj"`.
            relations: a list of relations, e.g, `['FV', 'VV']`, where `'V'` for Vertex, `'E'` for Edge, `'F'` for Face, and `'C'` for Cell.
            cache: `True` if uses metadata caching.

        Returns:
            A MeshInstance.

        Example::
        >>> import meshtaichi_patcher as Patcher
        >>> mesh = Patcher.load_mesh("bunny.obj", relations=['FV']) # load model 'bunny.obj' as triangle mesh and initialize Face-Vert relation.
    """
    return ti.Mesh._create_instance(mesh2meta(meshes, 
            relations,
            patch_size,
            cache, 
            cluster_option, 
            max_order, 
            refresh_cache, 
            patch_relation,
            debug))


def mesh2meta_cpp(filename, relations):
    patcher = Patcher()
    start = time.time()
    patcher.run(filename, relations)
    #print('run time:', time.time() - start)
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
            relation["patch_offset"] = patcher.get_relation_arr("patch_offset", from_order * 4 + to_order)
            relation["offset"] = patcher.get_relation_arr("offset", from_order * 4 + to_order)
    data["attrs"]["x"] = patcher.get_mesh_x().reshape(-1)
    start = time.time()
    # pp = pprint.PrettyPrinter(indent=4)
    # pp.pprint(data)
    meta = ti.Mesh.generate_meta(data)
    return meta

def mesh2meta(meshes, relations=[], patch_size=256, cache=False, cluster_option="greedy", max_order=-1, refresh_cache=False, patch_relation="all", debug=False):
    if isinstance(meshes, str):
        mesh_name = meshes
    if not isinstance(meshes, list):
        # meshes = [meshes]
        total = meshes
    else:
        total = {0: []}
        for mesh in meshes:
            for i in mesh:
                if i == 0:
                    pos = mesh[i]
                else:
                    if i not in total:
                        total[i] = []
                    total[i] += list(mesh[i] + len(total[0]))
            total[0] += list(pos)
        for i in total:
            total[i] = np.array(total[i])
    if cache:
        base_name, ext_name = re.findall(r'^(.*)\.([^.]+)$', mesh_name)[0]
        if ext_name in ['node', 'ele']:
            name = f'{base_name}.node {base_name}.ele'
        else:
            name = meshes
        result = subprocess.run(f'cat {name} | sha256sum', shell=True, capture_output=True)
        sha = re.findall(r'\S+', result.stdout.decode('utf-8'))[0]
        cache_name = f'{sha}_{patch_size}_{cluster_option}_{max_order}_{"".join(patch_relation)}'
        cache_sha = hashlib.sha256(cache_name.encode('utf-8')).hexdigest()
        cache_path = path.expanduser(f'~/.patcher_cache/{cache_sha}')
        if path.exists(cache_path) and not refresh_cache:
            return load_meta(cache_path, relations)
    if isinstance(total, str):
        total = load_mesh_rawdata(total)
    m = meshpatcher.MeshPatcher(total)
    m.patcher.patch_size = patch_size
    m.patcher.cluster_option = cluster_option
    m.patcher.debug = debug
    m.patch(max_order, patch_relation)
    meta = m.get_meta(relations)
    if cache:
        if not path.exists(cache_path):
            cache_folder = path.expanduser(f'~/.patcher_cache')
            subprocess.run(f'mkdir -p {cache_folder}', shell=True)
            m.write(cache_path)
    meta = ti.Mesh.generate_meta(meta)
    return meta

def load_meta(filename, relations=[]):
    assert path.exists(filename)
    m = meshpatcher.MeshPatcher()
    m.read(filename)
    meta = m.get_meta(relations)
    meta = ti.Mesh.generate_meta(meta)
    return meta

def save_meta(filename, meta):
    meta.patcher.write(filename)

def patched_mesh(filename):
    mesh = ti.TetMesh()
    meta = mesh2meta(filename)
    return mesh.build(meta)

def load_mesh_rawdata(filename):
    base_name, ext_name = re.findall(r'^(.*)\.([^.]+)$', filename)[0]
    if ext_name in ['node', 'ele']:
        ans = {}
        # def name2np(filename, type, size):
        #     if not os.path.exists(filename):
        #         return None
        #     lists = []
        #     with open(filename, 'r') as fi:
        #         lines = fi.readlines()[1: -1]
        #     for line in lines:
        #         numbers = re.findall(r'\S+', line)
        #         lists.append([type(i) for i in numbers[1: size + 1]])
        #     return np.array(lists)
        # ans[0] = name2np(f'{base_name}.node', float, 3)
        # ans[3] = name2np(f'{base_name}.ele', int, 4)
        # ans["face"] = name2np(f'{base_name}.face', int, 3)
        ans[0] = read_tetgen(f'{base_name}.node')[0].reshape(-1, 3)
        ans[3] = read_tetgen(f'{base_name}.ele')[0].reshape(-1, 4)
        ans["face"] = read_tetgen(f'{base_name}.face')[0].reshape(-1, 3)
    else:
        ans = {}
        import importlib.util
        if importlib.util.find_spec('meshio'):
            import meshio
            m = meshio.read(filename)
            ans[0] = m.points
            for cell in m.cells:
                if cell.type == 'triangle':
                    ans[2] = cell.data
        else:
            import pymeshlab
            ml_ms = pymeshlab.MeshSet()
            ml_ms.load_new_mesh(filename)
            ml_m = ml_ms.current_mesh()
            ans[0] = ml_m.vertex_matrix()
            if len(ml_m.edge_matrix()):
                ans[1] = ml_m.edge_matrix()
            ans[2] = ml_m.face_matrix()
    return ans

