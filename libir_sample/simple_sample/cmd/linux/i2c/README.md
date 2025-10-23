# 简介  
当前路径下的示例展示了Linux平台下基于i2c的命令下发功能  

# 目录结构
```
i2c/
│
├─CMakeLists.txt                    // CMake文件
│
├─cross_compilation_tool_chain.sh   // 交叉编译脚本
│
├─README.md                         // 说明文档
│
└─smaple.cpp                        // 程序源码
```

# 编译
**前提条件**: 准备好编译指定平台时所使用的编译工具链并修改cross_compilation_tool_chain.sh中编译链的相关信息  
**执行编译**: 命令行执行"sh cross_compilation_tool_chain.sh"  
**编译完成**：编译成功后会在当前路径下生成build文件夹，build文件夹中为编译好的程序和程序运行时使用到的.so文件  
**编译后的目录结构**:  
```
i2c/
│
├─build/                            // 可执行文件与相关库
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
将编译出来的二进制文件和库一并拷贝到目标平台上  
在目标平台执行使用命令行执行二进制文件"./sample_cmd_i2c"  
如果运行时提示缺少相关库，则执行"LD_LIBRARY_PATH=. ./sample_cmd_i2c"  
正常运行的情况下会输出设备名称并执行一次打快门操作  