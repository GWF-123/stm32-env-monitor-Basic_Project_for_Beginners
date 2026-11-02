#ifndef __PWM_H
#define __PWM_H

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"

#define PWM_TIM     TIM3
#define PWM_CHANNAL     TIM_CHANNEL_2

void Pwm_Init(uint16_t psc, uint16_t arr);
//void Pwm_Control_Led(void);
void Led_Control(uint16_t light);

#endif
