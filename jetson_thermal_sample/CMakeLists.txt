PROJECT(jetson_thermal_sample)
CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Release by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set compiler flags for Jetson ARM64
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libir_SDK_release/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libir_sample/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libir_sample/components)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libir_sample/thirdparty/cJSON/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libir_sample/thirdparty/libdrm/include)

# Link directories for Jetson ARM64
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libir_SDK_release/linux/aarch64-linux-gnu)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENCV REQUIRED opencv4)

# Include OpenCV directories
include_directories(${OPENCV_INCLUDE_DIRS})

# Add full executable (with SDK dependencies) - PRIMARY TARGET
add_executable(jetson_thermal_sample
    src/main.cpp
    src/thermal_camera.cpp
    src/thermal_camera.h
    src/config_parser.cpp
    src/config_parser.h
    ../libir_sample/common/config.cpp
    ../libir_sample/common/data.cpp
    ../libir_sample/common/v4l2_camera.cpp
    ../libir_sample/common/drm_display.cpp
    ../libir_sample/components/cmd.cpp
    ../libir_sample/thirdparty/cJSON/src/cJSON.c
    ../libir_sample/thirdparty/libdrm/xf86drm.c
    ../libir_sample/thirdparty/libdrm/xf86drmHash.c
    ../libir_sample/thirdparty/libdrm/xf86drmMode.c
    ../libir_sample/thirdparty/libdrm/xf86drmRandom.c
    ../libir_sample/thirdparty/libdrm/xf86drmSL.c
)

# Add simple executable (no SDK dependencies) - FALLBACK
add_executable(jetson_thermal_sample_simple
    src/simple_main.cpp
    src/simple_thermal_camera.cpp
    src/simple_thermal_camera.h
)

# Link libraries for full version (PRIMARY)
target_link_libraries(jetson_thermal_sample
    ircam
    ircmd
    iruart
    irv4l2
    iri2c
    irspi
    irparse
    pthread
    m
    ${OPENCV_LIBRARIES}
)

# Link libraries for simple version (FALLBACK)
target_link_libraries(jetson_thermal_sample_simple
    ${OPENCV_LIBRARIES}
    pthread
)

# Set RPATH for shared libraries
set_target_properties(jetson_thermal_sample PROPERTIES
    INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/../libir_SDK_release/linux/aarch64-linux-gnu"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Install targets
install(TARGETS jetson_thermal_sample DESTINATION bin)
install(TARGETS jetson_thermal_sample_simple DESTINATION bin)
install(FILES config/jetson_thermal.conf DESTINATION config)
