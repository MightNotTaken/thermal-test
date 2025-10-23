# 简介  
当前路径下的示例展示了linux平台下基于uvc的出图功能  
控制通道使用usb

# 目录结构
```
uvc_usb/
│
├─CMakeLists.txt                    // CMake文件
│
├─cross_compilation_tool_chain.sh   // 交叉编译脚本
│
├─README.md                         // 说明文档
│
└─smaple.cpp                        // 程序源码
```

# 修改源码
当前源码针对384测温版模组进行编写  
384测温版默认配置为：  
自动出图，出图类型为USB，出图格式为YUYV  
出双图，帧数30  
数据排布为 图像数据+温度数据+信息行数据  
所有数据系数均为2  
数据宽度均为384  
图像数据和温度数据高度为288  
信息行数据高度为2  
在Linux上使用uvc进行出图  

# 编译
**前提条件**: 准备好编译指定平台时所使用的编译工具链并修改cross_compilation_tool_chain.sh中编译链的相关信息  
**执行编译**: 命令行执行"sh cross_compilation_tool_chain.sh"  
**编译完成**：编译成功后会在当前路径下生成build文件夹，build文件夹中为编译好的程序和程序运行时使用到的.so文件  
**编译后的目录结构**:  
```
uart/
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
在目标平台执行使用命令行执行二进制文件"./sample_cmd_camera_linux"  
正常运行的情况下会在二进制文件当前所在的目录下生成frame_data.bin文件  
将该文件使用**专用的图片工具**打开，即可看到图像  
当前示例的数据为图像数据+温度数据+信息行数据  
因此显示的画面将分为三部分，一个"正常"画面，一个与"正常"画面相似的画面，以及最下方的很细的小绿条