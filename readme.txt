============================================================
  STM32 智能环境监测终端
============================================================

一、项目简介
------------------------------------------------------------
基于 STM32F103ZET6（STM32 HAL 库，Keil MDK-ARM 开发）的环境采集监测
终端，实现“采集 - 显示 - 交互 - 存储”完整功能链路：
  - 3 路 ADC 模拟电压采集（光敏电阻 / 电位器等），DMA 循环搬运 + 软件滤波
  - DHT11 温湿度采集
  - 0.96 寸 OLED 四页中文界面，三按键循环翻页
  - 串口命令行交互（115200，DMA 收发，支持一行多命令）
  - PWM 调光、光照阈值超限自动报警
  - W25Q64 SPI Flash 掉电保存阈值 / 亮度 / 页码，上电自动恢复

当前版本：1.3.3
工程文件：Projects/MDK-ARM/atk_f103.uvprojx（用 Keil MDK 打开）

二、目录结构
------------------------------------------------------------
  Drivers/APP/                 应用层（页面、保存、命令业务等）
  Drivers/BSP/                 板级驱动（ADC、DHT11、OLED、Flash 等）
  Drivers/SYSTEM/              sys / delay / usart 基础组件
  Drivers/CMSIS/               ARM CMSIS 标准文件
  Drivers/STM32F1xx_HAL_Driver/ ST 官方 HAL 库
  Users/                       main.c、中断、HAL 配置
  Projects/MDK-ARM/            Keil 工程文件
  archive/                     1.1.0 ~ 1.3.3 历次版本代码快照

三、版权与出处声明
------------------------------------------------------------
1. Drivers/CMSIS 与 Drivers/STM32F1xx_HAL_Driver 目录下的代码为
   STMicroelectronics 官方提供的 CMSIS 与 STM32F1 HAL 库，版权归
   STMicroelectronics 所有，许可条款以各目录内自带的 LICENSE.txt 为准。
2. Drivers/SYSTEM（delay / sys / usart）基于正点原子 STM32F103 教学
   例程修改，版权归正点原子所有，本项目仅用于个人学习交流。
3. Drivers/APP 与 Drivers/BSP 下的应用层代码与业务驱动为本人在学习
   过程中编写实现。
4. 本项目为个人学习与求职作品展示用途，不用于任何商业目的。
