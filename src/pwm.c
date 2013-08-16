/*
 * pwm.c
 *
 *  Created on: Jun 27, 2013
 *      Author: owen
 */

#ifdef __USE_CMSIS
#include "stm32f4xx.h"
#endif
#include "pwm.h"


void pwm_init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef               TIM_OCInitStructure;
    GPIO_InitTypeDef                GPIO_InitStructure;
    uint16_t                                period;
    uint16_t                                pulse;

    /* Initialize clock - defined in system_stm32f2xx.h */
    /* May not be needed */

    /* Compute the value for the ARR register to have a period of 20 KHz */
    period = (1000000 / 200 ) - 1;

    /* Compute the CCR1 value to generate a PWN signal with 50% duty cycle */
    pulse = (uint16_t) (((uint32_t) 5 * (period - 1)) / 10);

    /* GPIOA clock enable */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    /* Initialize PA8, Alternative Function, 100Mhz, Output, Push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);

    /* TIM1 clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 , ENABLE);

    /* Timer Base configuration */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler = 167;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* Channel 1 output configuration */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = pulse;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    /* Very much needed.  Enable Preload register on CCR1. */
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);

    /* TIM1 counter enable */
    TIM_Cmd(TIM1, ENABLE);

    /* TIM1 Main Output Enable */
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
}

float32_t pwm_set(float32_t f) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef               TIM_OCInitStructure;
    uint32_t                                period;
    uint32_t                                pulse;

    uint32_t tmp;


    if (f > 0){
		/* Initialize clock - defined in system_stm32f2xx.h */
		/* May not be needed */
    	TIM_CtrlPWMOutputs(TIM1, ENABLE);

		/* Compute the value for the ARR register to have a period of 20 KHz */
		period = (1000000 / f ) - 1;

		/* Compute the CCR1 value to generate a PWN signal with 50% duty cycle */
		pulse = period / 2;

		TIM1->ARR = period;
		TIM1->CCR1 = pulse;

		// for jumping to shorter periods, timer count could end up running away
		if (TIM1->CNT >= period)
			TIM1->CNT = 0;
    }
    else {
        /* TIM1 Main Output disable for 0 freq */
        TIM_CtrlPWMOutputs(TIM1, DISABLE);
    }

    return f;

}

void pwm_test(void){

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef               TIM_OCInitStructure;
    GPIO_InitTypeDef                GPIO_InitStructure;
    uint16_t                                period;
    uint16_t                                pulse;

    /* Initialize clock - defined in system_stm32f2xx.h */
    /* May not be needed */

    /* Compute the value for the ARR register to have a period of 20 KHz */
    period = (1000000 / 200 ) - 1;

    /* Compute the CCR1 value to generate a PWN signal with 50% duty cycle */
    pulse = (uint16_t) (((uint32_t) 5 * (period - 1)) / 10);

    /* GPIOA clock enable */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    /* Initialize PA8, Alternative Function, 100Mhz, Output, Push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);

    /* TIM1 clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 , ENABLE);

    /* Timer Base configuration */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler = 167;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* Channel 1 output configuration */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = pulse;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    /* Very much needed.  Enable Preload register on CCR1. */
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);

    /* TIM1 counter enable */
    TIM_Cmd(TIM1, ENABLE);

    /* TIM1 Main Output Enable */
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

}
