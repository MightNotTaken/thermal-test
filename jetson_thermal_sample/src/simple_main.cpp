#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include "simple_thermal_camera.h"

// Global variables for signal handling
SimpleThermalCamera* g_thermal_camera = nullptr;
bool g_running = true;

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ". Shutting down gracefully..." << std::endl;
    g_running = false;
    if (g_thermal_camera) {
        g_thermal_camera->stop();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== Jetson Simple Thermal Camera Sample ===" << std::endl;
    std::cout << "Built for NVIDIA Jetson with Ubuntu 22.04" << std::endl;
    
    // Check command line arguments
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file_path>" << std::endl;
        std::cerr << "Example: " << argv[0] << " config/jetson_thermal.conf" << std::endl;
        return -1;
    }
    
    const char* config_path = argv[1];
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Print OpenCV version
        std::cout << "\n=== Library Versions ===" << std::endl;
        std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
        
        // Create thermal camera instance
        std::cout << "\n=== Initializing Simple Thermal Camera ===" << std::endl;
        g_thermal_camera = new SimpleThermalCamera();
        
        if (!g_thermal_camera->initialize(config_path)) {
            std::cerr << "Failed to initialize thermal camera" << std::endl;
            delete g_thermal_camera;
            return -1;
        }
        
        std::cout << "\n=== Starting Simple Thermal Camera Stream ===" << std::endl;
        std::cout << "Press Ctrl+C to stop the application" << std::endl;
        
        // Start the thermal camera
        if (!g_thermal_camera->start()) {
            std::cerr << "Failed to start thermal camera" << std::endl;
            delete g_thermal_camera;
            return -1;
        }
        
        // Main loop
        int frame_count = 0;
        while (g_running && g_thermal_camera->isRunning()) {
            // Process thermal camera frames
            if (!g_thermal_camera->processFrame()) {
                std::cerr << "Error processing frame" << std::endl;
                break;
            }
            
            // Display frame every 30 frames (1 second at 30fps)
            if (frame_count % 30 == 0) {
                cv::Mat thermal_img = g_thermal_camera->getThermalImage();
                if (!thermal_img.empty()) {
                    std::cout << "Frame " << frame_count << " processed successfully" << std::endl;
                    
                    // Save frame every 300 frames (10 seconds)
                    if (frame_count % 300 == 0) {
                        std::string filename = "/tmp/thermal_frame_" + std::to_string(frame_count) + ".png";
                        if (g_thermal_camera->saveFrame(filename)) {
                            std::cout << "Frame saved to: " << filename << std::endl;
                        }
                    }
                }
            }
            
            frame_count++;
            
            // Small delay to prevent excessive CPU usage
            usleep(1000); // 1ms
        }
        
        std::cout << "\n=== Stopping Simple Thermal Camera ===" << std::endl;
        g_thermal_camera->stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return -1;
    }
    
    // Cleanup
    if (g_thermal_camera) {
        delete g_thermal_camera;
        g_thermal_camera = nullptr;
    }
    
    std::cout << "\n=== Application Terminated ===" << std::endl;
    return 0;
}
