PROJECT(cmd_i2c)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(EXTERN_LIB extern_lib.cmake)
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${EXTERN_LIB})
    file(WRITE ${EXTERN_LIB} "")
endif()
include(${EXTERN_LIB})

include_directories(../../../../../libir_SDK_release/include)   # 先按照打包后的路径进行查找
include_directories(../../../../interfaces/)                    # 按照sample项目路径进行查找
include_directories(../../../../drivers/)                       # 按照sample项目路径进行查找

option(BUILD32 "Build x86" OFF)
if(BUILD32)
    message("building x86")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
else()
    message("building x64")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
endif()

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../libir_SDK_release/linux/${PLATFORM_NAME})   # 先按照打包后的路径进行查找
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../libs/linux/${PLATFORM_NAME})                      # 先按照打包后的路径进行查找

add_executable(sample_cmd_i2c
    ./sample.cpp
    )
target_link_libraries(sample_cmd_i2c ircam ircmd iri2c -lm)
