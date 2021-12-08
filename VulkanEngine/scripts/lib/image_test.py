import engine
import math
import time

def update_array(w,h,array,array2):
    total = 0
    for i in range(len(array)):
        for j in range(len(array)):
            array[i,j]=[array2[total][0], array2[total][1], array2[total][2]]
            total+=1
            
    return array