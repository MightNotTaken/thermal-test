#!/bin/bash

# Jetson Thermal Camera Sample Build Script
# For NVIDIA Jetson with Ubuntu 22.04
# Fixed version with all compilation errors resolved

set -e  # Exit on any error

echo "=== Jetson Thermal Camera Sample Build Script ==="
echo "Target: NVIDIA Jetson with Ubuntu 22.04"
echo "Architecture: ARM64 (aarch64-linux-gnu)"
echo "Fixed version with all compilation errors resolved"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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
    print_status "Checking if running on NVIDIA Jetson..."
    
    if [ -f /etc/nv_tegra_release ]; then
        print_success "NVIDIA Jetson detected"
        cat /etc/nv_tegra_release
    else
        print_warning "Not running on Jetson - build may not work correctly"
    fi
    echo ""
}

# Check system requirements
check_requirements() {
    print_status "Checking system requirements..."
    
    # Check Ubuntu version
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        if [[ "$VERSION_ID" == "22.04" ]]; then
            print_success "Ubuntu 22.04 detected"
        else
            print_warning "Ubuntu version: $VERSION_ID (expected 22.04)"
        fi
    fi
    
    # Check architecture
    ARCH=$(uname -m)
    if [[ "$ARCH" == "aarch64" ]]; then
        print_success "ARM64 architecture detected"
    else
        print_error "Unsupported architecture: $ARCH (expected aarch64)"
        exit 1
    fi
    
    # Check available memory
    MEMORY=$(free -m | awk 'NR==2{printf "%.0f", $2}')
    if [ "$MEMORY" -lt 4000 ]; then
        print_warning "Low memory detected: ${MEMORY}MB (recommended: 4GB+)"
    else
        print_success "Memory: ${MEMORY}MB"
    fi
    
    echo ""
}

# Install dependencies
install_dependencies() {
    print_status "Installing dependencies..."
    
    # Update package list
    sudo apt-get update
    
    # Install build tools
    sudo apt-get install -y \
        build-essential \
        cmake \
        git \
        pkg-config \
        libopencv-dev \
        libv4l-dev \
        libudev-dev \
        libusb-1.0-0-dev \
        libi2c-dev \
        libserial-dev \
        libjpeg-dev \
        libpng-dev \
        libtiff-dev \
        libavcodec-dev \
        libavformat-dev \
        libswscale-dev \
        libgtk-3-dev \
        libcanberra-gtk-module \
        libcanberra-gtk3-module
    
    print_success "Dependencies installed"
    echo ""
}

# Check SDK availability
check_sdk() {
    print_status "Checking SDK availability..."
    
    SDK_PATH="../libir_SDK_release"
    if [ ! -d "$SDK_PATH" ]; then
        print_error "SDK not found at: $SDK_PATH"
        print_error "Please ensure the libir_SDK_release directory is available"
        exit 1
    fi
    
    # Check for ARM64 libraries
    ARM64_LIB_PATH="$SDK_PATH/linux/aarch64-linux-gnu"
    if [ ! -d "$ARM64_LIB_PATH" ]; then
        print_error "ARM64 libraries not found at: $ARM64_LIB_PATH"
        print_error "Please ensure the SDK includes ARM64 libraries"
        exit 1
    fi
    
    print_success "SDK found and ARM64 libraries available"
    echo ""
}

# Create build directory
create_build_dir() {
    print_status "Creating build directory..."
    
    if [ -d "build" ]; then
        print_warning "Build directory exists - cleaning..."
        # Force remove with sudo if needed
        if ! rm -rf build 2>/dev/null; then
            print_warning "Standard removal failed, trying with sudo..."
            sudo rm -rf build
        fi
    fi
    
    mkdir -p build
    cd build
    
    print_success "Build directory created"
    echo ""
}

# Configure CMake
configure_cmake() {
    print_status "Configuring CMake..."
    
    # Set environment variables for Jetson
    export CUDA_ROOT=/usr/local/cuda
    export OPENCV_ROOT=/usr
    
    # Configure with CMake
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=Linux \
        -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
        -DCMAKE_C_COMPILER=gcc \
        -DCMAKE_CXX_COMPILER=g++ \
        -DCMAKE_C_FLAGS="-O3 -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-format-truncation" \
        -DCMAKE_CXX_FLAGS="-O3 -Wall -Wextra -Wno-unused-variable -Wno-reorder -Wno-unused-parameter" \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DOPENCV_DIR=/usr/lib/aarch64-linux-gnu/cmake/opencv4
    
    print_success "CMake configuration completed"
    echo ""
}

# Build the project
build_project() {
    print_status "Building project..."
    
    # Get number of CPU cores for parallel build
    CORES=$(nproc)
    print_status "Using $CORES cores for parallel build"
    
    # Try to build the full SDK version first
    print_status "Building full SDK version (with libir SDK dependencies)..."
    if make jetson_thermal_sample -j$CORES 2>&1 | tee build.log; then
        print_success "Full SDK version built successfully"
    else
        print_error "Full SDK version build failed"
        print_status "Trying simple fallback version..."
        if make jetson_thermal_sample_simple -j$CORES; then
            print_success "Simple fallback version built successfully"
            print_warning "Using simple version - full SDK features not available"
        else
            print_error "Both versions failed to build"
            return 1
        fi
    fi
    
    print_success "Build completed"
    echo ""
}

# Install the project
install_project() {
    print_status "Installing project..."
    
    sudo make install
    
    print_success "Installation completed"
    echo ""
}

# Create launcher script
create_launcher() {
    print_status "Creating launcher script..."
    
    cat > ../run_thermal_camera.sh << 'EOF'
#!/bin/bash

# Jetson Thermal Camera Launcher Script

# Set environment variables
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
export CUDA_ROOT=/usr/local/cuda

# Check if config file exists
CONFIG_FILE="config/jetson_thermal.conf"
if [ ! -f "$CONFIG_FILE" ]; then
    echo "Error: Configuration file not found: $CONFIG_FILE"
    exit 1
fi

# Run the thermal camera application
echo "Starting Jetson Thermal Camera Sample..."
echo "Configuration: $CONFIG_FILE"
echo "Press Ctrl+C to stop"
echo ""

# Try full version first, fallback to simple
if [ -f "./jetson_thermal_sample" ]; then
    ./jetson_thermal_sample "$CONFIG_FILE"
elif [ -f "./jetson_thermal_sample_simple" ]; then
    ./jetson_thermal_sample_simple "$CONFIG_FILE"
else
    echo "Error: No executable found"
    exit 1
fi
EOF
    
    chmod +x ../run_thermal_camera.sh
    
    print_success "Launcher script created: run_thermal_camera.sh"
    echo ""
}

# Main build process
main() {
    echo "Starting build process for Jetson Thermal Camera Sample..."
    echo ""
    
    check_jetson
    check_requirements
    install_dependencies
    check_sdk
    create_build_dir
    configure_cmake
    build_project
    install_project
    create_launcher
    
    print_success "=== Build Process Completed Successfully ==="
    echo ""
    echo "To run the thermal camera sample:"
    echo "  cd .."
    echo "  ./run_thermal_camera.sh"
    echo ""
    echo "Or manually:"
    echo "  ./jetson_thermal_sample config/jetson_thermal.conf"
    echo "  # or"
    echo "  ./jetson_thermal_sample_simple config/jetson_thermal.conf"
    echo ""
}

# Run main function
main "$@"
