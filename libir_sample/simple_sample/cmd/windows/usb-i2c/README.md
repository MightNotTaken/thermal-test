# 简介  
当前路径下的示例展示了Windows平台下基于usb-i2c的命令下发功能  
usb-i2c指的是使用赛普拉斯转接板将usb接口转为i2c接口

# 目录结构
```
usb-i2c/
│
├─msvc/         // Visual Studio项目文件
│
├─build.bat     // 编译脚本
│
├─README.md     // 说明文档
│
└─smaple.cpp    // 程序源码
```

# 编译
**编译前修改**: 安装好相关usb驱动。usb-i2c类型的程序，需要在程序中指定设备的pid和vid（可以在设备管理器中查询到/随设备提供），否则将无法正常操作设备。  
**前提条件**: 安装好Visual Studio (并非Visutal Studio Code)  
**执行编译**: 双击build.bat脚本执行编译 (默认编译版本为64位Release版本)  
**编译完成**：编译成功后会在当前路径下生成log文件夹和output文件夹，output中为编译好的程序和程序运行时使用的dll文件  
**编译后的目录结构**:  
```
usb-i2c/
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
正常运行的情况下会输出设备名称并执行一次打快门  