# 针对PC环境的LuatOS集成

## 编译说明

本代码依赖xmake最新版本, 请先安装xmake


### windows平台

需要安装vs 2015及以上的版本

```
build_windows_32bit_msvc.bat
```

### linux平台

```
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt-get install -y lib32z1 binutils:i386 libc6:i386 libgcc1:i386 libstdc++5:i386 libstdc++6:i386 p7zip-full
./build_linux_32bit.sh
```


