#!/bin/bash

# Build Diagnostic Script
# This script checks the build status and helps debug issues

echo "=== Jetson Thermal Camera Build Diagnostic ==="
echo ""

# Check if we're in the right directory
if [ ! -f "build.sh" ]; then
    echo "Error: build.sh not found. Please run this script from the jetson_thermal_sample directory"
    exit 1
fi

echo "1. Checking project structure..."
if [ -d "src" ]; then
    echo "✅ src/ directory found"
else
    echo "❌ src/ directory not found"
fi

if [ -d "config" ]; then
    echo "✅ config/ directory found"
else
    echo "❌ config/ directory not found"
fi

if [ -f "config/jetson_thermal.conf" ]; then
    echo "✅ config/jetson_thermal.conf found"
else
    echo "❌ config/jetson_thermal.conf not found"
fi

echo ""
echo "2. Checking build directory..."
if [ -d "build" ]; then
    echo "✅ build/ directory found"
    echo "Build directory contents:"
    ls -la build/
    echo ""
    
    echo "3. Checking for executables in build directory..."
    if [ -f "build/jetson_thermal_sample" ]; then
        echo "✅ jetson_thermal_sample found in build/"
        ls -la build/jetson_thermal_sample
    else
        echo "❌ jetson_thermal_sample not found in build/"
    fi
    
    if [ -f "build/jetson_thermal_sample_simple" ]; then
        echo "✅ jetson_thermal_sample_simple found in build/"
        ls -la build/jetson_thermal_sample_simple
    else
        echo "❌ jetson_thermal_sample_simple not found in build/"
    fi
    
    echo ""
    echo "4. Checking build logs..."
    if [ -f "build/build.log" ]; then
        echo "✅ build.log found"
        echo "Last 20 lines of build log:"
        tail -20 build/build.log
    else
        echo "❌ build.log not found"
    fi
    
else
    echo "❌ build/ directory not found"
    echo "Build has not been run yet. Please run ./build.sh first"
fi

echo ""
echo "5. Checking for executables in current directory..."
if [ -f "jetson_thermal_sample" ]; then
    echo "✅ jetson_thermal_sample found in current directory"
    ls -la jetson_thermal_sample
else
    echo "❌ jetson_thermal_sample not found in current directory"
fi

if [ -f "jetson_thermal_sample_simple" ]; then
    echo "✅ jetson_thermal_sample_simple found in current directory"
    ls -la jetson_thermal_sample_simple
else
    echo "❌ jetson_thermal_sample_simple not found in current directory"
fi

echo ""
echo "6. Checking SDK availability..."
if [ -d "../libir_SDK_release" ]; then
    echo "✅ libir_SDK_release directory found"
    if [ -d "../libir_SDK_release/linux/aarch64-linux-gnu" ]; then
        echo "✅ ARM64 libraries found"
        ls -la ../libir_SDK_release/linux/aarch64-linux-gnu/ | head -5
    else
        echo "❌ ARM64 libraries not found"
    fi
else
    echo "❌ libir_SDK_release directory not found"
fi

echo ""
echo "=== Diagnostic Complete ==="
echo ""
echo "If no executables were found, try running:"
echo "  ./build.sh"
echo ""
echo "If build failed, check the build logs for errors."
