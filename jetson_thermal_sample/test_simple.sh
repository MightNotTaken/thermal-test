#!/bin/bash

# Simple test script for the Jetson Thermal Camera Sample
# This script tests the simple version without SDK dependencies

set -e

echo "=== Testing Simple Thermal Camera Sample ==="
echo ""

# Check if executable exists
if [ ! -f "./jetson_thermal_sample" ]; then
    echo "Error: jetson_thermal_sample executable not found"
    echo "Please run ./build.sh first"
    exit 1
fi

# Check if config file exists
if [ ! -f "config/jetson_thermal.conf" ]; then
    echo "Error: Configuration file not found"
    exit 1
fi

echo "Running simple thermal camera test..."
echo "This will run for 10 seconds and then stop"
echo ""

# Run the application with timeout
timeout 10s ./jetson_thermal_sample config/jetson_thermal.conf || true

echo ""
echo "=== Test Completed ==="
echo "Check /tmp/ for saved thermal images"
echo ""

# List any saved images
if ls /tmp/thermal_frame_*.png >/dev/null 2>&1; then
    echo "Saved thermal images:"
    ls -la /tmp/thermal_frame_*.png
else
    echo "No thermal images were saved"
fi
