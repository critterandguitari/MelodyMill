/*
===============================================================================
 Name        : main.c
 Author      : 
 Version     :
 Copyright   : Copyright (C) 
 Description : main definition
===============================================================================
*/



#ifdef __USE_CMSIS
#include "stm32f4xx.h"
#endif

/* Includes ------------------------------------------------------------------*/

#include "arm_math.h"
#include "CS4344.h"
#include "pp6.h"
#include "miditof.h"
#include "uart.h"
#include "timer.h"
#include "sequencer.h"
#include "audio.h"
#include "pwm.h"
#include "midi.h"
#include "notelist.h"

#define GATE_RESET_PERIOD 4

// from the DAC driver CS4344.c
extern unsigned int software_index;
extern unsigned int hardware_index;
extern short play_buf[];
extern uint32_t sample_clock;

// pocket piano object
extern pocket_piano pp6;

// from uart.c
// MIDI buffer
extern uint8_t  uart_recv_buf[32];
extern uint8_t  uart_recv_buf_write;
extern uint8_t  uart_recv_buf_read;

// for keeping track of time
uint32_t sample_clock_last = 0;

// led stuff
uint32_t led_counter = 0;  // for the flash function
uint8_t aux_led_color = BLACK;

uint8_t tmp8;

// params from knobs
float32_t rate, range, tune, glide, dur;
uint32_t rate_knob_10_bit_flipped = 0;

// not gen
uint32_t gate_time = 0;
uint32_t gate_reset = 0;
float32_t v, cents, cents_target;
float32_t glide_step;
uint8_t trig_time = 0;

// holds notes that get manipulated by arp modes
note_list nl;
note_list transformed;

// midi clock
uint8_t current_midi_clock = 0;

// for keeping track of midi output
uint8_t last_midi_note = 0;
uint8_t current_midi_note = 0;

// used for arp  modes
uint32_t period = 0;
uint32_t arp_count = 0;
uint32_t arp_tick = 0;
uint8_t current_note = 0;
int8_t oct = 0;
int8_t oct_delta;

// CV timing and detection
uint8_t cv_clock_state_history[2] = {0, 0};
uint8_t cv_clock_state_history_index = 0;
uint8_t cv_clock_state = 0;
uint8_t cv_clock_state_last = 0;
uint32_t cv_clock_period = CV_CLOCK_TIMEOUT + 1; // to make sure it starts off
uint32_t cv_clock_last_tick = 0;

// wether or not the instrument is in tuning mode
uint8_t tuning_mode = 0;

// general
uint32_t i;
uint16_t s;

// used to determine hold condition
uint32_t aux_button_depress_time = 0;

// used in the randomizer to prevent double play
uint32_t oct_last = 0;
uint32_t index_last = 0;

// for CA rule 130 of mode 6
uint8_t rows[2] = {0x08, 0};   // seed the CA with a 1 sorta in the middle
uint8_t row_index = 0;
uint8_t state = 0;


static void Delay(__IO uint32_t nCount);
static void flash_led_record_enable(void);
static void play_note(void);
static void adjust_f(void);
static void run_sequencer(void);
static void check_cv_clock(void);
static void determine_clock_source(void);

// ticks the arp by setting a flag.  uses the selected clock source
static void run_arp_ticks(void);

// just ticks at full speed (used for mode 0 single shot)
static void run_arp_ticks_full_speed(void);

// copy notes from keyboard and sequencer into the master note
// state array, performing a logical OR on note state so that
// the sequencer will never turn off a key being held down and vice versa
static void combine_keyboard_and_sequencer_notes(void);

static void check_for_hold_release(void);
static void update_note_list(void);
static void tune_up(void);
static uint8_t get_cell (uint8_t row, uint8_t num);

int main(void)
{


	rate = range = tune = glide = dur = 0;
	v = 0;


	// enable random number generator
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
	RNG_Cmd(ENABLE);

	// initialize low level stuff
	uart_init();
	timer_init();

	// initialize piano
	pp6_init();

	// setup codec
	CS4344_init();

	// timer test
	/*while (1){
		t = TIM_GetCounter(TIM2);
		TIM_SetCounter(TIM1,0);
		t_spent = t - t_last;
		t_last = t;
	}*/

	pwm_init();
	pwm_set(100);
	//	pwm_test();
	midi_init(1);

	seq_init();

	// zero out note lists
	note_list_init(&nl);
	note_list_init(&transformed);

	// get initial buttons
	// update keys has to be called > 8 times for key press to be debounced
	for (i = 0; i < 9; i++)
		pp6_keys_update();

	// see if we should be in tuning mode
	if (!((pp6_get_keys() >> 17) & 1))
		tuning_mode = 1;
	else
		tuning_mode = 0;

	// main loop
	while (1)	{

	    /* Update WWDG counter */
	    //WWDG_SetCounter(127);

		// check for new midi
	    // buffer midi reception
		// this happens in uart.c isr



        // empty the tx buffer
        uart_service_tx_buf();


		/*
		 * Control Rate, 64 sample periods
		 */
        // tuning mode
        if ( ((!(sample_clock & 0x3F)) && (sample_clock != sample_clock_last)) && tuning_mode ){
			// make sure this only happens once every 64 sample periods
			sample_clock_last = sample_clock;

			tune_up();
        }

        // MAIN CONTROL LOOP, 64 SAMPLE PERIODS
		if ( ((!(sample_clock & 0x3F)) && (sample_clock != sample_clock_last)) && !tuning_mode ){

			// make sure this only happens once every 64 sample periods
			sample_clock_last = sample_clock;

	        // process MIDI, pp6 will be updated from midi.c handlers if there are any relevant midi events
	        if (uart_recv_buf_read != uart_recv_buf_write){
	            tmp8 = uart_recv_buf[uart_recv_buf_read];
	            uart_recv_buf_read++;
	            uart_recv_buf_read &= 0x1f;
	            recvByte(tmp8);
	        }

	        // midi clock auto detection
	        pp6_check_for_midi_clock();

	        // update keys knobs
			pp6_keys_update();
			pp6_knobs_update();

			// update params
			rate = 1.075 - pp6_get_knob_5(); // don't want it to got all the way to 0, it might fuck up the midi output
			rate_knob_10_bit_flipped = 1024 - ((uint32_t) (pp6_get_knob_5() * 1024) );  // this is used for the midi clock division selection
			range = pp6_get_knob_4();
			tune = pp6_get_knob_1();
			glide = pp6_get_knob_2();
			dur = pp6_get_knob_3() + .1f; // don't let this be 0 cause we gotta send note messages (and be able to hear it!)

			// check for new key events
			pp6_get_key_events();

			// check for mode  change
			if (pp6_mode_button_pressed()){
				pp6_change_mode();
			}

			// maintain LED flasher
			pp6_flash_update();

			// read cv clock input, issue tick, and time its period
			check_cv_clock();

			// determine what the clock source will be for arps: cv, midi, or internal
			determine_clock_source();

			// now that we have checked for keyboard events (from internal keys and midi above), run  sequencer
			run_sequencer();

			// ticks for arps, in arp modes, ticks from cv, midi, or internal
			if (pp6_get_mode() != 0) {
				run_arp_ticks();
			}
			// in mode 0 (single shot)  just run clock at full speed
			else {
				run_arp_ticks_full_speed();
			}

			// copy keyboard notes to global notes
			combine_keyboard_and_sequencer_notes();

			// the sequencer looks at difference in keyboard note state
			// to determine events to log, so this needs to be called after run_seq
			pp6_set_current_keyboard_note_state_to_last();

			// check for a new key press that releases hold condition
			if (seq_get_status() == SEQ_HOLDING ){
				check_for_hold_release();
			}
			// check for events and update the note list, only if we are not holding
			if (seq_get_status() != SEQ_HOLDING ){
				update_note_list();
			}

			//Single shot
			if (pp6_get_mode() == 0){
				// single shot
				if (note_list_most_recent(&nl) != current_note){
					// only play if a key is down
					if (nl.len) {
						current_note = note_list_most_recent(&nl);
						cents_target = (float32_t)(current_note  + (oct * 12)) * 100;
						play_note();
						pp6_set_gate(1);
					}
				}
				if (!nl.len) {   // turn gate off if no notes are being held
					pp6_set_gate(0);
					pp6_set_all_midi_out_off();
					current_note = 0;
				}
			} // mode 0

			//?
			if (pp6_get_mode() == 1){
				note_list_copy_notes(&nl, &transformed);

				if (nl.len > 0) { // if notes are down

					if (arp_tick ) { // got an arp tick
						arp_tick = 0;
						if (transformed.index >= transformed.len){
							transformed.index=0;
							oct += 1;
							if (oct > ((int)(range * 6))){
								oct = 0;
							}
						}
						if (oct_delta == -1)
							cents_target = (float32_t)(transformed.note_list[(transformed.len - 1) - transformed.index]  + (oct * 12)) * 100;
						else
							cents_target = (float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100;

						play_note();
						transformed.index++;
					} // click
				}
				else {   // no notes down, reset arp
					if (arp_tick ) arp_tick = 0; // keep this unchecked
					transformed.index=0;
					oct = 0;
					oct_delta = 1;
				}
			} // mode 1
			// UP with REVERSE DOWN
			if (pp6_get_mode() == 2){
				note_list_copy_notes(&nl, &transformed);

				if (nl.len > 0) { // if notes are down

					if (arp_tick ) { // got an arp tick
						arp_tick = 0;
						if (transformed.index >= transformed.len){
							transformed.index=0;
							oct -= 1;
							if (oct < 0){
								oct = ((int)(range * 6));
							}

						}
						if (oct_delta == -1)
							cents_target = (float32_t)(transformed.note_list[(transformed.len - 1) - transformed.index]  + (oct * 12)) * 100;
						else
							cents_target = (float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100;

						play_note();
						transformed.index++;
					} // click
				}
				else {   // no notes down, reset arp
					if (arp_tick ) arp_tick = 0; // keep this unchecked
					transformed.index=0;
					oct = 0;
					oct_delta = 1;
				}
			} // mode 2

			if (pp6_get_mode() == 3){
				note_list_copy_notes(&nl, &transformed);

				if (nl.len > 0) { // if notes are down

					if (arp_tick ) { // got an arp tick
						arp_tick = 0;
						if (transformed.index >= transformed.len){
							transformed.index=0;
							oct += oct_delta;
							if (oct > 8) oct = 0;
							if (oct > ((int)(range * 6))){
								oct_delta = -1;
							}
							if (oct == 0){
								oct_delta = 1;
							}
						}
						if (oct_delta == -1)
							cents_target = (float32_t)(transformed.note_list[(transformed.len - 1) - transformed.index]  + (oct * 12)) * 100;
						else
							cents_target = (float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100;

						play_note();
						transformed.index++;
					} // click
				}
				else {   // no notes down, reset arp
					if (arp_tick ) arp_tick = 0; // keep this unchecked
					transformed.index=0;
					oct = 0;
					oct_delta = 1;
				}
			}

			if (pp6_get_mode() == 4){
				uint8_t tick_count;
				note_list_copy_notes(&nl, &transformed);

				if (nl.len > 0) { // if notes are down

					if (arp_tick ) { // got an arp tick
						arp_tick = 0;
						tick_count++;
						if (transformed.index >= transformed.len){
							transformed.index=0;
							oct += 1;
							if (oct > ((int)(range * 6))){
								oct = 0;
							}
						}

//						cents_target = (float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100;
						if (transformed.len == 1){
							if (tick_count & 1)
								cents_target = (float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100;
							else
								cents_target = (float32_t)(transformed.note_list[transformed.index]  ) * 100;
						}
						else {
							if (transformed.index & 1)
								cents_target = (float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100;
							else
								cents_target = (float32_t)(transformed.note_list[transformed.index]  ) * 100;
						}


						play_note();
						transformed.index++;
					} // click
				}
				else {   // no notes down, reset arp
					if (arp_tick ) arp_tick = 0; // keep this unchecked
					transformed.index=0;
					oct = 0;
					oct_delta = 1;
				}
			} // mode 4

			// random notes
			if (pp6_get_mode() == 5){
				note_list_copy_notes(&nl, &transformed);

				if (nl.len > 0) { // if notes are down

					if (arp_tick ) { // got an arp tick

						arp_tick = 0;

						// tick the CA
						/*row_index++;
						row_index &= 1;

						rows[row_index] = 0;  // zero out the row for new cells

						// go thru the 8 cells
						for(i=0; i<8; i++){
							state = 0;
							// calc state from prev row, (row_index + 1) & 1  since there are only 2
							state = (get_cell(rows[(row_index + 1) & 1], i + 1) << 2) | (get_cell(rows[(row_index + 1) & 1], i) << 1) | (get_cell(rows[(row_index + 1) & 1], i - 1) << 0);
							// put new cells in next row
							rows[row_index] |= (((30 >> state) & 1) << i);
						}*/


						i = (uint32_t)(range * 6);
						if (i)
							oct = RNG_GetRandomNumber() % (uint32_t)(range * 6);
						else
							oct = 0;

						// if more then 1 note held down, don't play same twince in a row
						if (transformed.len > 1){
							transformed.index = RNG_GetRandomNumber() % transformed.len;
							while (transformed.index == index_last){
								transformed.index = RNG_GetRandomNumber() % transformed.len;
							}
							index_last = transformed.index;
						}
						else
							transformed.index = RNG_GetRandomNumber() % transformed.len;

						cents_target = (float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100;

						play_note();

					} // click
				}
				else {   // no notes down, reset arp
					if (arp_tick ) arp_tick = 0; // keep this unchecked
				}
			} // mode 5

			// maintain the gate output, and midi note off output
			// gate goes low for 2 ms before going high (so we always have a note)
			// not used for single shot (mode 0)
			if (pp6_get_mode() != 0) {
				if (gate_reset){
					gate_reset--;
					pp6_set_gate(0);
					pp6_set_all_midi_out_off();
				}
				else {  // after gate has been low for a couple ms, bring it high for the specified dur, but only for arp modes

					if(gate_time) {
						gate_time--;
						pp6_set_gate(1);
						pp6_set_midi_out_note_on(current_midi_note); // remember that current_midi_note was calculated in the play_note function
					}
					else {
						pp6_set_gate(0);
						pp6_set_all_midi_out_off();
					}
				}
			}

			if (trig_time){
				trig_time--;
				pp6_set_trig(1);  // also set the trig
			}
			else {
				pp6_set_trig(0);  // set trig back to 0
			}

			// compare midi output note states to their previos, and send note messages if differnet
			for (i = 0; i < 128; i++){
				if (pp6_get_midi_out_note_state(i) != pp6_get_midi_out_note_state_last(i)){
					if (pp6_get_midi_out_note_state(i))
						sendNoteOn(1, i, 100);
					else
						sendNoteOff(1, i, 0);
				}
			}
			// update for next time thru
			pp6_set_current_midi_out_note_state_to_last();


			// clear all the events
			pp6_clear_flags();
			led_counter++;
			// ready for new note list
			note_list_set_current_to_last(&nl);
			note_list_set_current_to_last(&transformed);


		}



		/*
		 * Sample Rate
		 */
		if (software_index != hardware_index){
			if (!tuning_mode) adjust_f();

			if (software_index & 1){   // channel
				s = (uint16_t) (65535.f * (v / 10.f)) ;
				// put the r
				play_buf[software_index] = 0x0;   // make sure this is 0, see below
				software_index++;
				software_index &= 0xf;

			}
			else {   // channel 2, do the same thing
				// put the l
				// clock is out of phase (1 sample off) ,  since we are using I2S as SPI, but it works fine....
				play_buf[software_index] = (s >> 3) | (0x1 << 13);  // bits 0-11 are data, 12th bit is shutdown bit, needs to be 1
				software_index++;
				software_index &= 0xf;
	    	}
	    }
	}
}

uint8_t get_cell (uint8_t row, uint8_t num){
	num &= 0x7; // force to 8 cells
	return (row & (1 << num)) >> num;
}

void tune_up(void){
	if ((sample_clock >> 10) & 1){
		v = 10.f;
		pp6_set_seq_led(7);
		pp6_set_mode_led(7);
		pp6_set_clk_led(7);
	}
	else {
		v = 10;
		pp6_set_seq_led(0);
		pp6_set_mode_led(0);
		pp6_set_clk_led(0);
	}
}

void update_note_list(void){
	for (i = 0; i < 128; i++) {
		if (pp6_get_note_state(i) != pp6_get_note_state_last(i)) {
			if (pp6_get_note_state(i)) {
					note_list_note_on(&nl, i);
			}
			else {
					note_list_note_off(&nl, i);
			}
		}
	}
	pp6_set_current_note_state_to_last();
}


void check_for_hold_release(void){
	for (i = 0; i < 128; i++) {
		if (pp6_get_note_state(i) != pp6_get_note_state_last(i)) {
			if (pp6_get_note_state(i)) {
				seq_set_status(SEQ_STOPPED);
			}
		}
	}
}

void combine_keyboard_and_sequencer_notes(void){
	for (i=0; i<128; i++){

		if (pp6_get_keyboard_note_state(i) || seq_get_note_state(i))
			pp6_set_note_on(i);
		else
			pp6_set_note_off(i);
	}
}

// ticks the arp by setting a flag.  uses the selected clock source
void run_arp_ticks(void){

	if (pp6_get_clk_src() == CLK_SRC_INT) {
		arp_count++;
		period = rate * 200;
		if ((arp_count > period) ) {
			arp_tick = 1;
			arp_count = 0;
			if (pp6_get_clk_led() == BLACK) pp6_set_clk_led(BLUE);
			else pp6_set_clk_led(BLACK);
			//seq_tick();
		}
	}

	if (pp6_get_clk_src() == CLK_SRC_MIDI){

		if (rate_knob_10_bit_flipped < 256) {
			if ((!(pp6_get_midi_clock_count() % 3)) && (current_midi_clock != pp6_get_midi_clock_count())  ) {
				arp_tick = 1;
				current_midi_clock = pp6_get_midi_clock_count();
				if (pp6_get_clk_led() == BLACK) pp6_set_clk_led(GREEN);
				else pp6_set_clk_led(BLACK);
				//seq_tick();
			}
		}
		else if (rate_knob_10_bit_flipped < 512) {
			if ((!(pp6_get_midi_clock_count() % 6)) && (current_midi_clock != pp6_get_midi_clock_count())  ) {
				arp_tick = 1;
				current_midi_clock = pp6_get_midi_clock_count();
				if (pp6_get_clk_led() == BLACK) pp6_set_clk_led(GREEN);
				else pp6_set_clk_led(BLACK);
				//seq_tick();
			}
		}
		else if (rate_knob_10_bit_flipped < 768) {
			if ((!(pp6_get_midi_clock_count() % 8)) && (current_midi_clock != pp6_get_midi_clock_count())  ) {
				arp_tick = 1;
				current_midi_clock = pp6_get_midi_clock_count();
				if (pp6_get_clk_led() == BLACK) pp6_set_clk_led(GREEN);
				else pp6_set_clk_led(BLACK);
				//seq_tick();
			}
		}
		else if (rate_knob_10_bit_flipped <= 1024) {
		   if ((!(pp6_get_midi_clock_count() % 12)) && (current_midi_clock != pp6_get_midi_clock_count())  ) {
				current_midi_clock = pp6_get_midi_clock_count();
				arp_tick = 1;

				if (pp6_get_clk_led() == BLACK) pp6_set_clk_led(GREEN);
				else pp6_set_clk_led(BLACK);
				//seq_tick();
			}
		}
	}

	if (pp6_get_clk_src() == CLK_SRC_CV) {
		if (pp6_get_cv_clock_tick() ) {
			arp_tick = 1;
			arp_count = 0;
			if (pp6_get_clk_led() == BLACK) pp6_set_clk_led(MAGENTA);
			else pp6_set_clk_led(BLACK);
			//seq_tick();
		}
	}
}

// just ticks at full speed (used for mode 0 single shot)
void run_arp_ticks_full_speed(void){
	arp_count++;
	period = 1;  // just run it at full speed
	if ((arp_count > period) ) {
		arp_tick = 1;
		arp_count = 0;
		if (pp6_get_clk_led() == BLACK) pp6_set_clk_led(BLUE);
		else pp6_set_clk_led(BLACK);
		//seq_tick();
	}
}

void run_sequencer(void){

	// tick the main sequencer clock
	seq_tick();

	//
	if (seq_get_status() == SEQ_STOPPED){

		pp6_set_seq_led(BLACK);

		// aux button gets pressed and held
		if ( !pp6_get_keyboard_notes_on() ) {  // only if there are not notes held down, and a hold is not already set

			// record enable if seq button is pressed and held
			if ( (!(( pp6_get_keys() >> 17) & 1)) ) {
				aux_button_depress_time++;
				if (aux_button_depress_time > 500){
					aux_button_depress_time = 0;
					seq_set_status(SEQ_RECORD_ENABLE);
				}
			}

			// if pressed, and there is a sequence (positive length), play
			if (pp6_aux_button_pressed() || pp6_get_midi_start()) {
				if (seq_get_length()) {  // only play if positive length

					// TODO :: ?? can't have this in here  (at least have a reset_arps() function)
					//RESET ARPS
					transformed.index=0;
					oct = 0;
					oct_delta = 1;

					seq_set_status(SEQ_PLAYING);
					sendStart();  // send out a midi start
				}
				else seq_set_status(SEQ_STOPPED);
				seq_rewind();
				aux_button_depress_time = 0;
			}
		}
		else {  // check for hold condition
			if (pp6_aux_button_pressed()) {
				seq_set_status(SEQ_HOLDING);
			}
		}
	}
	else if (seq_get_status() == SEQ_HOLDING){
		pp6_set_seq_led(MAGENTA);
		if (pp6_aux_button_pressed()) {
			seq_set_status(SEQ_STOPPED);
		}
		// record enable
		if ( (!(( pp6_get_keys() >> 17) & 1)) ) {
			aux_button_depress_time++;
			if (aux_button_depress_time > 500){
				aux_button_depress_time = 0;
				seq_set_status(SEQ_RECORD_ENABLE);
			}
		}
	}
	else if (seq_get_status() == SEQ_RECORD_ENABLE){
		flash_led_record_enable();
		if (pp6_aux_button_pressed()) {
			seq_set_status(SEQ_STOPPED);
		}
		if (pp6_keyboard_note_on_flag()){
			seq_set_status(SEQ_RECORDING);
			seq_start_recording();
			seq_log_first_notes();
			seq_log_knobs(pp6_get_knob_array());
			sendStart();  // send out a midi start
		}
		if (pp6_get_midi_start()) {
			seq_set_status(SEQ_RECORDING);
			seq_start_recording();
			seq_log_first_note_null();   // sequence doesn't start with a note
			sendStart();  // send out a midi start
		}
	}
	else if (seq_get_status() == SEQ_RECORDING){

		pp6_set_seq_led(RED);
		seq_log_events();

		// stop recording
		if (pp6_aux_button_pressed() || seq_get_auto_stop()) {
			seq_stop_recording();
			seq_set_status(SEQ_PLAYING);
			seq_enable_knob_playback();
			aux_button_depress_time = 0;
			seq_clear_auto_stop();
			sendStop();  // send MIDI stop

			// TODO :: ?? can't have this in here  (atleast have a reset_arps() function)
			//RESET ARPS
			transformed.index=0;
			oct = 0;
			oct_delta = 1;
		}
		if (pp6_get_midi_stop()) {   // if a midi stop is received, stop recording, and dont play
			seq_stop_recording();
			seq_set_status(SEQ_STOPPED);
			aux_button_depress_time = 0;
			seq_clear_auto_stop();
			sendStop();  // send MIDI stop
		}

	}
	else if (seq_get_status() == SEQ_PLAYING) {

		seq_play_knobs();  	// play knobs
		seq_play_tick();  	// play notes
		aux_led_color = GREEN;

		pp6_set_seq_led(aux_led_color);

		// flash white on rollover
		if (seq_get_time() == 0) pp6_flash_seq_led(75);

		// aux button gets pressed and held
		if ( (!(( pp6_get_keys() >>17) & 1)) ) {
			aux_button_depress_time++;
			if (aux_button_depress_time > 500){
				aux_button_depress_time = 0;
				seq_set_status(SEQ_RECORD_ENABLE);
				seq_set_all_notes_off();
			}
		}
		// aux button swithes to stop
		if (pp6_aux_button_pressed() || pp6_get_midi_stop()) {
			seq_set_status(SEQ_STOPPED);
			aux_button_depress_time = 0;
			seq_set_all_notes_off();
			sendStop();  // send MIDI stop
		}
	}
}

void check_cv_clock(void){
	// check for CV clock
	cv_clock_state_history[cv_clock_state_history_index] = pp6_get_cv_clk();
	cv_clock_state_history_index++;
	cv_clock_state_history_index &= 1;

	// poor mans debounce
	if(cv_clock_state_history[0] == cv_clock_state_history[1])
		cv_clock_state = cv_clock_state_history[0];

	// click along
	if (cv_clock_state != cv_clock_state_last){
		cv_clock_state_last = cv_clock_state;
		cv_clock_last_tick = sample_clock;
		pp6_set_cv_clock_tick();
	}

	// get cv clock period
	cv_clock_period = sample_clock - cv_clock_last_tick;
}

void determine_clock_source(void){
	// determine clock source,
	// cv gets precedance
	if (cv_clock_period < CV_CLOCK_TIMEOUT)
		pp6_set_clk_src(CLK_SRC_CV);
	else {
			// then midi, then internal
			if (pp6_midi_clock_present()){
				pp6_set_clk_src(CLK_SRC_MIDI);
			}
			if (!pp6_midi_clock_present()){
				pp6_set_clk_src(CLK_SRC_INT);
			}
	}
}

void play_note(void){
	//cents = cents_target;
	gate_time = (int)(dur * 200);
	gate_reset = GATE_RESET_PERIOD;  // 4 control periods of reset

	trig_time = 5;

	glide_step = ABS(cents - cents_target) / (glide * 10000);   // determine slope for fix time glide

	current_midi_note = (uint8_t)(cents_target / 100);

	// if we are in mode 0, shut the previous note off, turn new one on
	// other modes get midi out when the gate is set
	if (pp6_get_mode() == 0) {
		// first turn off any midi notes playing
		pp6_set_all_midi_out_off();
		pp6_set_midi_out_note_on(current_midi_note);
	}
}

void adjust_f(void){

	if (ABS(cents - cents_target) <= glide_step)  // if within 1 glide step
		cents = cents_target;

	else if (cents < cents_target)
		cents += glide_step;
	else if (cents > cents_target)
		cents -= glide_step;

	// if the octave button is pressed
	if (GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3)){
		v = (cents + (tune * 2400.f)) / 1200.f;
		if(pp6_get_gate())   // turn off pwm if note is over
			pwm_set( c_to_f_ratio((cents + (tune * 2400.f))) * 8 * 1.020408163f );
		else
			pwm_set(0);
	}
	else {
		v = ((cents + 1200) + (tune * 2400.f)) / 1200.f;
		if(pp6_get_gate())   // turn off pwm if note is over
			pwm_set( c_to_f_ratio(((cents + 1200) + (tune * 2400.f))) * 8 * 1.020408163f  );
		else
			pwm_set(0);
	}
}

void flash_led_record_enable() {
	if (led_counter > 150){
		led_counter = 0;
		if (pp6_get_seq_led()){
			pp6_set_seq_led(0);
		}
		else {
			pp6_set_seq_led(1);
		}
	}
}

/*
filter_man_control_process(){


}*/



/**
  * @brief  Delay Function.
  * @param  nCount:specifies the Delay time length.
  * @retval None
  */
void Delay(__IO uint32_t nCount)
{
  while(nCount--)
  {
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
