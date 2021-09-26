import engine

x = 0

def update():
    global x
    x += 1
    tags = engine.get_tags()
    for i in range(len(tags)):
        engine.change_rotation(tags[i].decode('UTF-8'), x, 0, 0)
    #engine.change_rotation('cube01', x, 0, 0)