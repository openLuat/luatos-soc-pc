# 如何编译本BSP


## 编译说明

本代码依赖xmake最新版本, 请先安装xmake


### windows平台

需要安装vs 2022版本, 安装"C++ 桌面开发"组件

```
build_windows_32bit_msvc.bat
```

### linux平台

```
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt-get install -y lib32z1 binutils:i386 libc6:i386 libgcc1:i386 libstdc++5:i386 libstdc++6:i386 p7zip-full
./build_linux_32bit.sh
```

## 运行方式

windows 下, 先切换控制台编码集,否则中文会乱码

```
chcp 65001
luatos.exe xxx.lua
```
