import numpy as np
import engine
def multiply(a,b):
    print("Will compute", a, "times", b)
    c = 0
    for i in range(0, a):
        c = c + b
    return c
   
def sub(a,b):
    print("import test")
    return np.abs(a - b)
    
def move(f):
    engine.test_print(f, "test")
    