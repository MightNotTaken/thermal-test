#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <vector>
#include <map>

// Include config header for single_config
#include "config.h"

class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser();
    
    // Load configuration from file
    bool loadConfig(const std::string& config_file);
    
    // Get the parsed configuration
    const single_config& getConfig() const;
    
    // Validate configuration
    bool validateConfig() const;
    
    // Get configuration as JSON string
    std::string getConfigAsJson() const;
    
    // Set configuration parameters
    void setDeviceName(const std::string& device_name);
    void setControlType(const std::string& control_type);
    void setVideoDevice(const std::string& video_device);
    void setResolution(int width, int height);
    void setFrameRate(int fps);
    
    // Get configuration parameters
    std::string getDeviceName() const;
    std::string getControlType() const;
    std::string getVideoDevice() const;
    int getWidth() const;
    int getHeight() const;
    int getFrameRate() const;
    
    // Check if configuration is valid
    bool isValid() const;
    
    // Get error message if configuration is invalid
    std::string getErrorMessage() const;

private:
    // Internal methods
    bool parseJsonConfig(const std::string& json_content);
    bool validateDeviceConfig() const;
    bool validateControlConfig() const;
    bool validateCameraConfig() const;
    
    // Member variables
    single_config* m_config;
    mutable bool m_valid;
    mutable std::string m_error_message;
    std::string m_config_file;
    
    // Configuration parameters
    std::string m_device_name;
    std::string m_control_type;
    std::string m_video_device;
    int m_width;
    int m_height;
    int m_fps;
};

#endif // CONFIG_PARSER_H
