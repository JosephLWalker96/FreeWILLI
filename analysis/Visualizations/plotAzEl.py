"""
 doa3d.py
 Plot azimuth and elevation of detected signals on a 3-dimensional sphere.
 
 WASD 2024-08-21 - Created doa3d.py
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt # Plotting library
import argparse
from matplotlib.colors import ListedColormap

parser = argparse.ArgumentParser(description='Program command line arguments')
parser.add_argument('--data', default='C:/Users/HARP/Documents/WAD/miniHARP/Data/2024-8-21-14-2-33-763493_doa', type=str)
args = parser.parse_args()
path_doa = args.data

# load and format data
colnames = ['t', 'en','az', 'el', 'x', 'y', 'z']
doa = pd.read_csv(path_doa, delim_whitespace = True, skiprows = 1, header = None, names = colnames)
doa['t_numeric'] = pd.to_numeric(doa['t'], errors = 'coerce')
doa['en'] = pd.to_numeric(doa['en'], errors = 'coerce')
doa['az'] = pd.to_numeric(doa['az'], errors = 'coerce')
doa['el'] = pd.to_numeric(doa['el'], errors = 'coerce')
doa['x'] = pd.to_numeric(doa['x'], errors = 'coerce')
doa['y'] = pd.to_numeric(doa['y'], errors = 'coerce')
doa['z'] = pd.to_numeric(doa['z'], errors = 'coerce')
print('First couple of clicks:')
print(doa.head()) # Display data

