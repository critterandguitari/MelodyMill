/*
 * sawtooth.h
 *
 *  Created on: Apr 1, 2012
 *      Author: owen
 */

#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include "arm_math.h"
#include "audio.h"

typedef struct {
	float32_t freq;
	int32_t phase;
	uint32_t phase_step;
} phasor;

typedef struct {
	float32_t freq;
	float32_t amplitude;
	int32_t phase;
	uint32_t phase_step;
} sawtooth_oscillator;

typedef struct {
	float32_t freq;
	float32_t amplitude;
	int32_t phase;
	uint32_t phase_step;
} square_oscillator;

typedef struct {
	float32_t freq;
	float32_t amplitude;
	float32_t amplitude_target;
	int32_t phase;
	uint32_t phase_step;
} sin_oscillator;

typedef struct {
	float32_t freq;
	int32_t phase;
	uint32_t phase_step;
} bl_saw;

typedef struct {
	float32_t freq;
	int32_t phase_1;
	int32_t phase_2;
	uint32_t phase_step;
} bl_square;

typedef struct {
	uint32_t modulator_phase_accum;
	uint32_t carrier_phase_accum;
	uint32_t modulator_phase;
	uint32_t modulator_phase_step;
	uint32_t carrier_phase;
	int32_t carrier_phase_step; // this is a signed int because its sometimes negative frequency
} FM_oscillator;


// 0 - 1 phasor
void phasor_set(phasor * p, float32_t freq);
float32_t phasor_process(phasor * p);

// basic sawtooth oscillator
void sawtooth_set(sawtooth_oscillator * saw, float32_t freq, float32_t amp);
float32_t sawtooth_process(sawtooth_oscillator * saw);

// square wave
void squarewave_set(square_oscillator * square, float32_t freq, float32_t amp);
float32_t squarewave_process(square_oscillator * square);

// sine oscillator
void sin_init(sin_oscillator * oscil);
void sin_set(sin_oscillator * oscil, float32_t freq, float32_t amp);
void sin_reset(sin_oscillator * oscil);
float32_t sin_process(sin_oscillator * oscil);


/////////////////
float32_t simple_sin(float32_t f);
float32_t simple_FM(float32_t freq, float32_t harmonicity, float32_t index);
float32_t FM_oscillator_process (FM_oscillator * fm_osc, float32_t freq, float32_t harmonicity, float32_t index);



//////////////
float32_t bl_step_table_read(float32_t index);
float32_t bl_saw_process(bl_saw * saw);
void bl_saw_set(bl_saw * saw, float32_t freq);
void bl_saw_reset(bl_saw * saw);


float32_t bl_square_process(bl_square * square);
void bl_square_set(bl_square * square, float32_t freq);
void bl_square_init(bl_square * square);


#endif /* SAWTOOTH_H_ */
