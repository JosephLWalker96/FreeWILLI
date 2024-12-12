#!/bin/bash

# Check if the script is being run as root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

# Execute commands to block wifi and bluetooth using rfkill
sudo rfkill block wifi
sudo rfkill block bluetooth

sleep 15

sudo vcgencmd display_power 0
echo "Wifi, Bluetooth, and HDMI are now blocked."

sleep 5

python Python/multi_datalogger_reader.py --fw 1240
#sync
# check if wifi and bluetooth are blocked:
#
# iwconfig wlan0
# bluetoothctl show


