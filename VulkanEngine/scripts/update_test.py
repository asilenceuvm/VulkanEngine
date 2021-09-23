import engine
x = 0
def update():
    global x
    x += 0.01
    engine.change_translation('cube00', x, 0, 0)