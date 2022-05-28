import taichi as ti
import meshtaichi_patcher as Patcher
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--cpu', action='store_true')
args = parser.parse_args()

ti.init(arch=ti.cpu if args.cpu else ti.cuda)

mass = 1.0
stiffness = 5e5
damping = 0.005
bottom_z = -70.0
dt = 2e-4
eps = 1e-6

mesh = ti.TetMesh()
mesh.verts.place({'x' : ti.math.vec3, 
                  'ox' : ti.math.vec3, 
                  'v' : ti.math.vec3, 
                  'f' : ti.math.vec3})

mesh.edges.place({'rest_len' : ti.f32})
armadillo = mesh.build(Patcher.mesh2meta("models/armadillo0.1.node", relations=["ev", "vv", "cv"]))

armadillo.verts.ox.copy_from(armadillo.verts.x)
armadillo.verts.v.fill([0.0, 0.0, -100.0])

@ti.kernel
def vv_substep():
    ti.mesh_local(armadillo.verts.x, armadillo.verts.ox)
    for v0 in armadillo.verts:
        v0.v *= ti.exp(-dt * damping)
        total_f = ti.Vector([0.0, -98.0, 0.0])
        
        for i in range(v0.verts.size):
            v1 = v0.verts[i]
            disp = v0.x - v1.x
            rest_disp = v0.ox - v1.ox
            total_f += -stiffness * (disp.norm(eps) - rest_disp.norm(eps)) * disp.normalized(eps)

        v0.v += dt * total_f

@ti.kernel
def ev_substep():
    for v in armadillo.verts:
        v.v *= ti.exp(-dt * damping)
        v.f = ti.Vector([0.0, -98.0, 0.0])

    ti.mesh_local(armadillo.verts.f, armadillo.verts.x)
    for e in armadillo.edges:
        v0 = e.verts[0]
        v1 = e.verts[1]
        disp0 = v0.x - v1.x
        spring_force = -stiffness * (disp0.norm() - e.rest_len) * disp0.normalized(eps)
        v0.f += spring_force
        v1.f -= spring_force

@ti.kernel
def advance():
    for v0 in armadillo.verts:
        v0.v += dt * v0.f
        if v0.x[1] < bottom_z:
            v0.x[1] = bottom_z
            v0.v[1] = -v0.v[1]
            v0.v[0] = 0
            v0.v[2] = 0
        v0.x += v0.v * dt

res = (1024, 768)
window = ti.ui.Window("Mass Spring", res, vsync=False)

frame_id = 0
canvas = window.get_canvas()
scene = ti.ui.Scene()
camera = ti.ui.make_camera()
camera.position(-200, -20, -130)
camera.up(0, 1, 0)
camera.lookat(0, -70, 0)
camera.fov(75)

indices = ti.field(ti.u32, shape = len(armadillo.cells) * 4 * 3)

@ti.kernel
def initIndices():
    for c in armadillo.cells:
        ind = [[0, 2, 1], [0, 3, 2], [0, 1, 3], [1, 2, 3]]
        for i in ti.static(range(4)):
            for j in ti.static(range(3)):
                indices[(c.id * 4 + i) * 3 + j] = c.verts[ind[i][j]].id

initIndices()

@ti.kernel
def calcRestlen():
    for e in armadillo.edges:
        e.rest_len = (e.verts[0].x - e.verts[1].x).norm()

calcRestlen()

while True:
    for i in range(25):
        advance()
        vv_substep()

    camera.track_user_inputs(window, movement_speed=0.03, hold_key=ti.ui.RMB)
    scene.set_camera(camera)
    scene.ambient_light((0.5, 0.5, 0.5))
    scene.mesh(armadillo.verts.x, indices, color = (0.5, 0.5, 0.5))
    scene.point_light(pos=(0.5, 1.5, 0.5), color=(1, 1, 1))
    scene.point_light(pos=(0.5, 1.5, 1.5), color=(1, 1, 1))
    canvas.scene(scene)
    window.show()