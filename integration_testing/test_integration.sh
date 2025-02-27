#!/bin/bash

set -e  # Exit on error

# Build listener program before running the test
echo "Building listener program..."
cd listener_program/
rm -rf out/* && mkdir -p out && cd out
cmake ..
make -j$(nproc)
cd ../../

# Paths
LISTENER_CMD="./listener_program/bin/Listener listener_program/config_files/integration_test.json 5000"
SIMULATOR_CMD="python3 simulator_program/datalogger_simulator.py --ip self --fw 1240 --port 1045 --data_dir simulator_program/simulator_data/integration_test/ --tdoa_sim 20"
ARTIFACT_DIR="integration_testing/deployment_files/"
GROUND_TRUTH="integration_testing/integrationTest.txt"

echo "Starting listener program..."
$LISTENER_CMD &
LISTENER_PID=$!

echo "Listener started with PID: $LISTENER_PID"

echo "Running simulator program..."
$SIMULATOR_CMD

echo "Simulator finished. Stopping listener..."
kill $LISTENER_PID
wait $LISTENER_PID || echo "Listener process exited."

# Find the most recent artifact file (assuming filename format is timestamped)
OUTPUT_FILE=$(ls -t "$ARTIFACT_DIR" | head -n 1)
OUTPUT_PATH="$ARTIFACT_DIR/$OUTPUT_FILE"

echo "Checking output file: $OUTPUT_PATH"

# Ensure the output file exists
if [[ ! -f "$OUTPUT_PATH" ]]; then
    echo "Error: Expected output file not found!"
    exit 1
fi

# Compare output file to ground truth
if diff -q "$OUTPUT_PATH" "$GROUND_TRUTH" > /dev/null; then
    echo "Test Passed: Output matches ground truth."
    TEST_RESULT=0
else
    echo "Test Failed: Output does not match ground truth."
    TEST_RESULT=1
fi

# Clear the contents of ARTIFACT_DIR
echo "Clearing artifact directory: $ARTIFACT_DIR"
rm -rf "$ARTIFACT_DIR"/*

exit $TEST_RESULT

