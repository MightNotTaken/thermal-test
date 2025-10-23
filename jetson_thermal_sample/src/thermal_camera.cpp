#include "thermal_camera.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <chrono>
#include <thread>
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
    , m_video_streaming(false)
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
    m_video_streaming = false;
    
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
    if (m_video_stream_thread.joinable()) {
        m_video_stream_thread.join();
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

bool ThermalCamera::startVideoStream() {
    if (!m_initialized) {
        std::cerr << "Camera not initialized" << std::endl;
        return false;
    }
    
    if (m_video_streaming) {
        std::cout << "Video stream already running" << std::endl;
        return true;
    }
    
    std::cout << "Starting video stream..." << std::endl;
    
    m_video_streaming = true;
    m_video_stream_thread = std::thread(videoStreamThread, this);
    
    std::cout << "Video stream started" << std::endl;
    return true;
}

void ThermalCamera::stopVideoStream() {
    if (!m_video_streaming) {
        return;
    }
    
    std::cout << "Stopping video stream..." << std::endl;
    
    m_video_streaming = false;
    
    if (m_video_stream_thread.joinable()) {
        m_video_stream_thread.join();
    }
    
    // Close OpenCV windows
    cv::destroyAllWindows();
    
    std::cout << "Video stream stopped" << std::endl;
}

bool ThermalCamera::isVideoStreaming() const {
    return m_video_streaming;
}

void* ThermalCamera::videoStreamThread(void* arg) {
    ThermalCamera* camera = static_cast<ThermalCamera*>(arg);
    
    std::cout << "Video stream thread started" << std::endl;
    
    // Create OpenCV window
    cv::namedWindow("Thermal Camera Stream", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Temperature Visualization", cv::WINDOW_AUTOSIZE);
    
    cv::Mat thermal_frame;
    cv::Mat temperature_vis;
    cv::Mat visible_frame;
    
    int frame_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    while (camera->m_video_streaming) {
        try {
            // Simulate frame capture from thermal camera
            // In real implementation, this would capture from V4L2 stream
            thermal_frame = camera->simulateThermalFrame();
            temperature_vis = camera->createTemperatureVisualization(thermal_frame);
            visible_frame = camera->simulateVisibleFrame();
            
            if (!thermal_frame.empty()) {
                // Display thermal image
                cv::imshow("Thermal Camera Stream", thermal_frame);
                
                // Display temperature visualization
                cv::imshow("Temperature Visualization", temperature_vis);
                
                // Add frame info overlay
                camera->addFrameInfoOverlay(thermal_frame, frame_count);
                
                // Update frame counter
                frame_count++;
                
                // Calculate and display FPS
                if (frame_count % 30 == 0) {
                    auto current_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
                    double fps = 30000.0 / duration.count();
                    std::cout << "FPS: " << fps << std::endl;
                    start_time = current_time;
                }
            }
            
            // Handle keyboard input
            char key = cv::waitKey(1) & 0xFF;
            if (key == 'q' || key == 27) { // 'q' or ESC
                std::cout << "User requested exit" << std::endl;
                camera->m_video_streaming = false;
                break;
            } else if (key == 's') { // Save frame
                std::string filename = "thermal_frame_" + std::to_string(frame_count) + ".png";
                camera->saveFrame(filename);
                std::cout << "Frame saved: " << filename << std::endl;
            } else if (key == 't') { // Toggle temperature range
                camera->toggleTemperatureRange();
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in video stream thread: " << e.what() << std::endl;
        }
        
        // Control frame rate (30 FPS)
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    
    cv::destroyAllWindows();
    std::cout << "Video stream thread ended" << std::endl;
    return nullptr;
}

cv::Mat ThermalCamera::simulateThermalFrame() {
    // Create a simulated thermal image (640x480)
    cv::Mat thermal_frame = cv::Mat::zeros(480, 640, CV_8UC1);
    
    // Add some thermal patterns
    cv::circle(thermal_frame, cv::Point(320, 240), 50, cv::Scalar(255), -1);
    cv::circle(thermal_frame, cv::Point(200, 150), 30, cv::Scalar(200), -1);
    cv::circle(thermal_frame, cv::Point(450, 300), 40, cv::Scalar(180), -1);
    
    // Add some noise
    cv::Mat noise = cv::Mat::zeros(480, 640, CV_8UC1);
    cv::randn(noise, cv::Scalar(0), cv::Scalar(10));
    thermal_frame += noise;
    
    return thermal_frame;
}

cv::Mat ThermalCamera::simulateVisibleFrame() {
    // Create a simulated visible light image
    cv::Mat visible_frame = cv::Mat::zeros(480, 640, CV_8UC3);
    
    // Add some colored patterns
    cv::circle(visible_frame, cv::Point(320, 240), 50, cv::Scalar(0, 255, 0), -1);
    cv::circle(visible_frame, cv::Point(200, 150), 30, cv::Scalar(255, 0, 0), -1);
    cv::circle(visible_frame, cv::Point(450, 300), 40, cv::Scalar(0, 0, 255), -1);
    
    return visible_frame;
}

cv::Mat ThermalCamera::createTemperatureVisualization(const cv::Mat& thermal_frame) {
    cv::Mat temp_vis;
    
    // Apply colormap for temperature visualization
    cv::applyColorMap(thermal_frame, temp_vis, cv::COLORMAP_JET);
    
    // Add temperature scale
    cv::Mat scale = cv::Mat::zeros(480, 50, CV_8UC3);
    for (int i = 0; i < 480; i++) {
        int color_value = (i * 255) / 480;
        cv::Scalar color;
        cv::applyColorMap(cv::Mat(1, 1, CV_8UC1, color_value), color, cv::COLORMAP_JET);
        scale.row(i) = color[0];
    }
    
    // Combine thermal image with scale
    cv::hconcat(thermal_frame, scale, temp_vis);
    
    return temp_vis;
}

void ThermalCamera::addFrameInfoOverlay(cv::Mat& frame, int frame_count) {
    // Add frame information overlay
    std::string info = "Frame: " + std::to_string(frame_count);
    cv::putText(frame, info, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255), 2);
    
    std::string temp_range = "Temp: " + std::to_string(m_min_temp) + "°C - " + std::to_string(m_max_temp) + "°C";
    cv::putText(frame, temp_range, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255), 1);
    
    std::string controls = "Controls: 'q'=quit, 's'=save, 't'=temp range";
    cv::putText(frame, controls, cv::Point(10, frame.rows - 20), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255), 1);
}

void ThermalCamera::toggleTemperatureRange() {
    // Toggle between different temperature ranges
    static int range_index = 0;
    float ranges[][2] = {{20.0f, 100.0f}, {0.0f, 50.0f}, {50.0f, 150.0f}};
    
    range_index = (range_index + 1) % 3;
    m_min_temp = ranges[range_index][0];
    m_max_temp = ranges[range_index][1];
    
    std::cout << "Temperature range changed to: " << m_min_temp << "°C - " << m_max_temp << "°C" << std::endl;
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
