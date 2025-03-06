# XYModem

### 1.项目简介：
XYModem是一个串口IAP项目，实现在上位机对设备进行更新和擦除固件。项目使用XModem、YModem协议传输数据，支持使用bin、hex文件或AES-CBC-128加密的文件对设备进行更新。上位机除IAP功能外，还可以充当串口助手使用，方便日常的开发调试。

### 2.项目组成：
项目由三部分组成：
  + 上位机
  + Bootloader
  + 测试App

分别存放在App、Bootloader和Examples文件夹中。

#### 1.上位机：
  软件使用Qt6.5.3开发，依赖Serialport组件和[OpenSLL](https://github.com/openssl/openssl)库。OpenSSL版本要求3.0以上版本，编译安装好后需要在cmake中指定路径。
```cmake
set(OPENSSL_ROOT_DIR "D:/OpenSSL-MinGW64")
```
目录结构：
```
XYModem
└──App
    ├── inc             头文件
    ├── resource        资源文件
    ├── src             源文件
    ├── ui              界面文件
    └── CMakeLists.txt  cmake文件
```
#### 2.Bootloader:
  Bootloader目前仅在STM32F103ZET6上进行过测试，其他型号和平台的MCU需要自行移植测试。

目录结构：
```
XYModem
└──Bootloader
    ├── MDK         Keil工程文件
    ├── CMSIS       启动文件和内核文件
    ├── STDLib      STM32标准库
    ├── Tiny-AES    AES加密库
    ├── IAP         iap功能文件
    └── USER        main、中断和stm32配置文件
```
在IAP文件夹下的是实现功能和移植的文件，其中interface命名的文件是用于配置硬件和接口作用的，xymodem命名的是iap功能实现的主要部分。移植时只需修改interface命名相关的文件，在文件中修改你对应设备的相关参数和实现相关函数。

在程序中解密部分使用的是开源的Tiny-AES库，需要了解其内容的请移步到原作者的仓库：https://github.com/kokke/tiny-AES-c

3. 测试App

  这个就是个简单的测试程序。如果你需要实现自己的app程序，需要注意以下几点：
  + app的可用空间，防止程序超出flash范围。
  + 跳转到app后需要重置向量表。
  + 设置标志位为IMAGE_OK，否则下次上电后会停留在Bootloader中。
  + 标志位必须要与Bootloader里设置的一致。
  + 开启串口，实现上位机指令的解析函数，方便在上位机跳转回Bootloader（如有需要）。

### 3.使用方法：
1. 首先烧录Bootloader到你的设备。
2. 运行上位机，选择对应配置和端口，并连接设备。
3. 载入需要更新的文件，点击更新按键，即可更新固件。
4. 若需要擦除固件，请点击擦除按键。

**注：如果Bootloader中使能密文更新功能，需先加密文件再进行更新操作：**
1. 点击加密固件按键，填写对应的密钥和向量并点击生成按键。
2. 完成后会在原文件路径下生成对应文件名的 `.aes` 文件。
3. 重新载入这个 `.aes` 文件，点击更新按键进行固件更新。

