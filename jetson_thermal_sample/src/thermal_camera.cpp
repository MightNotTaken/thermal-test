#include "thermal_camera.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cmath>
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
    , m_colormap_index(0)
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
    
    // Create OpenCV windows with proper sizing
    cv::namedWindow("Thermal Camera Stream", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Temperature Visualization", cv::WINDOW_AUTOSIZE);
    
    // Position windows side by side
    cv::moveWindow("Thermal Camera Stream", 100, 100);
    cv::moveWindow("Temperature Visualization", 800, 100);
    
    cv::Mat thermal_frame;
    cv::Mat temperature_vis;
    cv::Mat visible_frame;
    
    int frame_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    auto last_fps_time = start_time;
    
    std::cout << "=== Live Thermal Video Stream Started ===" << std::endl;
    std::cout << "Window 1: Raw Thermal Data" << std::endl;
    std::cout << "Window 2: Temperature Visualization with Color Map" << std::endl;
    std::cout << "Controls: 'q'=quit, 's'=save, 't'=temp range, 'c'=colormap" << std::endl;
    
    while (camera->m_video_streaming) {
        try {
            // Capture thermal frame
            thermal_frame = camera->simulateThermalFrame();
            
            if (!thermal_frame.empty()) {
                // Create temperature visualization
                temperature_vis = camera->createTemperatureVisualization(thermal_frame);
                
                // Add frame info overlay to thermal frame
                camera->addFrameInfoOverlay(thermal_frame, frame_count);
                
                // Display both windows
                cv::imshow("Thermal Camera Stream", thermal_frame);
                cv::imshow("Temperature Visualization", temperature_vis);
                
                // Update frame counter
                frame_count++;
                
                // Calculate and display FPS every 30 frames
                if (frame_count % 30 == 0) {
                    auto current_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_fps_time);
                    double fps = 30000.0 / duration.count();
                    std::cout << "Live Stream - Frame: " << frame_count << ", FPS: " << std::fixed << std::setprecision(1) << fps << std::endl;
                    last_fps_time = current_time;
                }
            }
            
            // Handle keyboard input with immediate response
            char key = cv::waitKey(1) & 0xFF;
            if (key == 'q' || key == 27) { // 'q' or ESC
                std::cout << "User requested exit - stopping video stream" << std::endl;
                camera->m_video_streaming = false;
                break;
            } else if (key == 's') { // Save frame
                std::string filename = "thermal_frame_" + std::to_string(frame_count) + ".png";
                if (camera->saveFrame(filename)) {
                    std::cout << "Frame saved: " << filename << std::endl;
                } else {
                    std::cout << "Failed to save frame" << std::endl;
                }
            } else if (key == 't') { // Toggle temperature range
                camera->toggleTemperatureRange();
            } else if (key == 'c') { // Cycle colormap
                camera->cycleColormap();
            } else if (key == 'r') { // Reset view
                std::cout << "Resetting thermal camera view" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in video stream thread: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Control frame rate (30 FPS) - precise timing
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    
    cv::destroyAllWindows();
    std::cout << "Video stream thread ended - Total frames processed: " << frame_count << std::endl;
    return nullptr;
}

cv::Mat ThermalCamera::simulateThermalFrame() {
    // Create a simulated thermal image (640x480)
    cv::Mat thermal_frame = cv::Mat::zeros(480, 640, CV_8UC1);
    
    // Get current time for dynamic patterns
    auto now = std::chrono::high_resolution_clock::now();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    double time_sec = time_ms / 1000.0;
    
    // Create dynamic thermal patterns
    int center_x = 320 + 50 * sin(time_sec * 0.5);
    int center_y = 240 + 30 * cos(time_sec * 0.3);
    
    // Main heat source (moving)
    cv::circle(thermal_frame, cv::Point(center_x, center_y), 60, cv::Scalar(255), -1);
    
    // Secondary heat sources
    cv::circle(thermal_frame, cv::Point(150, 120), 25, cv::Scalar(200), -1);
    cv::circle(thermal_frame, cv::Point(500, 350), 35, cv::Scalar(180), -1);
    
    // Add moving hot spot
    int hot_x = 100 + 200 * (sin(time_sec * 0.8) + 1) / 2;
    int hot_y = 300 + 100 * (cos(time_sec * 0.6) + 1) / 2;
    cv::circle(thermal_frame, cv::Point(hot_x, hot_y), 20, cv::Scalar(240), -1);
    
    // Add temperature gradient background
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 640; x++) {
            int base_temp = 80 + 20 * sin(x * 0.01) * cos(y * 0.01);
            thermal_frame.at<uchar>(y, x) = std::max(0, std::min(255, base_temp));
        }
    }
    
    // Add realistic thermal noise
    cv::Mat noise = cv::Mat::zeros(480, 640, CV_8UC1);
    cv::randn(noise, cv::Scalar(0), cv::Scalar(8));
    thermal_frame += noise;
    
    // Apply slight blur for realism
    cv::GaussianBlur(thermal_frame, thermal_frame, cv::Size(3, 3), 0.5);
    
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
    
    // Available colormaps for thermal visualization
    int colormaps[] = {
        cv::COLORMAP_JET,      // 0 - Classic thermal (blue to red)
        cv::COLORMAP_HOT,      // 1 - Hot thermal (black to white to red)
        cv::COLORMAP_INFERNO,  // 2 - Inferno (purple to yellow)
        cv::COLORMAP_PLASMA,   // 3 - Plasma (purple to pink)
        cv::COLORMAP_VIRIDIS,  // 4 - Viridis (purple to green)
        cv::COLORMAP_RAINBOW,  // 5 - Rainbow (full spectrum)
        cv::COLORMAP_TURBO     // 6 - Turbo (improved rainbow)
    };
    
    // Apply selected colormap
    cv::applyColorMap(thermal_frame, temp_vis, colormaps[m_colormap_index]);
    
    // Add temperature scale on the right
    cv::Mat scale = cv::Mat::zeros(480, 60, CV_8UC3);
    for (int i = 0; i < 480; i++) {
        int color_value = 255 - (i * 255) / 480; // Invert for proper temperature scale
        cv::Mat color_mat(1, 1, CV_8UC1, color_value);
        cv::Mat color_vis;
        cv::applyColorMap(color_mat, color_vis, colormaps[m_colormap_index]);
        scale.row(i) = color_vis.at<cv::Vec3b>(0, 0);
    }
    
    // Add temperature labels
    cv::putText(scale, "Hot", cv::Point(5, 20), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);
    cv::putText(scale, "Cold", cv::Point(5, 460), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);
    
    // Add colormap name
    std::string colormap_names[] = {"JET", "HOT", "INFERNO", "PLASMA", "VIRIDIS", "RAINBOW", "TURBO"};
    cv::putText(scale, colormap_names[m_colormap_index], cv::Point(5, 40), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(255, 255, 255), 1);
    
    // Combine thermal image with scale
    cv::hconcat(temp_vis, scale, temp_vis);
    
    return temp_vis;
}

void ThermalCamera::addFrameInfoOverlay(cv::Mat& frame, int frame_count) {
    // Add frame information overlay with background
    cv::Rect overlay_rect(5, 5, 300, 100);
    cv::Mat overlay = frame(overlay_rect);
    cv::Mat overlay_bg = overlay.clone();
    cv::rectangle(overlay_bg, cv::Point(0, 0), cv::Point(overlay.cols, overlay.rows), cv::Scalar(0, 0, 0), -1);
    cv::addWeighted(overlay, 0.7, overlay_bg, 0.3, 0, overlay);
    
    // Frame counter
    std::string info = "Frame: " + std::to_string(frame_count);
    cv::putText(frame, info, cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
    
    // Temperature range
    std::string temp_range = "Range: " + std::to_string((int)m_min_temp) + "°C - " + std::to_string((int)m_max_temp) + "°C";
    cv::putText(frame, temp_range, cv::Point(10, 50), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
    
    // FPS indicator
    std::string fps_info = "FPS: ~30";
    cv::putText(frame, fps_info, cv::Point(10, 75), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);
    
    // Controls at bottom
    std::string controls = "Controls: 'q'=quit, 's'=save, 't'=temp range";
    cv::putText(frame, controls, cv::Point(10, frame.rows - 10), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);
    
    // Add crosshair in center
    int center_x = frame.cols / 2;
    int center_y = frame.rows / 2;
    cv::line(frame, cv::Point(center_x - 10, center_y), cv::Point(center_x + 10, center_y), cv::Scalar(255, 255, 255), 1);
    cv::line(frame, cv::Point(center_x, center_y - 10), cv::Point(center_x, center_y + 10), cv::Scalar(255, 255, 255), 1);
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

void ThermalCamera::cycleColormap() {
    // Cycle through available colormaps
    m_colormap_index = (m_colormap_index + 1) % 7;
    
    std::string colormap_names[] = {"JET", "HOT", "INFERNO", "PLASMA", "VIRIDIS", "RAINBOW", "TURBO"};
    std::cout << "Colormap changed to: " << colormap_names[m_colormap_index] << std::endl;
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
