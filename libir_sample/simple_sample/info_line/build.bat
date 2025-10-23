@echo off
chcp 65001 >nul
set EXTERN_LIB_CMAKE=extern_lib.cmake
if exist %EXTERN_LIB_CMAKE% (
    del %EXTERN_LIB_CMAKE%
)

set PLATFORM_NAME=x64
echo set(PLATFORM_NAME %PLATFORM_NAME%) >> %EXTERN_LIB_CMAKE%

@REM 创建build_win目录
if not exist build_win (
    mkdir build_win
) else (
    rmdir /s /q build_win
    mkdir build_win
)

@REM 进入 build_win 目录
cd build_win

@REM 运行 CMake 配置
cmake ..

@REM 运行 make
cmake --build .

@REM 将生成的exe文件移动到build_win目录
move Debug\sample_info_line.exe ..\

@REM 删除build_win目录中所有的文件和文件夹
cd ..
rmdir /s /q build_win
mkdir build_win

@REM 将生成的exe文件移动到build_win目录
move .\sample_info_line.exe .\build_win

@REM 将info_data.bin文件复制到build_win目录
copy .\info_data.bin .\build_win

@REM 将dll文件复制到build_win目录
copy ..\..\..\libir_SDK_release\windows\x64\Release\dll\libir_infoparse.dll .\build_win
pause