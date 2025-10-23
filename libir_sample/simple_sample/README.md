# 简介  
该目录提供了一些基础的模组示例，如向设备下发命令以及实现出图的功能。通过这些示例，用户可以方便的使用起我们的模组/机芯，快速上手。  
此外，用户可以修改这些示例或根据这些示例及相关文档，实现自己需要的功能。  

**Tips**: 目录下的所有Windows示例在安装有Visual Studio 2019的情况下可以直接进行编译。Linux示例配置好编译工具链的相关信息之后也可以直接进行编译。  
**WARNING**: 请不要更改当前目录下的结构。所有的编译脚本及配置文件均按照当前的相对路径进行配置，若变更目录结构，请自行调整相关的编译脚本和配置文件。 (Linux需要修改CMakeLists.txt文件和cross_compilation_tool_chain.sh文件，Windows需要修改build.bat和msvc/内的相关文件（info_line是非交叉编译的程序，请自行安装相关编译工具）。(*注：包括但不限于上述文件*)  

# 目录结构
```
simple_sample/
│
├─cmd/              使用命令控制机芯/模组
│  ├─linux/
│  └─windows/
│
├─cmd_camera/       出图
│  ├─linux/
│  └─windows/
│
└─info_line/        信息行解析
```
