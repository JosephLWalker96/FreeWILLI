# Common libraries for data manipulation and file reading
import csv
from collections import namedtuple

# Libraries for plotting
import matplotlib.pyplot as plt
from matplotlib import colors as mcolors
from matplotlib.cm import ScalarMappable
from matplotlib.lines import Line2D
# NumPy for numerical operations
import numpy as np
import os
#from datetime import datetime
from datetime import datetime, timezone


def plot_log(path_doa, kalman_log, grouped_time_intervals):
    times = []
    filter_ids = []
    predicted_xs = []
    predicted_ys = []
    
    for entry in kalman_log:
        times.append(entry.time)
        filter_ids.append(entry.filter_id)
        predicted_xs.append(entry.updated_x)
        predicted_ys.append(entry.updated_y)
    
    # Normalize time to start at 0 and convert to minutes
    times = np.array(times)
    startTime = convertToStamp(path_doa)
    times = (times - startTime) /  (1000000 * 60) # Subtract first timestamp and convert to minutes
    
    filter_ids = np.array(filter_ids)
    unique_filter_ids = np.unique(filter_ids)
    #filter_ids = np.arange(0, len(unique_ids),1)
    unique_ids = np.arange(0, len(unique_filter_ids),1)
    print("unique tracks: ", unique_ids)
    
    num_unique = len(unique_ids)
    colormap = plt.get_cmap('tab10', num_unique)
    #color_mapping = {uid: colormap(i) for i, uid in enumerate(unique_ids)}
    color_mapping = {i: colormap(i) for i in np.arange(0,np.max(unique_filter_ids)+1,1)}

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(5, 4), sharex=True)
    
    # Plot predicted_ys
    #ax1.scatter(times, predicted_ys, c=[color_mapping[fid] for fid in filter_ids], alpha=0.1)
    ax1.scatter(times, predicted_ys,s=6, c=[color_mapping[i] for i in filter_ids], alpha=0.1)
    ax1.set_ylim(-180, 200)
    ax1.set_ylabel("Azimuth [degrees]")
    ax1.grid(True)
    ax1.set_xlim(-3, 64.0188)
    # Plot predicted_xs
    ax2.scatter(times, predicted_xs,s=6, c=[color_mapping[fid] for fid in filter_ids], alpha=0.1)
    ax2.set_ylim(30, 200)
    ax2.set_ylabel("Elevation [degrees]")
    ax2.set_xlabel("Time [minutes]")
    ax2.grid(True)
    ax2.set_xlim(-3, 64.0188)
    
    # Shade regions specified by grouped_time_intervals
    normalized_intervals = [(interval[0] - times[0]) / 60 for interval in grouped_time_intervals]
    for interval in normalized_intervals:
        ax1.axvspan(interval[0], interval[1], color='gray', alpha=0.1)
        ax2.axvspan(interval[0], interval[1], color='gray', alpha=0.1)
    
    # Create a simplified legend
    legend_elements = [Line2D([0], [0], marker='o', color='w', markerfacecolor=color_mapping[uid], markersize=8, label=f"{uid}") for uid in unique_ids]
    ax1.legend(handles=legend_elements, title="Track IDs", loc='upper right', bbox_to_anchor=(1.35, 1.1))
    
    plt.tight_layout()

    # Extract the base path from the file path and create the output file path
    base_path = os.path.splitext(path_doa.split('/')[-1])[0]  # Remove file extension
    output_path = base_path + ".png"  # Append desired file name extension

    # Save the figure to the output path
    plt.savefig(output_path, dpi=300)
    plt.show()


def read_log_csv(filename):
    """
    Reads the kalman_log CSV file and returns a list of LogEntry namedtuples.
    """
    LogEntry = namedtuple('LogEntry', ['time', 'filter_id', 'updated_x', 'updated_y'])
    kalman_log = []
    with open(filename, 'r') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            entry = LogEntry(
                time=float(row['time']),
                filter_id=int(row['filter_id']),
                updated_x=float(row['updated_x']),
                updated_y=float(row['updated_y'])
            )
            kalman_log.append(entry)
    return kalman_log

def convertToStamp(path_doa):
    # Extract timestamp from the file path
    timestamp = path_doa.split('/')[-1]
    year = int(timestamp[:2]) + 2000  # Assume 21st century
    month = int(timestamp[2:4])
    day = int(timestamp[4:6])
    hour = int(timestamp[7:9])
    minute = int(timestamp[9:11])
    second = int(timestamp[11:13])
    microsecond = int(timestamp[14:14+6])

    # Create a datetime object
    dt = datetime(year, month, day, hour, minute, second, microsecond, tzinfo=timezone.utc)
    print("Datetime object: ", dt)

    # Convert to Unix timestamp in seconds
    unix_time_seconds = dt.timestamp()

    # Convert to microseconds
    unix_time_microseconds = int(unix_time_seconds * 1000000)
    print("Unix time in microseconds: ", unix_time_microseconds)

    return unix_time_microseconds