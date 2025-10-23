PROJECT(info_line)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(EXTERN_LIB extern_lib.cmake)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${EXTERN_LIB})
 file(WRITE ${EXTERN_LIB} "")
endif()
include(${EXTERN_LIB})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libir_SDK_release/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces)

option(BUILD32 "Build x86" OFF)
if(BUILD32)
    message(STATUS "x86")
else()
    message(STATUS "x64")
endif()

if (WIN32)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libir_SDK_release/windows/${PLATFORM_NAME}/Release/dll/)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/windows/${PLATFORM_NAME}/Release/dll/)
elseif (UNIX)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../libir_SDK_release/linux/x64/)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../libs/linux/x64/)
endif()

add_executable(sample_info_line
    ./sample.cpp
    )

if(WIN32)
    target_link_libraries(sample_info_line libir_infoparse)
elseif(UNIX)
    target_link_libraries(sample_info_line libirinfoparse.so)
endif()
