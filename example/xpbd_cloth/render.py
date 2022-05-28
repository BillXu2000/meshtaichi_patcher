import taichi as ti

def initScene(position, lookat, show_window=True):
    global __show_window
    __show_window = show_window

    global scene, window, camera, canvas
    window = ti.ui.Window("XPBD Cloth", (1024, 768), vsync=True, show_window=show_window)
    canvas = window.get_canvas()
    scene = ti.ui.Scene()
    camera = ti.ui.make_camera()
    camera.position(position[0], position[1], position[2])
    camera.up(0, 0, 1.0)
    camera.lookat(lookat[0], lookat[1], lookat[2])
    camera.fov(55)

def renderScene(solver, frame):
    global scene, window, camera, canvas
    global __show_window
    camera.track_user_inputs(window, movement_speed=0.03, hold_key=ti.ui.RMB)
    scene.set_camera(camera)
    scene.ambient_light((0.6, 0.6, 0.6))
    scene.point_light(pos=(0.5, 1.5, 0.5), color=(1, 1, 1))
    scene.point_light(pos=(0.5, 1.5, 1.5), color=(1, 1, 1))
    solver.sdf.render(scene)
    scene.mesh(solver.model.verts.x, solver.indices, color=(0.5, 0.5, 0.5), two_sided=True)
    canvas.scene(scene)
    if __show_window:
        window.show()
    else:
        window.write_image(f"results/frame/{frame:06d}.jpg")
