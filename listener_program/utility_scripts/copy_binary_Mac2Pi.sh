#!/bin/bash

# Variables
REMOTE_IP="192.168.100.235"  # Replace with the actual IP address of your Raspberry Pi if needed
REMOTE_USER="HARP"           # Remote username
REMOTE_DIR="~/Embedded_miniHarp/C/bin/"  # Target directory on Raspberry Pi
LOCAL_FILES="*X"             # Wildcard for all files ending with 'X'

# Check if the password is provided
if [ $# -ne 1 ]; then
  echo "Usage: $0 <password>"
  exit 1
fi

PASSWORD=$1

# Use sshpass with scp to copy all files ending in 'X' to the Raspberry Pi
sshpass -p "$PASSWORD" scp $LOCAL_FILES "$REMOTE_USER@$REMOTE_IP:$REMOTE_DIR"

# Confirm completion
if [ $? -eq 0 ]; then
  echo "Files copied to $REMOTE_USER@$REMOTE_IP:$REMOTE_DIR"
else
  echo "Error occurred during file copy"
fi
