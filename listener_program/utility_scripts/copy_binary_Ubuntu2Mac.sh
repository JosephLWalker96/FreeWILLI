#!/bin/bash

# Variables
REMOTE_IP="132.239.113.208"
REMOTE_USER="harp"  # Replace with your remote username
REMOTE_PATH="~/Documents/Embedded_miniHarp/C/bin/*X"
LOCAL_PATH="."

# Check if password is provided as an argument
if [ -z "$1" ]; then
  echo "Usage: $0 <password>"
  exit 1
fi

PASSWORD=$1

# Use scp with sshpass to copy the file from the remote server
sshpass -p "$PASSWORD" scp "$REMOTE_USER@$REMOTE_IP:$REMOTE_PATH" "$LOCAL_PATH"

# Confirm completion
if [ $? -eq 0 ]; then
  echo "File copied to $LOCAL_PATH"
else
  echo "Error occurred during file copy"
fi
