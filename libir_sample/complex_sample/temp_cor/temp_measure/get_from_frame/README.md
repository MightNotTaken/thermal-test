# 简介  
当前路径下的示例展示了Windows平台通过数据帧中的温度数据计算温度的流程  
此代码基于cmd_camera/windows中的示例修改而来，删除了与计算温度无关的注释  
如果想要了解更多内容，请参考cmd_camera/windows中的示例中的注释进行理解  

# 目录结构
```
get_from_frame/
│
├─msvc/         // Visual Studio项目文件
│
├─build.bat     // 编译脚本
│
├─README.md     // 说明文档
│
└─smaple.cpp    // 程序源码
```

# 修改源码
示例默认使用384测温标准版的相关参数  
所有内容请参照cmd_camera/windows中的示例进行修改  

# 编译
**编译前修改**: 安装好相关usb驱动。usb类型的程序，需要在程序中指定设备的pid和vid（可以在设备管理器中查询到/随设备提供），否则将无法正常操作设备。  
**前提条件**: 安装好Visual Studio (并非Visutal Studio Code)  
**执行编译**: 双击build.bat脚本执行编译 (默认编译版本为64位Release版本)  
**编译完成**：编译成功后会在当前路径下生成log文件夹和output文件夹，output中为编译好的程序和程序运行时使用的dll文件  
**编译后的目录结构**:  
```
get_from_frame/
│
├─log/                  // 日志文件
│
├─msvc/
│
├─output/x64/Release    // 可执行文件与相关库
│
├─build.bat
│
├─README.md
│
└─smaple.cpp
```

# 运行示例
双击exe文件执行或使用命令行的方式运行  
正常运行的情况下会输出坐标(25，25)的温度 （如需尝试其他坐标，请自行根据修改代码）  
如果想要执行复杂的温度计算（如线温度计算和矩形温度计算），请自行编写代码，本示例仅展示如何根据帧中数据计算温度