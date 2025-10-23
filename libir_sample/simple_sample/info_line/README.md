# 简介  
本实例旨在引导用户使用正确使用libir_infoparse库解析信息行  
> **Tips**:  
> 1.本示例无需使用模组/机芯，本示例使用保存好的一帧带信息行的数据(info_data.bin)进行演示(该数据不包含图像数据，仅有信息行数据)。提供了Windows平台和Linux平台的编译脚本  
> 2.本示例中Liunx默认运行的平台是x64，不需要进行交叉编译，如果平台为arm或其他架构，请自行编译  
**Warning:** 本示例请直接在编译的平台运行，无需在特定的平台运行。在Windows编译运行时请自行安装cmake及相关编译工具链，Linux平台同理。  

# 目录结构
```
info_line/
│
├─build.bat                         // Windows下使用的编译脚本
│
├─build.sh                          // Linux下使用的编译脚本
│
├─CMakeLists.txt                    // CMake文件
│
├─info_data.bin                     // 信息行数据
│
├─README.md                         // 说明文档
│
└─smaple.cpp                        // 程序源码
```

# 编译
**前提条件**: 需要CMake及相关编译工具  
**执行编译**: 命令行执行 "build.bat"(Windows) 或 "sh build.sh"(Linux)  
**编译完成**：编译成功后会在当前路径下生成build_win (或build_linux) 文件夹，build_win (或build_linux) 文件夹中为编译好的程序和使用到的帧文件以及相关库文件  
**编译后的目录结构**:  
```
info_line/
│
├─build_win/ (或build_linux/)        // 可执行文件、相关库以及信息行数据文件
│
├─CMakelists.txt
│
├─cross_compilation_tool_chain.sh
│
├─extern_lib.cmake                  // 脚本生成的cmake文件，在编译过程中使用
│
├─README.md
│
└─smaple.cpp
```

# 运行示例
命令行执行 "sample_info_line" 或  "sample_info_line.exe"
命令行会输出从文件夹下info_data.bin文件中信息行中读取到的数据  