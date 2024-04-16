#!/bin/bash

# Check if the script is being run as root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

# Execute commands to block wifi and bluetooth using rfkill
sudo rfkill block wifi
sudo rfkill block bluetooth
vcgencmd display_power 0

echo "Wifi, Bluetooth, and HDMI are now blocked."
# check if wifi and bluetooth are blocked:
#
# iwconfig wlan0
# bluetoothctl show

