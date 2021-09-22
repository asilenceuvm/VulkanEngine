# file to handle python side engine start update
# imports for convenince
import engine

import numpy as np
import pandas as pd

import sys


def startup():
    # point to wherever python scripts are
    sys.path.append('./scripts')
    return 0