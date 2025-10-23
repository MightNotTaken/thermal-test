#ifndef THERMAL_CAMERA_H
#define THERMAL_CAMERA_H

#include <opencv2/opencv.hpp>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include "config_parser.h"

// Forward declarations
struct StreamFrameInfo_t;

// Include SDK headers for proper type definitions
extern "C" {
    #include "libircam.h"
    #include "libircmd.h"
    #include "libiruart.h"
    #include "libirv4l2.h"
    #include "libiri2c.h"
}

// Include config header for single_config
#include "config.h"

class ThermalCamera {
public:
    ThermalCamera();
    ~ThermalCamera();
    
    // Initialize the thermal camera with configuration
    bool initialize(const single_config& config);
    
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
    
    // Get current visible image
    cv::Mat getVisibleImage() const;
    
    // Get temperature data
    cv::Mat getTemperatureData() const;
    
    // Start video streaming with OpenCV display
    bool startVideoStream();
    
    // Stop video streaming
    void stopVideoStream();
    
    // Check if video streaming is active
    bool isVideoStreaming() const;
    
    // Save current frame to file
    bool saveFrame(const std::string& filename) const;
    
    // Set temperature range for visualization
    void setTemperatureRange(float min_temp, float max_temp);
    
    // Get device information
    std::string getDeviceName() const;
    std::string getFirmwareVersion() const;
    
private:
    // Internal methods
    bool initializeControl();
    bool initializeVideo();
    bool initializeDisplay();
    void cleanup();
    
    // Thread functions
    static void* streamThread(void* arg);
    static void* displayThread(void* arg);
    static void* commandThread(void* arg);
    static void* videoStreamThread(void* arg);
    
    // Video streaming helper methods
    cv::Mat simulateThermalFrame();
    cv::Mat simulateVisibleFrame();
    cv::Mat createTemperatureVisualization(const cv::Mat& thermal_frame);
    void addFrameInfoOverlay(cv::Mat& frame, int frame_count);
    void toggleTemperatureRange();
    
    // Member variables
    single_config m_config;
    StreamFrameInfo_t* m_stream_info;
    
    // Handles
    IrVideoHandle_t* m_video_handle;
    IrControlHandle_t* m_control_handle;
    IrcmdHandle_t* m_cmd_handle;
    Irv4l2VideoHandle_t* m_v4l2_handle;
    
    // Threading
    std::thread m_stream_thread;
    std::thread m_display_thread;
    std::thread m_command_thread;
    std::thread m_video_stream_thread;
    std::atomic<bool> m_running;
    std::atomic<bool> m_initialized;
    std::atomic<bool> m_video_streaming;
    
    // Data
    mutable std::mutex m_data_mutex;
    cv::Mat m_thermal_image;
    cv::Mat m_visible_image;
    cv::Mat m_temperature_data;
    
    // Temperature range for visualization
    float m_min_temp;
    float m_max_temp;
    
    // Device info
    std::string m_device_name;
    std::string m_firmware_version;
};

#endif // THERMAL_CAMERA_H
