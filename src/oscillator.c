/*
 * sawtooth.c
 *
 *  Created on: Apr 1, 2012
 *      Author: owen
 */

#include "oscillator.h"
#include "waves.h"

/*
 * phasor oscillator
 */
void phasor_set(phasor * p, float32_t freq) {
	p->phase_step = (uint32_t) (freq * (2147483648.0f / SR));
}

float32_t phasor_process(phasor * p){
	// just let it roll over
	p->phase += p->phase_step;
	return (((float32_t) p->phase / 2147483648.0f) + 1.f) * .5f;
}


/*
 * sawtooth oscillator
 */
void sawtooth_set(sawtooth_oscillator * saw, float32_t freq, float32_t amp) {
	saw->phase_step = (uint32_t) (freq * (2147483648.0f / SR));
	saw->amplitude = amp;
}

float32_t sawtooth_process(sawtooth_oscillator * saw){
	// just let it roll over
	saw->phase += saw->phase_step;
	return ((float32_t) saw->phase / 2147483648.0f) * saw->amplitude;
}

/*
 * square oscillator
 */
void squarewave_set(square_oscillator * square, float32_t freq, float32_t amp) {
	square->phase_step = (uint32_t) (freq * (2147483648.0f / SR));
	square->amplitude = amp;
}

float32_t squarewave_process(square_oscillator * square){
	float32_t squarewave;
	// just let it roll over
	square->phase += square->phase_step;
	if (square->phase > 0) squarewave = 1.0f;
	else squarewave = -1.0f;
	return squarewave * square->amplitude;
}

/*
 * sine oscillator
 */
void sin_init(sin_oscillator * oscil){
	oscil->amplitude = 0.f;
	oscil->freq = 0.f;
	oscil->phase = 0;
	oscil->phase_step = 0;
}

void sin_reset(sin_oscillator * oscil){
	oscil->phase = 0;
}

void sin_set(sin_oscillator * oscil, float32_t freq, float32_t amp){
	if (freq < 0) freq *= -1.f;
	oscil->phase_step = (uint32_t) (freq * (2147483648.0f / SR));
	oscil->amplitude_target = amp;
}

float32_t sin_process(sin_oscillator * oscil){

	float32_t phase;

	oscil->amplitude = oscil->amplitude_target;
	oscil->phase += oscil->phase_step;
	phase = (((float32_t) oscil->phase / 2147483648.0f) + 1.0f) * PI;

	return arm_sin_f32(phase) * oscil->amplitude;

}


//////////////

static inline float cube_interp(float fr, float inm1, float
                                in, float inp1, float inp2)
{
	return in + 0.5f * fr * (inp1 - inm1 +
	 fr * (4.0f * inp1 + 2.0f * inm1 - 5.0f * in - inp2 +
	 fr * (3.0f * (in - inp1) - inm1 + inp2)));
}

float32_t simple_sin(float32_t freq) {
	static uint32_t phase_accum;
	uint32_t phase;
	uint32_t phase_step = (uint32_t) (freq * (2147483648.0f / SR));

	float32_t ym1, y, yp1, yp2, frac;

	phase = ((phase_accum >> 24) & 0xff) + 1;

	ym1 = sin_table[phase - 1];
	y = sin_table[phase];
	yp1 = sin_table[phase + 1];
	yp2 = sin_table[phase + 2];

	//ym1 = saw_table[phase - 1];
	//y = saw_table[phase];
	//yp1 = saw_table[phase + 1];
	//yp2 = saw_table[phase + 2];

	frac =  (float32_t) (phase_accum & 0x00FFFFFF) / 16777216.f;


	phase_accum += phase_step;
	return y + frac * (yp1 - y);
	//return cube_interp(frac, ym1, y, yp1, yp2);
}

// single FM
float32_t simple_FM(float32_t freq, float32_t harmonicity, float32_t index) {

	static uint32_t modulator_phase_accum;
	static uint32_t carrier_phase_accum;
	uint32_t modulator_phase;
	uint32_t modulator_phase_step;
	uint32_t carrier_phase;
	int32_t carrier_phase_step; // this is a signed int because its sometimes negative frequency

	float32_t  y, yp1, frac, modulator, carrier;

	// modulator frequency = f * harmonicity
	modulator_phase_step = (uint32_t) (freq * (2147483648.0f / SR) * harmonicity);

	modulator_phase = ((modulator_phase_accum >> 24) & 0xff) + 1;
	y = sin_table[modulator_phase];
	yp1 = sin_table[modulator_phase + 1];
	frac =  (float32_t) (modulator_phase_accum & 0x00FFFFFF) / 16777216.f;
	modulator_phase_accum += modulator_phase_step;
	modulator = y + frac * (yp1 - y);

	// scale it with index (deviation = mod freq * index)
	modulator *= harmonicity * freq * index;

	// do the fm
	freq += modulator;

	// compute carrier freq
	carrier_phase_step = (int32_t) (freq * (2147483648.0f / SR)) + (int32_t)(modulator * (2147483648.0f / SR));

	// carrier oscillator
	carrier_phase = ((carrier_phase_accum >> 24) & 0xff) + 1;
	y = sin_table[carrier_phase];
	yp1 = sin_table[carrier_phase + 1];
	frac =  (float32_t) (carrier_phase_accum & 0x00FFFFFF) / 16777216.f;
	carrier_phase_accum += carrier_phase_step;

	// interpolate
	carrier = y + frac * (yp1 - y);

	return carrier;
}

float32_t FM_oscillator_process (FM_oscillator * fm_osc, float32_t freq, float32_t harmonicity, float32_t index) {


	float32_t  y, yp1, frac, modulator, carrier;

	// modulator frequency = f * harmonicity
	fm_osc->modulator_phase_step = (uint32_t) (freq * (2147483648.0f / SR) * harmonicity);

	fm_osc->modulator_phase = ((fm_osc->modulator_phase_accum >> 24) & 0xff) + 1;
	y = sin_table[fm_osc->modulator_phase];
	yp1 = sin_table[fm_osc->modulator_phase + 1];
	frac =  (float32_t) (fm_osc->modulator_phase_accum & 0x00FFFFFF) / 16777216.f;
	fm_osc->modulator_phase_accum += fm_osc->modulator_phase_step;
	modulator = y + frac * (yp1 - y);

	// scale it with index (deviation = mod freq * index)
	modulator *= harmonicity * freq * index;

	// do the fm
	freq += modulator;

	// compute carrier freq
	fm_osc->carrier_phase_step = (int32_t) (freq * (2147483648.0f / SR)) + (int32_t)(modulator * (2147483648.0f / SR));

	// carrier oscillator
	fm_osc->carrier_phase = ((fm_osc->carrier_phase_accum >> 24) & 0xff) + 1;
	y = sin_table[fm_osc->carrier_phase];
	yp1 = sin_table[fm_osc->carrier_phase + 1];
	frac =  (float32_t) (fm_osc->carrier_phase_accum & 0x00FFFFFF) / 16777216.f;
	fm_osc->carrier_phase_accum += fm_osc->carrier_phase_step;

	// interpolate
	carrier = y + frac * (yp1 - y);

	return carrier;
}


// step table, floating index
float32_t bl_step_table_read(float32_t index) {
	uint32_t table_index;
	float32_t frac;
	float32_t y, yp1;

	table_index = (uint32_t) index;

	frac = (float32_t) table_index - index;

	y = transition_table[table_index];
	yp1 = transition_table[table_index + 1];
	return y + frac * (yp1 - y);
}

void bl_saw_set(bl_saw * saw, float32_t freq) {
	saw->phase_step = (uint32_t) (freq * (2147483648.0f / SR));
	saw->freq = freq;
}

void bl_saw_reset(bl_saw * saw){
	saw->phase = 0;
}

// from PD patch, transition table is used for the step
float32_t bl_saw_process(bl_saw * saw){

	float32_t sig;
	float32_t transition_table_index;

	saw->phase += saw->phase_step;
	sig = ((((float32_t) saw->phase / 2147483648.0f) + 1.f) * .5f) - .5f;
	//sig -= .5f;

	// scale it, numerator controls the band limit
	transition_table_index = sig * (5000.f / saw->freq); // 4410 is nyquist * .4
	// clip it
	if (transition_table_index > .5f) transition_table_index = .5f;
	if (transition_table_index < -.5f) transition_table_index = -.5f;
	// range it for transistion table
	transition_table_index *= 1000.f;
	transition_table_index += 501.f;

	// transition tabled is railed, so siq passes through, except during the step
	return bl_step_table_read(transition_table_index) - sig;
}

void bl_square_set(bl_square * square, float32_t freq){
	square->phase_step = (uint32_t) (freq * (2147483648.0f / SR));
	square->freq = freq;


}

// this has to be called to set the two saw waves 180 out of phase
void bl_square_init(bl_square * square) {
	// phases must be reset (or do they?)
	square->phase_1 = 0;
	square->phase_2 = 1073741824; // the ratio could also be set for PWM
}

// computes 2 bl saw waves 180 out of phase, and adds them together
float32_t bl_square_process(bl_square * square){
	float32_t sig, saw_1, saw_2;
	float32_t transition_table_index;

	square->phase_1 += square->phase_step;
	sig = ((((float32_t) square->phase_1 / 2147483648.0f) + 1.f) * .5f) - .5f;
	// scale it,  numerator controls the band limit
	transition_table_index = sig * (5000.f / square->freq); // 4410 is nyquist * .4
	// clip it
	if (transition_table_index > .5f) transition_table_index = .5f;
	if (transition_table_index < -.5f) transition_table_index = -.5f;
	// range it for transistion table
	transition_table_index *= 1000.f;
	transition_table_index += 501.f;
	saw_1 = bl_step_table_read(transition_table_index) - sig;

	square->phase_2 += square->phase_step;
	sig = ((((float32_t) square->phase_2 / 2147483648.0f) + 1.f) * .5f) - .5f;
	// scale it
	transition_table_index = sig * (5000.f / square->freq); // 4410 is nyquist * .4
	// clip it,  numerator controls the band limit
	if (transition_table_index > .5f) transition_table_index = .5f;
	if (transition_table_index < -.5f) transition_table_index = -.5f;
	// range it for transistion table
	transition_table_index *= 1000.f;
	transition_table_index += 501.f;
	saw_2 = bl_step_table_read(transition_table_index) - sig;

	return saw_1 + saw_2;
}




/////////////////////




