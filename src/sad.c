/*
 * sad.c
 *
 *  Created on: Jul 4, 2012
 *      Author: owen
 */

#include "sad.h"

void sad_init(sad * sad){
	sad->segment = 3;  // envelope stopped
	sad->val = 0;
	sad->attack_delta = .01f;
	sad->decay_delta = .01f;
}

float32_t sad_process(sad * sad){

	// 3 segments: 0, 1, 2.  3 is envelope stopped
	if (sad->segment == 0){
		sad->val = sad->val - sad->stop_delta;
		if (sad->val < 0.f){
			sad->val = 0.f;
			sad->segment = 1;
		}
	}
	else if (sad->segment == 1) {
		sad->val = sad->val + sad->attack_delta;
		if (sad->val > 1.f){
			sad->val = 1.f;
			sad->segment = 2;
		}
	}
	else if (sad->segment == 2){
		sad->val = sad->val - sad->decay_delta;
		if (sad->val < 0.f){
			sad->val = 0.f;
			sad->segment = 3;
		}
	}

	return sad->val;
}

void sad_set(sad * sad, float32_t a, float32_t d){

	sad->attack_time = a;
	sad->decay_time = d;

	sad->attack_delta = (1.f / a) / SR;
	sad->decay_delta = (1.f / d) / SR;

}

void sad_go(sad * sad){

	// reset
	sad->segment = 0;
	sad->stop_delta = .01f;//sad->val / 16.f; // take 16 samples to 0

}
