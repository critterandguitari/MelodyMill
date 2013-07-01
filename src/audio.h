/*
 * audio.h
 *
 *  Created on: Apr 1, 2012
 *      Author: owen
 */

#ifndef AUDIO_H_
#define AUDIO_H_

#define ABS(a)	   (((a) < 0) ? -(a) : (a))

//#define BASE_FREQ 32.70f   // C1
//#define BASE_FREQ 65.4063913251f   // C1
#define BASE_FREQ 8.1757989156f   // midi note 0

//#define BASE_FREQ 220.f   // A


#define SR 22374.0f
#define NYQUIST 11260.0f
//#define SR 11025.0f
//#define KR 1378.125f // sr / 16
#define KR 22520.0f // sr / 16
#define TWO_PI 6.283185307f

float32_t c_to_f(float32_t c);
float32_t c_to_f_ratio(float32_t c);

#endif /* AUDIO_H_ */
