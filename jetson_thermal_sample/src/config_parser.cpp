#include "config_parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cJSON.h>

// Include the config structure from the SDK
#include "config.h"

ConfigParser::ConfigParser() 
    : m_config(nullptr)
    , m_valid(false)
    , m_width(640)
    , m_height(512)
    , m_fps(30)
{
    // Initialize with default values
    m_device_name = "G1280s";
    m_control_type = "uart";
    m_video_device = "/dev/video0";
}

ConfigParser::~ConfigParser() {
    if (m_config) {
        delete m_config;
        m_config = nullptr;
    }
}

bool ConfigParser::loadConfig(const std::string& config_file) {
    m_config_file = config_file;
    m_valid = false;
    m_error_message.clear();
    
    std::cout << "Loading configuration from: " << config_file << std::endl;
    
    // Read file content
    std::ifstream file(config_file);
    if (!file.is_open()) {
        m_error_message = "Failed to open config file: " + config_file;
        std::cerr << m_error_message << std::endl;
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::string json_content = buffer.str();
    if (json_content.empty()) {
        m_error_message = "Config file is empty";
        std::cerr << m_error_message << std::endl;
        return false;
    }
    
    // Parse JSON content
    if (!parseJsonConfig(json_content)) {
        std::cerr << "Failed to parse JSON config: " << m_error_message << std::endl;
        return false;
    }
    
    // Validate configuration
    if (!validateConfig()) {
        std::cerr << "Configuration validation failed: " << m_error_message << std::endl;
        return false;
    }
    
    m_valid = true;
    std::cout << "Configuration loaded successfully" << std::endl;
    return true;
}

bool ConfigParser::parseJsonConfig(const std::string& json_content) {
    cJSON* json = cJSON_Parse(json_content.c_str());
    if (!json) {
        m_error_message = "Failed to parse JSON";
        return false;
    }
    
    // Parse device configuration
    cJSON* device_config = cJSON_GetObjectItem(json, "device");
    if (device_config) {
        cJSON* device_name = cJSON_GetObjectItem(device_config, "name");
        if (device_name && cJSON_IsString(device_name)) {
            m_device_name = std::string(device_name->valuestring);
        }
        
        cJSON* control_type = cJSON_GetObjectItem(device_config, "control_type");
        if (control_type && cJSON_IsString(control_type)) {
            m_control_type = std::string(control_type->valuestring);
        }
    }
    
    // Parse camera configuration
    cJSON* camera_config = cJSON_GetObjectItem(json, "camera");
    if (camera_config) {
        cJSON* video_device = cJSON_GetObjectItem(camera_config, "video_device");
        if (video_device && cJSON_IsString(video_device)) {
            m_video_device = std::string(video_device->valuestring);
        }
        
        cJSON* width = cJSON_GetObjectItem(camera_config, "width");
        if (width && cJSON_IsNumber(width)) {
            m_width = width->valueint;
        }
        
        cJSON* height = cJSON_GetObjectItem(camera_config, "height");
        if (height && cJSON_IsNumber(height)) {
            m_height = height->valueint;
        }
        
        cJSON* fps = cJSON_GetObjectItem(camera_config, "fps");
        if (fps && cJSON_IsNumber(fps)) {
            m_fps = fps->valueint;
        }
    }
    
    cJSON_Delete(json);
    return true;
}

bool ConfigParser::validateConfig() const {
    if (!validateDeviceConfig()) {
        return false;
    }
    
    if (!validateControlConfig()) {
        return false;
    }
    
    if (!validateCameraConfig()) {
        return false;
    }
    
    return true;
}

bool ConfigParser::validateDeviceConfig() const {
    if (m_device_name.empty()) {
        const_cast<ConfigParser*>(this)->m_error_message = "Device name is not specified";
        return false;
    }
    
    return true;
}

bool ConfigParser::validateControlConfig() const {
    if (m_control_type.empty()) {
        const_cast<ConfigParser*>(this)->m_error_message = "Control type is not specified";
        return false;
    }
    
    if (m_control_type != "uart" && m_control_type != "i2c" && m_control_type != "usb") {
        const_cast<ConfigParser*>(this)->m_error_message = "Invalid control type: " + m_control_type;
        return false;
    }
    
    return true;
}

bool ConfigParser::validateCameraConfig() const {
    if (m_video_device.empty()) {
        const_cast<ConfigParser*>(this)->m_error_message = "Video device is not specified";
        return false;
    }
    
    if (m_width <= 0 || m_height <= 0) {
        const_cast<ConfigParser*>(this)->m_error_message = "Invalid resolution: " + std::to_string(m_width) + "x" + std::to_string(m_height);
        return false;
    }
    
    if (m_fps <= 0) {
        const_cast<ConfigParser*>(this)->m_error_message = "Invalid frame rate: " + std::to_string(m_fps);
        return false;
    }
    
    return true;
}

const single_config& ConfigParser::getConfig() const {
    // Create and populate the actual SDK config structure
    static single_config config;
    
    // Initialize control configuration
    config.control.is_uart_control = (m_control_type == "uart");
    config.control.is_i2c_control = (m_control_type == "i2c");
    config.control.is_usb_control = (m_control_type == "usb");
    config.control.is_i2c_usb_control = false;
    
    // Set device names
    if (config.control.is_i2c_control) {
        strncpy(config.control.i2c_param.dev_name, m_video_device.c_str(), sizeof(config.control.i2c_param.dev_name) - 1);
        config.control.i2c_param.dev_name[sizeof(config.control.i2c_param.dev_name) - 1] = '\0';
    }
    
    // Initialize camera configuration
    config.camera.is_auto_image = false;
    config.camera.width = m_width;
    config.camera.height = m_height;
    config.camera.fps = m_fps;
    
    // Set video device
    config.camera.v4l2_config.image_stream.device_name = m_video_device;
    config.camera.v4l2_config.image_stream.fps = m_fps;
    config.camera.v4l2_config.image_stream.dev_width = m_width;
    config.camera.v4l2_config.image_stream.dev_height = m_height;
    config.camera.v4l2_config.has_image = true;
    
    return config;
}

std::string ConfigParser::getConfigAsJson() const {
    cJSON* json = cJSON_CreateObject();
    
    // Device configuration
    cJSON* device = cJSON_CreateObject();
    cJSON_AddStringToObject(device, "name", m_device_name.c_str());
    cJSON_AddStringToObject(device, "control_type", m_control_type.c_str());
    cJSON_AddItemToObject(json, "device", device);
    
    // Camera configuration
    cJSON* camera = cJSON_CreateObject();
    cJSON_AddStringToObject(camera, "video_device", m_video_device.c_str());
    cJSON_AddNumberToObject(camera, "width", m_width);
    cJSON_AddNumberToObject(camera, "height", m_height);
    cJSON_AddNumberToObject(camera, "fps", m_fps);
    cJSON_AddItemToObject(json, "camera", camera);
    
    char* json_string = cJSON_Print(json);
    std::string result(json_string);
    
    free(json_string);
    cJSON_Delete(json);
    
    return result;
}

void ConfigParser::setDeviceName(const std::string& device_name) {
    m_device_name = device_name;
}

void ConfigParser::setControlType(const std::string& control_type) {
    m_control_type = control_type;
}

void ConfigParser::setVideoDevice(const std::string& video_device) {
    m_video_device = video_device;
}

void ConfigParser::setResolution(int width, int height) {
    m_width = width;
    m_height = height;
}

void ConfigParser::setFrameRate(int fps) {
    m_fps = fps;
}

std::string ConfigParser::getDeviceName() const {
    return m_device_name;
}

std::string ConfigParser::getControlType() const {
    return m_control_type;
}

std::string ConfigParser::getVideoDevice() const {
    return m_video_device;
}

int ConfigParser::getWidth() const {
    return m_width;
}

int ConfigParser::getHeight() const {
    return m_height;
}

int ConfigParser::getFrameRate() const {
    return m_fps;
}

bool ConfigParser::isValid() const {
    return m_valid;
}

std::string ConfigParser::getErrorMessage() const {
    return m_error_message;
}
