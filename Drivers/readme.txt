    该文件夹下面存放驱动层代码，包括：
    1，BSP：如：LED、BEEP、KEY、EXTI、TIMER等
    2，CMSISI：ARM提供的CMSIS代码（主要包括各种头文件和启动文件（.s文件）	      
    3，STM32F7xx_HAL_Driver：ST提供的F1 HAL库驱动代码
    4，SYSTEM：包括sys、delay和usart三个子文件夹，分别提供时钟配置，延时和串口驱动等关键代码，方便构建工程
