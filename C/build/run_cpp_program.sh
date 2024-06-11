#!/bin/bash

# Example to run program ./run_cpp_program.sh 2 1 self 1045 

if [ "$#" -ne 5 ]; then
  echo "Usage: $0 <run_time_in_minutes> <sleep_time_in_minutes> <UDP_IP> <UDP_PORT> <DETECTION_THRESHOLD>"
  exit 1
fi

RUN_TIME=$1
SLEEP_TIME=$2
UDP_IP=$3
UDP_PORT=$4
DETECTION_THRESHOLD=$5
FIRMWARE_VERSION=1240

# Convert minutes to seconds
RUN_TIME_SEC=$((RUN_TIME * 60))
SLEEP_TIME_SEC=$((SLEEP_TIME * 60))

# Program executable
PROGRAM="./listen_RM101"

# Define the error log file as a variable
ERROR_LOG="../deployment_files/error_log.txt"

# Clear the error_log.txt file at the start
> "$ERROR_LOG"

# Function to handle cleanup upon receiving SIGINT
cleanup() {
  echo "Caught SIGINT signal! Terminating..."
  if [ -n "$PID" ] && kill -0 $PID 2>/dev/null; then
    kill $PID 2>/dev/null
  fi
  exit 0
}

# Trap SIGINT signal (Ctrl-C)
trap cleanup SIGINT

while true; do
  # Run the compiled C++ program in the background
  #"$PROGRAM" "$UDP_IP" $UDP_PORT $FIRMWARE_VERSION $DETECTION_THRESHOLD &
  "$PROGRAM" "$UDP_IP" $UDP_PORT $FIRMWARE_VERSION $DETECTION_THRESHOLD > /dev/null 2>> "$ERROR_LOG" &
  
  # Get the PID of the last background command
  PID=$!

  # Allow the program to run for the specified time in seconds
  sleep $RUN_TIME_SEC
  
  # Check if the program is still running and kill it if necessary
  if kill -0 $PID 2>/dev/null; then
    kill $PID
    echo "Restarting program."
  else
    echo "Program crashed or terminated unexpectedly."
  fi

  # Wait for the specified sleep time in seconds before restarting
  #sleep $SLEEP_TIME_SEC
done
