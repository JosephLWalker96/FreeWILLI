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
colnames = ['t', 'az', 'el', 'x', 'y', 'z']
doa = pd.read_csv(path_doa, delim_whitespace = True, skiprows = 1, header = None, names = colnames)
doa['t_numeric'] = pd.to_numeric(doa['t'], errors = 'coerce')
doa['az'] = pd.to_numeric(doa['az'], errors = 'coerce')
doa['el'] = pd.to_numeric(doa['el'], errors = 'coerce')
doa['x'] = pd.to_numeric(doa['x'], errors = 'coerce')
doa['y'] = pd.to_numeric(doa['y'], errors = 'coerce')
doa['z'] = pd.to_numeric(doa['z'], errors = 'coerce')
print('First couple of clicks:')
print(doa.head()) # Display data

# Set up plot
fig = plt.figure(figsize=(12,8), facecolor='black')
ax = plt.axes([0.05, 0.05, 0.9, 0.9], projection = '3d')
ax.set_facecolor('black')
origin_scatter = ax.scatter(0,0,0,s=250,c='#777777') # Plot origin as single point at center

# Plot custom pitch and azimuth gridlines
degrees = np.linspace(0, 360, num=361, endpoint=True)
pitches = np.array([-80, -60, -40, -20, 0, 20, 40, 60, 80]) # Pitch
for i in pitches:
    guide_x = np.sin(np.deg2rad(degrees))*np.cos(np.deg2rad(i))
    guide_y = np.cos(np.deg2rad(degrees))*np.cos(np.deg2rad(i))
    guide_z = np.repeat(np.sin(np.deg2rad(i)), 361)
    ax.plot(guide_x, guide_y, guide_z, c='#777777')
    ax.text(1.1*np.cos(np.deg2rad(i)), 0, 1.1*np.sin(np.deg2rad(i)), i,
            horizontalalignment='center', verticalalignment='center', color='white') # Label degrees elevation)
azimuths = np.array([0, 30, 60, 90, 120, 150]) # Azimuth
for i in azimuths:
    guide_x = np.sin(np.deg2rad(degrees))*np.cos(np.deg2rad(i)) # These get confusing, but they have been sanity checked and they are correct :)
    guide_y = np.sin(np.deg2rad(degrees))*np.sin(np.deg2rad(i)) # ^
    guide_z = np.cos(np.deg2rad(degrees))
    ax.plot(guide_x, guide_y, guide_z, c='#777777')
    ax.text(1.1*np.cos(np.deg2rad(i)), 1.1*np.sin(np.deg2rad(i)), 0, i,
            horizontalalignment='center', verticalalignment='center', color='white') # Label degrees azimuth
    ax.text(-1.1*np.cos(np.deg2rad(i)), -1.1*np.sin(np.deg2rad(i)), 0, i-180,
            horizontalalignment='center', verticalalignment='center', color='white') # Label degrees azimuth (on other side)
# Finally, add a center axis to help keep track of where 0 degrees azimuth is
ax.plot([0, 1], [0, 0], [0, 0], c='#777777')


# Custom colormap
N = 256
vals = np.ones((N, 4))
vals[:, 0] = np.linspace(0, 1, N) # R
vals[:, 1] = np.linspace(1, 1, N) # G
vals[:, 2] = np.linspace(0, 1, N) # B
WhBG = ListedColormap(vals)

# Plot data
doa_scatter = ax.scatter(doa['x'], doa['y'], doa['z'], c=doa['t_numeric'], alpha=1, cmap=WhBG) # DOA
cbar = plt.colorbar(doa_scatter) # colorbar
cbar.ax.yaxis.set_tick_params(color='white')
cbar.ax.tick_params(axis='y', colors='white')
cbar.set_label('Time (s since Epoch)', color='white')

# Display
plt.axis('off') # Hide Cartesian axes
ax.set_xlim(-.75,.75)
ax.set_ylim(-.75,.75)
ax.set_zlim(-.75,.75)
ax.set_box_aspect([1,1,1])
ax.view_init(elev=6, azim=-173, roll=0)
ax.set_title('DOA')
plt.show() # display the plot
