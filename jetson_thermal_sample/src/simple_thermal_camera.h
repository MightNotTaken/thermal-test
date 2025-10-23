#ifndef SIMPLE_THERMAL_CAMERA_H
#define SIMPLE_THERMAL_CAMERA_H

#include <opencv2/opencv.hpp>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

class SimpleThermalCamera {
public:
    SimpleThermalCamera();
    ~SimpleThermalCamera();
    
    // Initialize the thermal camera
    bool initialize(const std::string& config_file);
    
    // Start the thermal camera stream
    bool start();
    
    // Stop the thermal camera stream
    void stop();
    
    // Check if the camera is running
    bool isRunning() const;
    
    // Process a single frame
    bool processFrame();
    
    // Get current thermal image
    cv::Mat getThermalImage() const;
    
    // Save current frame to file
    bool saveFrame(const std::string& filename) const;
    
    // Set temperature range for visualization
    void setTemperatureRange(float min_temp, float max_temp);
    
    // Get device information
    std::string getDeviceName() const;
    
private:
    // Internal methods
    void cleanup();
    
    // Thread functions
    static void* streamThread(void* arg);
    
    // Member variables
    std::string m_config_file;
    std::string m_device_name;
    
    // Threading
    std::thread m_stream_thread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_initialized;
    
    // Data
    mutable std::mutex m_data_mutex;
    cv::Mat m_thermal_image;
    
    // Temperature range for visualization
    float m_min_temp;
    float m_max_temp;
};

#endif // SIMPLE_THERMAL_CAMERA_H
