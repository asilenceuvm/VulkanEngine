import engine
x = 0
def update():
    global x
    x += 1
    engine.change_rotation('cube01', x, 0, 0)