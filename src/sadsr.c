/*
 * sadsrsr.c
 *
 *  Created on: Jul 4, 2012
 *      Author: owen
 */


#include "sadsr.h"

void sadsr_init(sadsr * sadsr){
	sadsr->segment = 3;  // envelope stopped
	sadsr->val = 0;
	sadsr->attack_delta = .01f;
	sadsr->decay_delta = .01f;
	sadsr->zero_flag = 0;
}

float32_t sadsr_process(sadsr * sadsr){

	// 3 segments: 0, 1, 2.  3 is envelope stopped
	if (sadsr->segment == 0){
		sadsr->val = sadsr->val - sadsr->stop_delta;
		if (sadsr->val < 0.f){
			sadsr->val = 0.f;
			sadsr->segment = 1;
			sadsr->zero_flag = 0;   // this is only 1 for a single sample, so it must be checked at audio rate
		}
	}
	else if (sadsr->segment == 1) {
		sadsr->val = sadsr->val + sadsr->attack_delta;
		sadsr->zero_flag = 0;  // no longer 0 amplitude !
		if (sadsr->val > 1.f){
			sadsr->val = 1.f;
			sadsr->segment = 2;

		}
	}
	else if (sadsr->segment == 2){   // stall here until release
		sadsr->val = sadsr->val - sadsr->decay_delta;
		if (sadsr->val < sadsr->sustain_level){
			sadsr->val = sadsr->sustain_level;
			//sadsr->segment = 3;
		}
	}
	else if (sadsr->segment == 3){
		sadsr->val = sadsr->val - sadsr->release_delta;
		if (sadsr->val < 0 ){
			sadsr->val = 0;
			//sadsr->segment = 3;
		}
	}

	return sadsr->val;
}

void sadsr_set(sadsr * sadsr, float32_t a, float32_t d, float32_t r, float32_t sus_level){
	sadsr->sustain_level = sus_level ;
	sadsr->attack_delta = (1.f / a) / SR;
	sadsr->decay_delta = (1.f / d) / SR;
	sadsr->release_delta = (1.f / r) / SR;
}

void sadsr_go(sadsr * sadsr){
	// reset
	sadsr->segment = 1;
	sadsr->stop_delta = .1f;//sadsr->val / 16.f; // take 16 samples to 0
}

void sadsr_release(sadsr * sadsr){
	sadsr->segment = 3;
}

uint8_t sadsr_zero_flag(sadsr * sadsr){
	return sadsr->zero_flag;
}
