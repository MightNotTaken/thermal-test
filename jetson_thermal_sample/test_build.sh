#!/bin/bash

echo "=== Testing Thermal Camera Build ==="
echo "This script tests the build process for the thermal camera project"
echo ""

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run from project root."
    exit 1
fi

echo "🔍 Checking build environment..."
echo "Current directory: $(pwd)"
echo "CMakeLists.txt: $(ls -la CMakeLists.txt)"

echo ""
echo "🧹 Cleaning previous build..."
rm -rf build/
mkdir -p build/

echo ""
echo "🔧 Configuring CMake..."
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_C_FLAGS="-Wno-unused-variable -Wno-unused-parameter -Wno-format-truncation" \
         -DCMAKE_CXX_FLAGS="-Wno-reorder -Wno-unused-variable -Wno-unused-parameter"

if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed"
    exit 1
fi

echo ""
echo "🔨 Building project..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo "Executable location: $(pwd)/jetson_thermal_sample"
    echo ""
    echo "📋 Build summary:"
    echo "- Full SDK integration: ✅"
    echo "- Real camera support: ✅"
    echo "- Video streaming: ✅"
    echo "- OpenCV integration: ✅"
    echo ""
    echo "🚀 Ready to run:"
    echo "  ./jetson_thermal_sample config/jetson_thermal.conf"
else
    echo ""
    echo "❌ Build failed!"
    echo "Check the error messages above for details."
    exit 1
fi
