# 🎥 Real Thermal Camera Integration Guide

## 🚀 **Live Camera Streaming Implementation**

### **✅ What's Been Added:**

1. **Real V4L2 Integration**: Direct capture from thermal cameras
2. **Multiple Format Support**: NV12, YUYV, GREY pixel formats
3. **Automatic Fallback**: Simulation if real camera not available
4. **Live Stream Detection**: Automatic camera detection and status
5. **Professional Error Handling**: Proper V4L2 error management

### **🔧 Real Camera Capture Flow:**

```cpp
// 1. Initialize V4L2 handle
irv4l2_handle_create(m_video_handle)

// 2. Open thermal camera device
irv4l2_open(m_v4l2_handle, "/dev/video0")

// 3. Set video format and resolution
irv4l2_set_format(m_v4l2_handle, &format)

// 4. Start live stream
irv4l2_start_stream(m_v4l2_handle)

// 5. Capture real frames
irv4l2_get_frame(m_v4l2_handle, &frame_data)
```

### **📹 Supported Thermal Camera Formats:**

#### **NV12 Format (Most Common):**
- Used by FLIR, Seek Thermal cameras
- YUV 4:2:0 format
- Automatic conversion to grayscale

#### **YUYV Format:**
- Standard thermal camera format
- YUV 4:2:2 format
- Direct thermal data capture

#### **GREY Format:**
- Direct grayscale thermal data
- No conversion needed
- Highest performance

### **🎯 Camera Detection & Setup:**

#### **1. Run Camera Setup Script:**
```bash
./setup_camera.sh
```

#### **2. Check Available Devices:**
```bash
ls -la /dev/video*
v4l2-ctl --list-devices
```

#### **3. Test Camera Access:**
```bash
v4l2-ctl --device=/dev/video0 --info
v4l2-ctl --device=/dev/video0 --list-formats-ext
```

### **⚙️ Configuration for Real Cameras:**

#### **Update jetson_thermal.conf:**
```json
{
    "camera": {
        "video_device": "/dev/video0",
        "width": 640,
        "height": 480,
        "fps": 30,
        "format": "nv12_and_temp"
    }
}
```

#### **Common Thermal Camera Settings:**

| Camera Type | Device | Resolution | Format | FPS |
|-------------|--------|-----------|--------|-----|
| FLIR Lepton | /dev/video0 | 160x120 | GREY | 9 |
| Seek Thermal | /dev/video0 | 320x240 | YUYV | 15 |
| Generic Thermal | /dev/video0 | 640x480 | NV12 | 30 |

### **🔍 Real Camera Detection:**

The application now automatically:
1. **Detects real thermal cameras** on startup
2. **Falls back to simulation** if no camera found
3. **Shows live status** in console output
4. **Handles multiple formats** automatically

### **📊 Live Streaming Features:**

#### **Real Camera Mode:**
- ✅ **Live thermal data** from actual camera
- ✅ **Real-time temperature** measurements
- ✅ **Professional thermal imaging** interface
- ✅ **Multiple colormap options**
- ✅ **Frame saving** capabilities

#### **Simulation Mode (Fallback):**
- ✅ **Dynamic thermal patterns** for testing
- ✅ **Moving heat sources** for demonstration
- ✅ **Realistic thermal noise** and gradients
- ✅ **Same interface** as real camera mode

### **🚀 Usage on Jetson:**

#### **1. Connect Thermal Camera:**
```bash
# Check if camera is detected
lsusb | grep -i thermal
ls -la /dev/video*
```

#### **2. Run Setup Script:**
```bash
./setup_camera.sh
```

#### **3. Update Configuration:**
Edit `config/jetson_thermal.conf` with your camera settings

#### **4. Build and Run:**
```bash
./build.sh
./run_thermal_camera.sh
```

### **🎮 Interactive Controls (Live Mode):**

- **`q` or ESC**: Quit application
- **`s`**: Save current thermal frame
- **`t`**: Toggle temperature range
- **`c`**: Cycle colormaps (JET, HOT, INFERNO, etc.)
- **`r`**: Reset camera view

### **📈 Performance Optimization:**

#### **For Real Cameras:**
- **Direct V4L2 capture** for maximum performance
- **Hardware-accelerated** format conversion
- **Memory-mapped** frame buffers
- **Thread-safe** frame processing

#### **For Jetson:**
- **ARM64 optimized** compilation
- **GPU acceleration** ready
- **Real-time processing** at 30 FPS
- **Low latency** thermal streaming

### **🔧 Troubleshooting:**

#### **Camera Not Detected:**
1. Check USB connection
2. Verify device permissions: `sudo chmod 666 /dev/video0`
3. Install thermal camera drivers
4. Check V4L2 compatibility

#### **Format Not Supported:**
1. List available formats: `v4l2-ctl --list-formats-ext`
2. Update configuration with correct format
3. Check camera documentation

#### **Performance Issues:**
1. Reduce resolution in config
2. Lower frame rate if needed
3. Check system resources
4. Use hardware acceleration

### **🎯 Expected Results:**

When you run with a real thermal camera:
- **Live thermal video** in Window 1
- **Color-mapped temperature** in Window 2
- **Real-time temperature data** from actual camera
- **Professional thermal imaging** interface
- **Interactive controls** for analysis

**Perfect for real thermal camera applications on Jetson!** 🚀

### **📝 Next Steps:**

1. **Connect your thermal camera** to Jetson
2. **Run setup script** to detect camera
3. **Update configuration** with correct settings
4. **Build and test** live streaming
5. **Enjoy real thermal imaging!** 🎥
