#!/bin/bash

echo "=== Thermal Camera Setup for Jetson ==="
echo "This script helps detect and configure thermal cameras"
echo ""

# Check for V4L2 devices
echo "🔍 Scanning for video devices..."
echo "Available video devices:"
ls -la /dev/video* 2>/dev/null || echo "No video devices found"

echo ""
echo "📹 V4L2 device information:"
for device in /dev/video*; do
    if [ -e "$device" ]; then
        echo "--- $device ---"
        v4l2-ctl --device=$device --info 2>/dev/null || echo "Cannot get info for $device"
        echo ""
    fi
done

# Check for thermal camera specific devices
echo "🌡️ Checking for thermal camera devices..."
echo "USB devices (thermal cameras often appear as USB):"
lsusb | grep -i "thermal\|ir\|flir\|seek\|lepton" || echo "No thermal camera USB devices detected"

echo ""
echo "📋 Supported thermal camera formats:"
echo "- NV12: Common thermal camera format"
echo "- YUYV: Standard thermal format" 
echo "- GREY: Grayscale thermal data"
echo ""

# Test camera access
echo "🧪 Testing camera access..."
if [ -e "/dev/video0" ]; then
    echo "Testing /dev/video0..."
    timeout 5s v4l2-ctl --device=/dev/video0 --stream-mmap --stream-count=1 2>/dev/null && echo "✓ /dev/video0 is accessible" || echo "✗ /dev/video0 access failed"
fi

if [ -e "/dev/video1" ]; then
    echo "Testing /dev/video1..."
    timeout 5s v4l2-ctl --device=/dev/video1 --stream-mmap --stream-count=1 2>/dev/null && echo "✓ /dev/video1 is accessible" || echo "✗ /dev/video1 access failed"
fi

echo ""
echo "🔧 Configuration recommendations:"
echo "1. If thermal camera is detected, update jetson_thermal.conf:"
echo "   - Set correct video_device path (e.g., /dev/video0)"
echo "   - Set correct resolution and format"
echo "   - Set appropriate frame rate"
echo ""
echo "2. For FLIR/Seek thermal cameras:"
echo "   - Usually appear as /dev/video0 or /dev/video1"
echo "   - Common formats: NV12, YUYV"
echo "   - Typical resolution: 160x120, 320x240, 640x480"
echo ""
echo "3. For Lepton thermal cameras:"
echo "   - Often use SPI interface"
echo "   - May require special drivers"
echo "   - Resolution: 80x60, 160x120"
echo ""

# Check OpenCV installation
echo "📦 Checking OpenCV installation..."
python3 -c "import cv2; print('OpenCV version:', cv2.__version__)" 2>/dev/null || echo "OpenCV not found in Python"

# Check for thermal camera libraries
echo ""
echo "📚 Checking for thermal camera libraries..."
ldconfig -p | grep -i "thermal\|ir\|flir" || echo "No thermal camera libraries found"

echo ""
echo "✅ Setup complete!"
echo "Next steps:"
echo "1. Connect your thermal camera"
echo "2. Run this script again to verify detection"
echo "3. Update jetson_thermal.conf with correct device settings"
echo "4. Build and run the thermal camera application"
