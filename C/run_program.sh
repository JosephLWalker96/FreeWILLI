#!/bin/bash

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <path/to/config_file.txt> <runtime>"
  echo "Example: ./run_cpp_program.sh config_files/config.json 500"
  exit 1
fi

CONFIG_FILE=$1
RUNTIME_SEC=$2
CHECK_TIME=$((RUNTIME_SEC + 2))

# Program executable
PROGRAM="./bin/HarpListen"

DIR="deployment_files"

# Check if the directory exists
if [ -d "$DIR" ]; then
  # If it exists, clear its contents
  rm -rf "${DIR:?}"/*
else
  # If it does not exist, create the directory
  mkdir "$DIR"
fi

# Define the error log file as a variable
ERROR_LOG="$DIR/error_log.txt"

# Function to handle cleanup upon receiving SIGINT
cleanup() {
  echo "Caught SIGINT signal! Terminating..."
  if [ -n "$PID" ] && kill -0 $PID ; then
    kill $PID
  fi
  exit 0
}

# Trap SIGINT signal (Ctrl-C)
trap cleanup SIGINT

while true; do
  # Run the compiled C++ program in the background
  #"$PROGRAM" "$CONFIG_FILE" $RUNTIME_SEC > /dev/null 2>> "$ERROR_LOG" &
  "$PROGRAM" "$CONFIG_FILE" $RUNTIME_SEC > /dev/null 2>> "$ERROR_LOG" &
  
  # Get the PID of the last background command
  PID=$!

  sleep 1 # allow program to initialize variables and begin execution before checking it 
  
  if kill -0 $PID ; then
    echo "Program started..."
  else
    echo "Program failed to start..."
  fi

  # Allow the program to run for the specified time in seconds
  sleep $CHECK_TIME
  
  # Check if the program is still running and kill it if necessary
  if kill -0 $PID 2>/dev/null; then
    echo "Program did not exit within the specified time, forcefully terminating..."
    kill -SIGKILL $PID
  fi

  # Wait for the program to terminate and get its exit status
  wait $PID
  EXIT_STATUS=$?

  if [ $EXIT_STATUS -eq 0 ]; then
    echo "Program exited successfully with status 0."
  else
    echo "Program exited with status $EXIT_STATUS."
  fi  

done
