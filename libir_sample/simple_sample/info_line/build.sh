#!/bin/sh
EXTERN_LIB_CMAKE=extern_lib.cmake
rm ${EXTERN_LIB_CMAKE}

rm -rf build_linux
mkdir build_linux
cd build_linux
cmake ..
cmake --build .

mv ./sample_info_line ../
rm -rf *
mv ../sample_info_line ./

cd ..
cp ../../../libir_SDK_release/linux/x64/libirinfoparse.so ./build_linux/
cp ./info_data.bin ./build_linux/
