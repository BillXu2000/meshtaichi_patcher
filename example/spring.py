import taichi as ti, sys, os, argparse, subprocess, re, numpy as np, time
import meshtaichi_patcher as patcher

parser = argparse.ArgumentParser()
parser.add_argument('--mesh', default='./bunny/bunny0.obj')
parser.add_argument('--save_obj', action='store_true')
parser.add_argument('--num_run', type=int, default=20000000)
parser.add_argument('--exp', default='ev')
parser.add_argument('--opt', default='mesh')
parser.add_argument('--arch', default='cuda')
parser.add_argument('--thread', type=int, default=8)
parser.add_argument('--gui', action='store_true')
parser.add_argument('--block_dim', type=int, default=512)
parser.add_argument('--reorder', action='store_true')
args = parser.parse_args()

if args.exp == 'implicit': 
    args.implicit = True
    args.exp = 'vv'
else: args.implicit = False

ti.init(arch=ti.cuda if args.arch == 'cuda' else ti.cpu, kernel_profiler=(args.arch == 'cpu'), device_memory_fraction=0.7, cpu_max_num_threads=args.thread)

vec3f = ti.types.vector(3, ti.f32)

mesh = ti.TetMesh()
mesh.verts.place({'x' : vec3f, 
                  'ox' : vec3f, 
                  'v' : vec3f, 
                  'm' : ti.f32, 
                  'f' : vec3f}, reorder = args.reorder)

mesh.edges.place({'rest_len' : ti.f32}, reorder = args.reorder)

mesh.verts.link(mesh.verts)
mesh.edges.link(mesh.verts)
meta = patcher.meta_test(args.mesh, ['ev', 'vv'])
bunny = mesh.build(meta)

x = bunny.verts.x.to_numpy()
for i in range(x.shape[1]):
    x[:, i] -= x[:, i].min()
x /= x[:, 1].max()
bunny.verts.x.from_numpy(x)

x = bunny.verts.x
bunny.verts.ox.copy_from(bunny.verts.x)
mass = 1.0 / bunny.verts.x.shape[0]
bunny.verts.m.fill(mass)


bottom = 0.1
stiffness = 1e1
dt = 2e-5

edges = ti.Vector.field(2, dtype=ti.i32, shape=bunny.edges.rest_len.shape)
offset = ti.field(dtype=ti.i32, shape=(bunny.verts.x.shape[0] + 1,))
dst = ti.field(dtype=ti.i32, shape=bunny.edges.rest_len.shape[0] * 2)
raw_len = ti.field(dtype=ti.f32, shape=bunny.edges.rest_len.shape[0] * 2)

@ti.kernel
def ev_substep():
    ti.mesh_local(bunny.verts.x, bunny.verts.f, bunny.verts.ox)
    #if args.arch == 'cuda': ti.block_dim(args.block_dim) # TODO: why?
    ti.block_dim(args.block_dim)
    for e in bunny.edges:
        v0 = e.verts[0]
        v1 = e.verts[1]
        d = v0.x - v1.x
        force = stiffness * (d.norm() - (v0.ox - v1.ox).norm()) * d.normalized()
        v0.f -= force
        v1.f += force

@ti.kernel
def vv_substep():
    ti.block_dim(args.block_dim)
    ti.mesh_local(bunny.verts.x, bunny.verts.ox)
    for v0 in bunny.verts:
        total_f = ti.Vector([0.0, 0.0, 0.0])
        for i in range(v0.verts.size):
            v1 = v0.verts[i]
            d = v0.x - v1.x
            force = stiffness * (d.norm() - (v0.ox - v1.ox).norm()) * d.normalized()
            total_f -= force
        v0.f = total_f

@ti.kernel
def raw_ev():
    ti.block_dim(args.block_dim)
    for e in edges:
        v0 = edges[e][0]
        v1 = edges[e][1]
        d = bunny.verts.x[v0] - bunny.verts.x[v1]
        force = stiffness * (d.norm() - (bunny.verts.ox[v0] - bunny.verts.ox[v1]).norm()) * d.normalized()
        bunny.verts.f[v0] -= force
        bunny.verts.f[v1] += force

@ti.kernel
def raw_vv():
    ti.block_dim(args.block_dim)
    for v0 in bunny.verts.x:
        total_f = ti.Vector([0.0, 0.0, 0.0])
        my_ox = bunny.verts.ox[v0]
        my_x = bunny.verts.x[v0]
        for i in range(offset[v0], offset[v0 + 1]):
            v1 = dst[i]
            d = my_x - bunny.verts.x[v1]
            force = stiffness * (d.norm() - (my_ox - bunny.verts.ox[v1]).norm()) * d.normalized()
            total_f -= force
        bunny.verts.f[v0] = total_f

@ti.kernel
def advance():
    for v0 in bunny.verts:
        if v0.x[1] >= bottom:
            v0.v += dt * (v0.f / v0.m + ti.Vector([0.0, 9.8, 0.0]))
            v0.f = ti.Vector([0.0] * 3)
            v0.x += v0.v * dt

@ti.kernel
def advance_im():
    for v0 in bunny.verts:
        v0.f = ti.Vector([0.0] * 3)
        if v0.x[1] >= bottom:
            v0.x += v0.v * dt
        else:
            v0.v = ti.Vector([0.0] * 3)

@ti.kernel
def matmul_mesh(ret: ti.template(), x: ti.template()):
    ti.mesh_local(bunny.verts.x, x, bunny.verts.ox)
    for u in bunny.verts:
        #if u.ox[1] >= bottom:
        if True:
            sum = ti.Vector.zero(ti.f32, 3)
            for i in range(u.verts.size):
                v = u.verts[i]
                x_uv = u.x - v.x
                l = (u.ox - v.ox).norm()
                df = -stiffness * l * (ti.Matrix.identity(ti.f32, 3) * (1 - l / x_uv.norm()) + x_uv @ x_uv.transpose() * l / x_uv.norm()**3)
                #if v.ox[1] >= bottom: sum += df @ (x[u.id] - x[v.id])
                #else: sum += df @ x[u.id]
                sum += df @ (x[u.id] - x[v.id])

            ret[u.id] = (x[u.id] - dt**2 / mass * sum)
        else: ret[u.id] = x[u.id]

@ti.kernel
def matmul_raw(ret: ti.template(), x: ti.template()):
    for u in bunny.verts.x:
        #if bunny.verts.ox[u][1] >= bottom:
        if True:
            sum = ti.Vector.zero(ti.f32, 3)
            for i in range(offset[u], offset[u + 1]):
                v = dst[i]
                x_uv = bunny.verts.x[u] - bunny.verts.x[v]
                l = (bunny.verts.ox[u] - bunny.verts.ox[v]).norm()
                df = -stiffness * l * (ti.Matrix.identity(ti.f32, 3) * (1 - l / x_uv.norm()) + x_uv @ x_uv.transpose() * l / x_uv.norm()**3)
                #if bunny.verts.ox[v][1] >= bottom: sum += df @ (x[u] - x[v])
                #else: sum += df @ x[u]
                sum += df @ (x[u] - x[v])

            ret[u] = (x[u] - dt**2 / mass * sum)
        else: ret[u] = x[u]

@ti.kernel
def add(ans: ti.template(), a: ti.template(), k: ti.f32, b: ti.template()):
    for i in ans:
        ans[i] = a[i] + k * b[i]
    
@ti.kernel
def dot(a: ti.template(), b: ti.template()) -> ti.f32:
    ans = 0.0
    ti.block_dim(32)
    for i in a:
        ans += a[i].dot(b[i])
    return ans

@ti.kernel
def fix_gravity():
    for u in bunny.verts:
        u.f[1] += 9.8 * u.m

x0 = bunny.verts.v
b = ti.Vector.field(3, dtype=ti.f32, shape=x0.shape)
r0 = ti.Vector.field(3, dtype=ti.f32, shape=x0.shape)
p0 = ti.Vector.field(3, dtype=ti.f32, shape=x0.shape)
mul_ans = ti.Vector.field(3, dtype=ti.f32, shape=x0.shape)
tmp0 = ti.field(dtype=ti.f32, shape=x0.shape[0] * 3)
tmp1 = ti.field(dtype=ti.f32, shape=x0.shape[0] * 3)
def cg(matmul):
    def mul(x):
        matmul(mul_ans, x)
        return mul_ans
    add(b, x0, dt / mass, bunny.verts.f)
    add(r0, b, -1, mul(x0))
    p0.copy_from(r0)
    r_2 = dot(r0, r0)
    n_iter = 10
    hack_eps = 1e-9
    for iter in range(n_iter):
        A_p0 = mul(p0)
        dot_ans = dot(p0, A_p0)
        if dot_ans < hack_eps: alpha = hack_eps
        else: alpha = r_2 / dot_ans
        add(x0, x0, alpha, p0)
        add(r0, r0, -alpha, A_p0)
        r_2_new = dot(r0, r0)
        if r_2 < hack_eps: beta = hack_eps
        else: beta = r_2_new / r_2
        add(p0, r0, beta, p0)
        r_2 = r_2_new
    print(iter, r_2_new)

if args.gui:
    res = (1024, 768)
    window = ti.ui.Window("Mass Spring", res, vsync=False)

    frame_id = 0
    canvas = window.get_canvas()
    scene = ti.ui.Scene()
    camera = ti.ui.make_camera()
    camera.position(-2, 2, -1)
    camera.up(0, 1, 0)
    camera.lookat(0, 0, 0)
    camera.fov(75)

def reset():
    global bunny
    bunny.verts.x.copy_from(bunny.verts.ox)
    bunny.verts.v.fill([0.0, 0.0, 0.0])
    bunny.verts.f.fill([0.0, 0.0, 0.0])

reset()

@ti.kernel
def calcRestlen():
    for e in bunny.edges:
        e.rest_len = (e.verts[0].x - e.verts[1].x).norm()
        edges[e.id] = [e.verts[0].id, e.verts[1].id]

calcRestlen()

if args.opt == 'raw' and args.exp == 'ev':
    edge_np = edges.to_numpy()
    edge_arg = np.argsort(edge_np[:, 0])
    edge_np_copy = np.copy(edge_np)
    for i in range(edge_np.shape[0]):
        edge_np[i] = edge_np_copy[edge_arg[i]]
    edges.from_numpy(edge_np)

if args.opt == 'raw' and args.exp == 'vv':
    off = offset
    tmp = ti.field(dtype=ti.i32, shape=off.shape)
    outer = 'verts'
    inner = 'verts'
    word2dim = {'verts': 0, 'edges': 1, 'faces': 2, 'cells': 3}

    for i in meta.element_fields:
        j = meta.element_fields[i]
        if i.value == word2dim[outer]: g2r_o = j['g2r']
        if i.value == word2dim[inner]: g2r_i = j['g2r']
    if args.reorder == False:
        g2r_o = None

    @ti.kernel
    def get_off():
        off[0] = 0
        for i in getattr(bunny, outer):
            if ti.static(g2r_o == None): off[i.id + 1] = getattr(i, inner).size
            else: off[g2r_o[i.id] + 1] = getattr(i, inner).size
    get_off()

    off_np = off.to_numpy()[1:]
    if off_np.max() == off_np.min(): off_size = off_np.max()
    else: off_size = None

    @ti.kernel
    def naive(d: ti.i32):
        for i in off:
            if i >= 1 << d: tmp[i] = off[i] + off[i - (1 << d)]
            else: tmp[i] = off[i]
        for i in off:
            off[i] = tmp[i]

    def naive_prefixsum():
        d = 0
        while 1 << d < off.shape[0]:
            naive(d)
            d += 1
    naive_prefixsum()

    dst = ti.field(dtype=ti.i32, shape=off[off.shape[0] - 1])

    @ti.kernel
    def get_dst():
        for i in getattr(bunny, outer):
            for j in range(getattr(i, inner).size):
                if ti.static(g2r_o == None):
                    dst[off[i.id] + j] = getattr(i, inner)[j].id
                else:
                    dst[off[g2r_o[i.id]] + j] = g2r_i[getattr(i, inner)[j].id]
    get_dst()

@ti.kernel
def calcRawlen():
    for v0 in x:
        for i in range(offset[v0], offset[v0 + 1]):
            v1 = dst[i]
            raw_len[i] = (x[v0] - x[v1]).norm()
calcRawlen()

num_step = 100
timer = 0.0
for frame in range(args.num_run):
    start = time.time()
    if args.implicit:
        for i in range(num_step):
            if args.opt == 'mesh':
                vv_substep()
                fix_gravity()
                cg(matmul_mesh)
            else:
                raw_vv()
                fix_gravity()
                cg(matmul_raw)
            advance_im()
    else:
        for i in range(num_step):
            if args.opt == 'mesh':
                if args.exp == 'ev':
                    ev_substep()
                else: 
                    vv_substep()
            else:
                if args.exp == 'ev':
                    raw_ev()
                else: 
                    raw_vv()
            advance()
    timer += time.time() - start
    if args.gui:
        if window.is_pressed('q'): break
        if window.is_pressed('r'): reset()

        camera.track_user_inputs(window, movement_speed=0.03, hold_key=ti.ui.RMB)
        scene.set_camera(camera)

        scene.ambient_light((0.5, 0.5, 0.5))

        #scene.mesh(bunny.verts.x, indices, color = (0.5, 0.5, 0.5))
        scene.particles(bunny.verts.x, 0.01)
        scene.point_light(pos=(0.5, 1.5, 0.5), color=(1, 1, 1))
        scene.point_light(pos=(0.5, 1.5, 1.5), color=(1, 1, 1))

        canvas.scene(scene)

        window.show()