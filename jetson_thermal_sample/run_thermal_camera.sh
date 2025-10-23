#!/bin/bash

# Jetson Thermal Camera Launcher Script
# This script launches the thermal camera application with proper environment setup

set -e  # Exit on any error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running on Jetson
check_jetson() {
    if [ -f /etc/nv_tegra_release ]; then
        print_success "NVIDIA Jetson detected"
    else
        print_warning "Not running on Jetson - some features may not work"
    fi
}

# Set up environment
setup_environment() {
    print_status "Setting up environment..."
    
    # Set library path
    export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
    
    # Set CUDA path if available
    if [ -d "/usr/local/cuda" ]; then
        export CUDA_ROOT=/usr/local/cuda
        export PATH=$CUDA_ROOT/bin:$PATH
        export LD_LIBRARY_PATH=$CUDA_ROOT/lib64:$LD_LIBRARY_PATH
    fi
    
    # Set OpenCV path
    export OPENCV_ROOT=/usr
    
    print_success "Environment configured"
}

# Check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    # Check for executables in multiple locations
    EXECUTABLE_FOUND=false
    EXECUTABLE_PATH=""
    
    # Try full version first
    if [ -f "./jetson_thermal_sample" ]; then
        EXECUTABLE_PATH="./jetson_thermal_sample"
        EXECUTABLE_FOUND=true
        print_success "Found full SDK version: $EXECUTABLE_PATH"
    elif [ -f "./build/jetson_thermal_sample" ]; then
        EXECUTABLE_PATH="./build/jetson_thermal_sample"
        EXECUTABLE_FOUND=true
        print_success "Found full SDK version in build directory: $EXECUTABLE_PATH"
    # Try simple version
    elif [ -f "./jetson_thermal_sample_simple" ]; then
        EXECUTABLE_PATH="./jetson_thermal_sample_simple"
        EXECUTABLE_FOUND=true
        print_success "Found simple version: $EXECUTABLE_PATH"
    elif [ -f "./build/jetson_thermal_sample_simple" ]; then
        EXECUTABLE_PATH="./build/jetson_thermal_sample_simple"
        EXECUTABLE_FOUND=true
        print_success "Found simple version in build directory: $EXECUTABLE_PATH"
    fi
    
    if [ "$EXECUTABLE_FOUND" = false ]; then
        print_error "No executable found"
        print_status "Available files:"
        ls -la jetson_thermal_sample* 2>/dev/null || echo "No jetson_thermal_sample* files found"
        ls -la build/jetson_thermal_sample* 2>/dev/null || echo "No build/jetson_thermal_sample* files found"
        echo ""
        print_error "Please run ./build.sh first to build the project"
        exit 1
    fi
    
    # Check if config file exists
    if [ ! -f "config/jetson_thermal.conf" ]; then
        print_error "Configuration file not found: config/jetson_thermal.conf"
        exit 1
    fi
    
    print_success "Dependencies checked"
}

# Check hardware
check_hardware() {
    print_status "Checking hardware..."
    
    # Check for video devices
    if ls /dev/video* >/dev/null 2>&1; then
        print_success "Video devices found:"
        ls /dev/video*
    else
        print_warning "No video devices found"
    fi
    
    # Check for serial devices
    if ls /dev/ttyUSB* >/dev/null 2>&1; then
        print_success "USB serial devices found:"
        ls /dev/ttyUSB*
    else
        print_warning "No USB serial devices found"
    fi
    
    # Check for I2C devices
    if ls /dev/i2c-* >/dev/null 2>&1; then
        print_success "I2C devices found:"
        ls /dev/i2c-*
    else
        print_warning "No I2C devices found"
    fi
}

# Main function
main() {
    echo "=== Jetson Thermal Camera Launcher ==="
    echo ""
    
    check_jetson
    setup_environment
    check_dependencies
    check_hardware
    
    echo ""
    print_status "Starting Jetson Thermal Camera Sample..."
    print_status "Configuration: config/jetson_thermal.conf"
    print_status "Press Ctrl+C to stop"
    echo ""
    
    # Run the application
    exec $EXECUTABLE_PATH config/jetson_thermal.conf
}

# Handle signals
trap 'echo ""; print_status "Shutting down..."; exit 0' INT TERM

# Run main function
main "$@"
