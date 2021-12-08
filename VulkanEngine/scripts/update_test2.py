import engine
import keyvals

x = 0

def update():    
    global x
    if engine.get_key_down(keyvals.GLFW_KEY_I):
        x += 0.01
        tags = engine.get_tags()
        for i in range(len(tags)):
            engine.change_scale(tags[i], x, x, x)
    if engine.get_key_down(keyvals.GLFW_KEY_K):
        x -= 0.01
        tags = engine.get_tags()
        for i in range(len(tags)):
            engine.change_scale(tags[i], x, x, x)
    #engine.change_rotation('cube01', x, 0, 0)