#include "simple_thermal_camera.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

SimpleThermalCamera::SimpleThermalCamera() 
    : m_running(false)
    , m_initialized(false)
    , m_min_temp(20.0f)
    , m_max_temp(100.0f)
{
    std::cout << "SimpleThermalCamera constructor called" << std::endl;
}

SimpleThermalCamera::~SimpleThermalCamera() {
    stop();
    cleanup();
    std::cout << "SimpleThermalCamera destructor called" << std::endl;
}

bool SimpleThermalCamera::initialize(const std::string& config_file) {
    std::cout << "Initializing simple thermal camera..." << std::endl;
    
    m_config_file = config_file;
    m_device_name = "Simple Thermal Camera";
    
    // Create a dummy thermal image for demonstration
    m_thermal_image = cv::Mat::zeros(480, 640, CV_8UC3);
    
    // Add some thermal-like pattern
    for (int y = 0; y < m_thermal_image.rows; y++) {
        for (int x = 0; x < m_thermal_image.cols; x++) {
            // Create a thermal-like gradient pattern
            float intensity = (float)x / m_thermal_image.cols;
            cv::Vec3b color;
            color[0] = (uchar)(255 * intensity); // Blue
            color[1] = (uchar)(255 * (1.0f - intensity)); // Green
            color[2] = (uchar)(128 + 127 * sin(intensity * 3.14159f)); // Red
            m_thermal_image.at<cv::Vec3b>(y, x) = color;
        }
    }
    
    m_initialized = true;
    std::cout << "Simple thermal camera initialized successfully" << std::endl;
    return true;
}

bool SimpleThermalCamera::start() {
    if (!m_initialized) {
        std::cerr << "Camera not initialized" << std::endl;
        return false;
    }
    
    std::cout << "Starting simple thermal camera stream..." << std::endl;
    
    m_running = true;
    
    // Start thread
    m_stream_thread = std::thread(streamThread, this);
    
    std::cout << "Simple thermal camera stream started" << std::endl;
    return true;
}

void SimpleThermalCamera::stop() {
    if (!m_running) {
        return;
    }
    
    std::cout << "Stopping simple thermal camera stream..." << std::endl;
    
    m_running = false;
    
    // Wait for thread to finish
    if (m_stream_thread.joinable()) {
        m_stream_thread.join();
    }
    
    std::cout << "Simple thermal camera stream stopped" << std::endl;
}

bool SimpleThermalCamera::isRunning() const {
    return m_running;
}

bool SimpleThermalCamera::processFrame() {
    // This is a simplified version - actual implementation would process frames from the stream
    if (!m_running) {
        return false;
    }
    
    // Simulate frame processing
    usleep(33000); // ~30 FPS
    return true;
}

cv::Mat SimpleThermalCamera::getThermalImage() const {
    std::lock_guard<std::mutex> lock(m_data_mutex);
    return m_thermal_image.clone();
}

bool SimpleThermalCamera::saveFrame(const std::string& filename) const {
    cv::Mat thermal_img = getThermalImage();
    if (thermal_img.empty()) {
        std::cerr << "No thermal image available to save" << std::endl;
        return false;
    }
    
    return cv::imwrite(filename, thermal_img);
}

void SimpleThermalCamera::setTemperatureRange(float min_temp, float max_temp) {
    m_min_temp = min_temp;
    m_max_temp = max_temp;
    std::cout << "Temperature range set to: " << min_temp << "°C - " << max_temp << "°C" << std::endl;
}

std::string SimpleThermalCamera::getDeviceName() const {
    return m_device_name;
}

void* SimpleThermalCamera::streamThread(void* arg) {
    SimpleThermalCamera* camera = static_cast<SimpleThermalCamera*>(arg);
    
    std::cout << "Stream thread started" << std::endl;
    
    while (camera->m_running) {
        // Process video stream
        // This is where the actual video processing would happen
        usleep(33000); // ~30 FPS
    }
    
    std::cout << "Stream thread ended" << std::endl;
    return nullptr;
}

void SimpleThermalCamera::cleanup() {
    std::cout << "Cleaning up simple thermal camera resources..." << std::endl;
    std::cout << "Cleanup completed" << std::endl;
}
