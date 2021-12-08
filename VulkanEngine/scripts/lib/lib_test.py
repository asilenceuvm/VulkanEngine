import engine
import math
import time

def test(obj):
    for i in range(1000000):
        engine.change_rotation(obj, 0, 90 * math.sin(time.time()), 0)
        #engine.change_scale(obj, i / 100, i / 100, i / 100)