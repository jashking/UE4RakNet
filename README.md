# UE4RakNet
RakNet plugin for UE4.

## 测试平台 ##

* 已测试通过
	* `Win32`, `Win64`
	* `Android`
	* `Mac`

* 未测试
	* `Linux`
	
## 如何使用 ##

直接将 `Plugins\RakNet` 文件夹复制到自己的项目源码目录下即可，如果不想用 `RakNetUDPClient` 的封装，也可以直接把 `RakNetUDPClient.h/.cpp` 删除，然后自己封装