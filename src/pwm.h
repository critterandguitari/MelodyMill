/*
 * pwm.h
 *
 *  Created on: Jun 27, 2013
 *      Author: owen
 */

#ifndef PWM_H_
#define PWM_H_


#include "arm_math.h"


void pwm_init(void);
void pwm_set(float32_t f);
void pwm_test(void);

#endif /* PWM_H_ */
