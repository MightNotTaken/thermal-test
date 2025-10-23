#include "thermal_camera.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

// Include SDK headers
extern "C" {
    #include "libircam.h"
    #include "libircmd.h"
    #include "libiruart.h"
    #include "libirv4l2.h"
    #include "libiri2c.h"
    #include "libirspi.h"
    #include "libirparse.h"
}

ThermalCamera::ThermalCamera() 
    : m_stream_info(nullptr)
    , m_video_handle(nullptr)
    , m_control_handle(nullptr)
    , m_cmd_handle(nullptr)
    , m_v4l2_handle(nullptr)
    , m_running(false)
    , m_initialized(false)
    , m_min_temp(20.0f)
    , m_max_temp(100.0f)
{
    std::cout << "ThermalCamera constructor called" << std::endl;
}

ThermalCamera::~ThermalCamera() {
    stop();
    cleanup();
    std::cout << "ThermalCamera destructor called" << std::endl;
}

bool ThermalCamera::initialize(const single_config& config) {
    std::cout << "Initializing thermal camera..." << std::endl;
    
    m_config = config;
    
    // Initialize control interface
    if (!initializeControl()) {
        std::cerr << "Failed to initialize control interface" << std::endl;
        return false;
    }
    
    // Initialize video interface
    if (!initializeVideo()) {
        std::cerr << "Failed to initialize video interface" << std::endl;
        return false;
    }
    
    // Initialize display
    if (!initializeDisplay()) {
        std::cerr << "Failed to initialize display" << std::endl;
        return false;
    }
    
    m_initialized = true;
    std::cout << "Thermal camera initialized successfully" << std::endl;
    return true;
}

bool ThermalCamera::initializeControl() {
    std::cout << "Initializing control interface..." << std::endl;
    
    // Create control handle
    ir_control_handle_create(&m_control_handle);
    if (!m_control_handle) {
        std::cerr << "Failed to create control handle" << std::endl;
        return false;
    }
    
    // Initialize based on control type
    if (m_config.control.is_uart_control) {
        std::cout << "Using UART control interface" << std::endl;
        
        // Create UART handle
        IruartHandle_t* uart_handle = iruart_handle_create(m_control_handle);
        if (!uart_handle) {
            std::cerr << "Failed to create UART handle" << std::endl;
            return false;
        }
        
        // Open UART device
        std::string uart_device = "/dev/ttyUSB0"; // Default, should be configured
        int ret = m_control_handle->ir_control_open(m_control_handle->ir_control_handle, (void*)uart_device.c_str());
        if (ret != IRLIB_SUCCESS) {
            std::cerr << "Failed to open UART device: " << uart_device << std::endl;
            return false;
        }
        
        // Initialize UART
        UartConDevParams_t uart_params = {0};
        uart_params.baudrate = 115200;
        // Note: reserved field is an array, not a single int
        ret = m_control_handle->ir_control_init(m_control_handle->ir_control_handle, &uart_params);
        if (ret != IRLIB_SUCCESS) {
            std::cerr << "Failed to initialize UART" << std::endl;
            return false;
        }
        
    } else if (m_config.control.is_i2c_control) {
        std::cout << "Using I2C control interface" << std::endl;
        
        // Create I2C handle
        Iri2cHandle_t* i2c_handle = iri2c_handle_create(m_control_handle);
        if (!i2c_handle) {
            std::cerr << "Failed to create I2C handle" << std::endl;
            return false;
        }
        
        // Open I2C device
        std::string i2c_device = m_config.control.i2c_param.dev_name;
        int ret = m_control_handle->ir_control_open(m_control_handle->ir_control_handle, (void*)i2c_device.c_str());
        if (ret != IRLIB_SUCCESS) {
            std::cerr << "Failed to open I2C device: " << i2c_device << std::endl;
            return false;
        }
    }
    
    // Create command handle
    m_cmd_handle = ircmd_create_handle(m_control_handle);
    if (!m_cmd_handle) {
        std::cerr << "Failed to create command handle" << std::endl;
        return false;
    }
    
    // Get device information
    char device_name[64] = {0};
    int ret = basic_device_info_get(m_cmd_handle, BASIC_DEV_NAME, device_name);
    if (ret == IRLIB_SUCCESS) {
        m_device_name = std::string(device_name);
        std::cout << "Device name: " << m_device_name << std::endl;
    }
    
    std::cout << "Control interface initialized successfully" << std::endl;
    return true;
}

bool ThermalCamera::initializeVideo() {
    std::cout << "Initializing video interface..." << std::endl;
    
    // Create video handle
    ir_video_handle_create(&m_video_handle);
    if (!m_video_handle) {
        std::cerr << "Failed to create video handle" << std::endl;
        return false;
    }
    
    // Create V4L2 handle
    m_v4l2_handle = (Irv4l2VideoHandle_t*)irv4l2_handle_create(m_video_handle);
    if (!m_v4l2_handle) {
        std::cerr << "Failed to create V4L2 handle" << std::endl;
        return false;
    }
    
    std::cout << "Video interface initialized successfully" << std::endl;
    return true;
}

bool ThermalCamera::initializeDisplay() {
    std::cout << "Initializing display interface..." << std::endl;
    
    // For Jetson, we'll use OpenCV for display
    // This is a placeholder - actual implementation would depend on the specific display requirements
    std::cout << "Display interface initialized successfully" << std::endl;
    return true;
}

bool ThermalCamera::start() {
    if (!m_initialized) {
        std::cerr << "Camera not initialized" << std::endl;
        return false;
    }
    
    std::cout << "Starting thermal camera stream..." << std::endl;
    
    m_running = true;
    
    // Start threads
    m_stream_thread = std::thread(streamThread, this);
    m_display_thread = std::thread(displayThread, this);
    m_command_thread = std::thread(commandThread, this);
    
    std::cout << "Thermal camera stream started" << std::endl;
    return true;
}

void ThermalCamera::stop() {
    if (!m_running) {
        return;
    }
    
    std::cout << "Stopping thermal camera stream..." << std::endl;
    
    m_running = false;
    
    // Wait for threads to finish
    if (m_stream_thread.joinable()) {
        m_stream_thread.join();
    }
    if (m_display_thread.joinable()) {
        m_display_thread.join();
    }
    if (m_command_thread.joinable()) {
        m_command_thread.join();
    }
    
    std::cout << "Thermal camera stream stopped" << std::endl;
}

bool ThermalCamera::isRunning() const {
    return m_running;
}

bool ThermalCamera::processFrame() {
    // This is a simplified version - actual implementation would process frames from the stream
    if (!m_running) {
        return false;
    }
    
    // Simulate frame processing
    usleep(33000); // ~30 FPS
    return true;
}

cv::Mat ThermalCamera::getThermalImage() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_thermal_image.clone();
}

cv::Mat ThermalCamera::getVisibleImage() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_visible_image.clone();
}

cv::Mat ThermalCamera::getTemperatureData() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_temperature_data.clone();
}

bool ThermalCamera::saveFrame(const std::string& filename) const {
    cv::Mat thermal_img = getThermalImage();
    if (thermal_img.empty()) {
        std::cerr << "No thermal image available to save" << std::endl;
        return false;
    }
    
    return cv::imwrite(filename, thermal_img);
}

void ThermalCamera::setTemperatureRange(float min_temp, float max_temp) {
    m_min_temp = min_temp;
    m_max_temp = max_temp;
    std::cout << "Temperature range set to: " << min_temp << "°C - " << max_temp << "°C" << std::endl;
}

std::string ThermalCamera::getDeviceName() const {
    return m_device_name;
}

std::string ThermalCamera::getFirmwareVersion() const {
    return m_firmware_version;
}

void* ThermalCamera::streamThread(void* arg) {
    ThermalCamera* camera = static_cast<ThermalCamera*>(arg);
    
    std::cout << "Stream thread started" << std::endl;
    
    while (camera->m_running) {
        // Process video stream
        // This is where the actual video processing would happen
        usleep(33000); // ~30 FPS
    }
    
    std::cout << "Stream thread ended" << std::endl;
    return nullptr;
}

void* ThermalCamera::displayThread(void* arg) {
    ThermalCamera* camera = static_cast<ThermalCamera*>(arg);
    
    std::cout << "Display thread started" << std::endl;
    
    while (camera->m_running) {
        // Handle display updates
        usleep(33000); // ~30 FPS
    }
    
    std::cout << "Display thread ended" << std::endl;
    return nullptr;
}

void* ThermalCamera::commandThread(void* arg) {
    ThermalCamera* camera = static_cast<ThermalCamera*>(arg);
    
    std::cout << "Command thread started" << std::endl;
    
    while (camera->m_running) {
        // Handle command processing
        usleep(100000); // 100ms
    }
    
    std::cout << "Command thread ended" << std::endl;
    return nullptr;
}

void ThermalCamera::cleanup() {
    std::cout << "Cleaning up thermal camera resources..." << std::endl;
    
    // Cleanup handles in reverse order
    if (m_cmd_handle) {
        ircmd_delete_handle(m_cmd_handle);
        m_cmd_handle = nullptr;
    }
    
    if (m_control_handle) {
        if (m_control_handle->ir_control_handle) {
            m_control_handle->ir_control_close(m_control_handle->ir_control_handle);
        }
        ir_control_handle_delete(&m_control_handle);
        m_control_handle = nullptr;
    }
    
    if (m_v4l2_handle) {
        irv4l2_handle_delete(m_v4l2_handle);
        m_v4l2_handle = nullptr;
    }
    
    if (m_video_handle) {
        ir_video_handle_delete(&m_video_handle);
        m_video_handle = nullptr;
    }
    
    std::cout << "Cleanup completed" << std::endl;
}
