#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include "thermal_camera.h"
#include "config_parser.h"

// Include SDK headers for version functions
extern "C" {
    #include "libircmd.h"
    #include "libircam.h"
    #include "libiruart.h"
    #include "libirv4l2.h"
}

// Global variables for signal handling
ThermalCamera* g_thermal_camera = nullptr;
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
    std::cout << "=== Jetson Thermal Camera Sample ===" << std::endl;
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
        // Print library versions
        std::cout << "\n=== Library Versions ===" << std::endl;
        std::cout << "IRCMD Version: " << ircmd_version() << std::endl;
        std::cout << "IRCAM Version: " << ircam_version() << std::endl;
        std::cout << "IRUART Version: " << iruart_version() << std::endl;
        std::cout << "IRV4L2 Version: " << libv4l2_version() << std::endl;
        
        // Set log levels
        std::cout << "\n=== Setting Log Levels ===" << std::endl;
        ircmd_log_register(IRCMD_LOG_DEBUG, NULL, NULL);
        iruart_log_register(IRUART_LOG_DEBUG, NULL, NULL);
        irv4l2_log_register(IRV4L2_LOG_DEBUG, NULL, NULL);
        ircam_log_register(IRCAM_LOG_DEBUG, NULL, NULL);
        
        // Parse configuration
        std::cout << "\n=== Loading Configuration ===" << std::endl;
        ConfigParser config_parser;
        if (!config_parser.loadConfig(config_path)) {
            std::cerr << "Failed to load configuration from: " << config_path << std::endl;
            return -1;
        }
        
        // Create thermal camera instance
        std::cout << "\n=== Initializing Thermal Camera ===" << std::endl;
        g_thermal_camera = new ThermalCamera();
        
        if (!g_thermal_camera->initialize(config_parser.getConfig())) {
            std::cerr << "Failed to initialize thermal camera" << std::endl;
            delete g_thermal_camera;
            return -1;
        }
        
        std::cout << "\n=== Starting Thermal Camera Stream ===" << std::endl;
        std::cout << "Press Ctrl+C to stop the application" << std::endl;
        
        // Start the thermal camera
        if (!g_thermal_camera->start()) {
            std::cerr << "Failed to start thermal camera" << std::endl;
            delete g_thermal_camera;
            return -1;
        }
        
        // Main loop
        while (g_running && g_thermal_camera->isRunning()) {
            // Process thermal camera frames
            if (!g_thermal_camera->processFrame()) {
                std::cerr << "Error processing frame" << std::endl;
                break;
            }
            
            // Small delay to prevent excessive CPU usage
            usleep(1000); // 1ms
        }
        
        std::cout << "\n=== Stopping Thermal Camera ===" << std::endl;
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
