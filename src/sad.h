/*
 * sad.h
 *
 *  Created on: Jul 4, 2012
 *      Author: owen
 */

#ifndef SAD_H_
#define SAD_H_

#include "arm_math.h"
#include "audio.h"


typedef struct {
	float32_t val;
	float32_t attack_time;
	float32_t decay_time;
	float32_t attack_delta;
	float32_t decay_delta;
	float32_t stop_delta;
	uint8_t segment;
} sad;

void sad_init(sad * sad);
float32_t sad_process(sad * sad);
void sad_set(sad * sad, float32_t a, float32_t d);
void sad_go(sad * sad);

#endif /* SAD_H_ */
