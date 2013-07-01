/*
 * sadsr.h
 *
 *  Created on: Jul 4, 2012
 *      Author: owen
 */

#ifndef SADSR_H_
#define SADSR_H_


#include "arm_math.h"
#include "audio.h"

typedef struct {
	float32_t val;
	float32_t attack_delta;
	float32_t decay_delta;
	float32_t release_delta;
	float32_t sustain_level;
	float32_t stop_delta;
	uint8_t segment;
	uint8_t zero_flag;
} sadsr;

void sadsr_init(sadsr * sadsr);

float32_t sadsr_process(sadsr * sadsr);

void sadsr_set(sadsr * sadsr, float32_t a, float32_t d, float32_t r, float32_t sus_level);

void sadsr_go(sadsr * sadsr);
void sadsr_release(sadsr * sadsr);


uint8_t sadsr_zero_flag(sadsr * sadsr);

#endif /* SADSR_H_ */
