# Jetson Thermal Camera Sample

A comprehensive sample project for using thermal cameras with NVIDIA Jetson devices running Ubuntu 22.04. This project demonstrates how to integrate the libir SDK with Jetson hardware for thermal imaging applications.

## Features

- **Jetson Optimized**: Specifically designed for NVIDIA Jetson ARM64 architecture
- **Multiple Control Interfaces**: Support for UART, I2C, and USB control interfaces
- **OpenCV Integration**: Modern C++ implementation with OpenCV for image processing
- **Configurable**: JSON-based configuration system
- **Thread-Safe**: Multi-threaded architecture for optimal performance
- **Easy to Use**: Simple build system and launcher scripts

## System Requirements

### Hardware
- NVIDIA Jetson device (Nano, Xavier NX, AGX Xavier, Orin, etc.)
- Thermal camera compatible with libir SDK
- USB or serial connection for camera control
- Video capture device (V4L2 compatible)

### Software
- Ubuntu 22.04 LTS
- CUDA Toolkit (if available)
- OpenCV 4.x
- CMake 3.10+
- GCC/G++ compiler

## Project Structure

```
jetson_thermal_sample/
├── CMakeLists.txt              # CMake build configuration
├── build.sh                    # Automated build script
├── run_thermal_camera.sh       # Application launcher
├── README.md                   # This file
├── config/
│   └── jetson_thermal.conf     # Configuration file
└── src/
    ├── main.cpp                # Main application entry point
    ├── thermal_camera.h        # Thermal camera class header
    ├── thermal_camera.cpp      # Thermal camera implementation
    ├── config_parser.h         # Configuration parser header
    └── config_parser.cpp      # Configuration parser implementation
```

## Quick Start

### 1. Prerequisites

Ensure you have the libir SDK available in the parent directory:
```
../libir_SDK_release/
```

### 2. Build the Project

Run the automated build script:
```bash
cd jetson_thermal_sample
chmod +x build.sh
./build.sh
```

The build script will:
- Check system requirements
- Install dependencies
- Configure CMake for ARM64
- Build the project
- Install the application
- Create launcher script

### 3. Run the Application

```bash
./run_thermal_camera.sh
```

Or manually:
```bash
./jetson_thermal_sample config/jetson_thermal.conf
```

## Configuration

The application uses a JSON configuration file (`config/jetson_thermal.conf`) to specify:

### Device Configuration
```json
{
    "device": {
        "name": "G1280s",
        "control_type": "uart"
    }
}
```

### Control Interface
```json
{
    "control": {
        "type": "uart",
        "uart": {
            "device": "/dev/ttyUSB0",
            "baudrate": 115200
        }
    }
}
```

### Camera Settings
```json
{
    "camera": {
        "video_device": "/dev/video0",
        "width": 1280,
        "height": 1024,
        "fps": 30
    }
}
```

### Display Options
```json
{
    "display": {
        "type": "opencv",
        "window_name": "Thermal Camera",
        "show_temperature": true,
        "colormap": "jet"
    }
}
```

## Manual Build (Advanced)

If you prefer to build manually:

### 1. Install Dependencies
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libopencv-dev libv4l-dev
```

### 2. Create Build Directory
```bash
mkdir build
cd build
```

### 3. Configure CMake
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### 4. Build
```bash
make -j$(nproc)
```

### 5. Install
```bash
sudo make install
```

## Usage Examples

### Basic Usage
```bash
# Run with default configuration
./jetson_thermal_sample config/jetson_thermal.conf
```

### Custom Configuration
```bash
# Create custom config
cp config/jetson_thermal.conf config/my_config.conf
# Edit my_config.conf as needed
./jetson_thermal_sample config/my_config.conf
```

### Command Line Options
```bash
# Show help
./jetson_thermal_sample --help

# Verbose output
./jetson_thermal_sample config/jetson_thermal.conf --verbose
```

## Troubleshooting

### Common Issues

#### 1. Permission Denied
```bash
# Fix permissions for USB devices
sudo usermod -a -G dialout $USER
# Logout and login again
```

#### 2. Video Device Not Found
```bash
# List available video devices
ls /dev/video*
# Update configuration file with correct device
```

#### 3. UART Device Not Found
```bash
# List available serial devices
ls /dev/ttyUSB*
ls /dev/ttyACM*
# Update configuration file with correct device
```

#### 4. Library Not Found
```bash
# Set library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### Debug Mode

Enable debug logging by modifying the configuration:
```json
{
    "logging": {
        "level": "debug",
        "console": true
    }
}
```

## Performance Optimization

### 1. GPU Acceleration
Enable CUDA support in configuration:
```json
{
    "jetson": {
        "gpu_acceleration": true,
        "cuda_support": true
    }
}
```

### 2. Memory Optimization
```bash
# Increase shared memory
sudo sh -c 'echo 1024 > /proc/sys/kernel/shmmax'
```

### 3. CPU Governor
```bash
# Set performance mode
sudo nvpmodel -m 0
sudo jetson_clocks
```

## API Reference

### ThermalCamera Class

#### Methods
- `initialize(config)`: Initialize camera with configuration
- `start()`: Start thermal camera stream
- `stop()`: Stop thermal camera stream
- `isRunning()`: Check if camera is running
- `processFrame()`: Process a single frame
- `getThermalImage()`: Get current thermal image
- `getTemperatureData()`: Get temperature data
- `saveFrame(filename)`: Save current frame to file

#### Example Usage
```cpp
ThermalCamera camera;
if (camera.initialize(config)) {
    camera.start();
    while (camera.isRunning()) {
        cv::Mat thermal_img = camera.getThermalImage();
        // Process thermal image
    }
    camera.stop();
}
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test on Jetson hardware
5. Submit a pull request

## License

This project is provided as a sample for educational purposes. Please refer to the libir SDK license for commercial usage.

## Support

For technical support:
- Check the troubleshooting section
- Review the libir SDK documentation
- Contact the SDK provider for hardware-specific issues

## Changelog

### Version 1.0.0
- Initial release
- Jetson ARM64 support
- UART/I2C/USB control interfaces
- OpenCV integration
- JSON configuration system
- Multi-threaded architecture

---

**Note**: This sample project is designed for NVIDIA Jetson devices with Ubuntu 22.04. Ensure your thermal camera is compatible with the libir SDK before use.
