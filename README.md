# STM32 智能环境监测终端（裸机 / FreeRTOS 双版本）

> 主控：**STM32F103ZET6**（Cortex-M3，72 MHz）｜固件库：**STM32Cube HAL**｜开发环境：**Keil MDK-ARM**
> 版本：裸机版 **V1.3.3** ｜ FreeRTOS 移植版 **V2.3.3**

围绕「采集 — 显示 — 交互 — 存储」完整链路设计的嵌入式环境监测终端：3 路 ADC 电压采集、DHT11 温湿度检测、OLED 四页中文界面、串口命令行控制、PWM 调光、光照越限报警、参数掉电保存与上电恢复。

本仓库提供两个版本：**裸机版**（前后台 + 中断）与 **FreeRTOS 移植版**（采集 / 串口 / 显示三任务架构），业务代码一致，可对照学习前后台与 RTOS 两种写法。

## 仓库结构

| 目录 | 说明 |
| --- | --- |
| [BareMetal/](./BareMetal) | 裸机版工程，Keil 打开 `BareMetal/Projects/MDK-ARM/atk_f103.uvprojx` |
| [FreeRTOS/](./FreeRTOS) | FreeRTOS 移植版工程，Keil 打开 `FreeRTOS/Projects/MDK-ARM/atk_f103.uvprojx` |
| [docs/](./docs) | 项目简介 Word、演示视频 |

## 资源链接

- 📄 项目简介（Word 版）：[STM32智能环境监测终端-项目简介.docx](./docs/STM32%E6%99%BA%E8%83%BD%E7%8E%AF%E5%A2%83%E7%9B%91%E6%B5%8B%E7%BB%88%E7%AB%AF-%E9%A1%B9%E7%9B%AE%E7%AE%80%E4%BB%8B.docx)
- 📺 演示视频：[docs/video/演示视频.mp4](./docs/video/%E6%BC%94%E7%A4%BA%E8%A7%86%E9%A2%91.mp4)
- 📷 示波器实测波形照片（共 9 张，不直接内嵌，点开查看）：
  - [I2C 总线（OLED / MAX30102）](./BareMetal/docs/images/IIC%E7%9B%B8%E5%85%B3)
  - [PWM 输出](./BareMetal/docs/images/PWM%E7%9B%B8%E5%85%B3)
  - [串口 USART1](./BareMetal/docs/images/%E4%B8%B2%E5%8F%A3%E7%9B%B8%E5%85%B3)

## 快速开始

1. 用 Keil MDK-ARM 打开对应版本的 `Projects/MDK-ARM/atk_f103.uvprojx`，编译并下载到 STM32F103ZET6 开发板（正点原子精英板）；
2. USB 转串口连接 USART1（PA9/PA10），串口助手 115200-8-N-1；
3. 发送 `READ_ADC`、`PWM:500`、`PAGE:2` 等命令交互，KEY0 / KEY1 切换 OLED 页面，`SAVE` 保存参数后断电重启验证恢复。

常用串口命令：`READ_ADC` 查询三路电压｜`LED_ON` / `LED_OFF`｜`LIGHT:n` 光照阈值｜`PWM:n` 亮度｜`PAGE:n` 跳页｜`SAVE` / `LOAD` 参数存取。完整命令表见项目简介 Word。

## FreeRTOS 版说明

- **三任务划分**：`task_collect`（ADC 采集 + 滤波）、`task_usart`（串口命令收发）、`task_display`（OLED 刷新 + 按键扫描），优先级与 1ms 节拍在 `FreeRTOSConfig.h` 中配置；
- **长度 1 队列当邮箱**：`xQueueOverwrite` 只保留最新采集快照，显示任务与串口 `READ_ADC` 均用 `xQueuePeek` 非破坏性读取，保证两边数值一致；
- **中断安全**：调用 FromISR 接口的中断（USART1、DMA1_ADC）抢占优先级均 ≥ `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`（5）。

## 版权与出处

1. `Drivers/CMSIS`、`Drivers/STM32F1xx_HAL_Driver` 版权归 **STMicroelectronics**；`FreeRTOS/` 内核版权归 **Amazon.com, Inc. 或其关联公司**，许可条款以各目录自带 LICENSE 为准；
2. `SYSTEM`（delay / sys / usart）基于**正点原子** STM32F103 教学例程修改，版权归正点原子所有；
3. `APP`、`BSP` 下的应用层与业务驱动为本人学习过程中编写实现；
4. 本项目为个人学习与求职作品展示用途，不用于任何商业目的。
