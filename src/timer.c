/*
 * timer.c
 *
 *  Created on: Jul 30, 2012
 *      Author: owen
 */

#ifdef __USE_CMSIS
#include "stm32f4xx.h"
#endif
#include "timer.h"



void timer_init(){

	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	/* TIM2 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 0xFFFFFFFF;
	TIM_TimeBaseStructure.TIM_Prescaler = 31; // Down to 1 MHz (adjust per your clock)
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	/* TIM2 enable counter */
	TIM_Cmd(TIM2, ENABLE);

}


void timer_reset(){
	TIM_SetCounter(TIM2,0);
}


uint32_t timer_get_time() {
	return TIM_GetCounter(TIM2);
}
