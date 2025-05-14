# 针对PC环境的LuatOS集成


* [使用说明](doc/usage.md)
* [编译说明](doc/compile.md)
* [设计文档](doc/design.md)

本BSP已经替代主库的`bsp/linux`和`bsp/win32`

## 待完成列表功能

* [x] 支持windows编译,运行
* [x] 支持linux编译,运行
* [x] 支持macos编译,运行
* [x] 交互模式 REPL
* [x] 单文件模式,直接跑main.lua
* [x] 目录模式,把指定目录挂载成/luadb,模拟真实设备的路径
* [ ] ZTT机制(模拟器)的设计和实现
* [ ] 跑通iRTU代码

## 已支持的库

* lua基础库, io/os/table/math/bit等所有自带的库
* luatos基础库,log/rtos/timer
* 外设库, uart/gpio/mcu/fskv
* 网络库, socket/http/mqtt/websocket/sntp,含TLS/SSL
* UI库,   lcd/lvgl
* 工具库, crypto/pack/json/gmssl/iotauth/bit64/zbuff/protobuf等所有工具库

## 授权协议

[MIT License](LICENSE)

