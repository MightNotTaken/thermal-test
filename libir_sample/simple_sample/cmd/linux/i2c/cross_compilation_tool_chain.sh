#!/bin/sh
EXTERN_LIB_CMAKE=extern_lib.cmake
rm ${EXTERN_LIB_CMAKE}

# 请使用自己的编译链替换下方的编译链地址和平台名字
# 若包中所带的库文件未包括您所使用的平台，请联系技术支持
PLATFORM_NAME=arm-linux-gnueabihf   # 请使用自己的平台名字替换
echo "set(PLATFORM_NAME arm-linux-gnueabihf)" >> $EXTERN_LIB_CMAKE
echo "set(CMAKE_SYSTEM_NAME Linux)" >> $EXTERN_LIB_CMAKE
echo "set(TOOLCHAIN_PATH /home/public/tool_chain/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin)" >> $EXTERN_LIB_CMAKE                             # 请使用自己的编译链地址替换
echo "set(CMAKE_C_COMPILER /home/public/tool_chain/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)" >> $EXTERN_LIB_CMAKE   # 请使用自己的编译链地址替换
echo "set(CMAKE_C_FLAGS "-I/include")" >> $EXTERN_LIB_CMAKE
echo "set(CMAKE_CXX_COMPILER /home/public/tool_chain/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++)" >> $EXTERN_LIB_CMAKE # 请使用自己的编译链地址替换
echo "set(CMAKE_FIND_ROOT_PATH )" >> $EXTERN_LIB_CMAKE
echo "set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> $EXTERN_LIB_CMAKE
echo "set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> $EXTERN_LIB_CMAKE
echo "set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> $EXTERN_LIB_CMAKE
echo "list(APPEND CMAKE_PREFIX_PATH )" >> $EXTERN_LIB_CMAKE

mkdir build
cd build
cmake ..
make

dir=$(pwd)"/"
DIR_SO=$dir"../../../../../../libir_SDK_release/linux/${PLATFORM_NAME}/"

cp "../../../../../../libir_SDK_release/linux/${PLATFORM_NAME}/"*.so "./"
