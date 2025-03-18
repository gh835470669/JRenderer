# JRenderer

![主界面](docs/主界面.png)

a renderer to learn

## Build, Install And Run

1. git clone https://github.com/gh835470669/JRenderer.git --recurse-submodules
或者 
git clone https://github.com/gh835470669/JRenderer.git 
克隆下来后执行git submodule update --init --recursive
保证子模块也clone
克隆仓库完毕，接下来config项目
2. cd JRenderer
工作目录跳转到项目根目录
3. mkdir build
4. cmake -S. -B./build
等待解决方案生成完毕，接着build项目
5. cmake --build ./build --config Release
生成Relase版可执行文件
然后安装（构造可运行环境：dll和资源目录）
6. mkdir install
7. cmake --install ./build --prefix ./install
安装完毕，工作目录转到安装目录，运行 JRenderApp.exe
8. cd install
9. JRenderApp.exe