#!/bin/bash

# Check if the script is being run as root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

# Execute commands to block wifi and bluetooth using rfkill
sudo rfkill unblock wifi
sudo rfkill unblock bluetooth
vcgencmd display_power 1  
echo "Wifi, Bluetooth, and HDMI are now unblocked."
