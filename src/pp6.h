/*
 * pp6.h
 *
 *  Created on: Jun 23, 2012
 *      Author: owen
 */

#ifndef PP6_H_
#define PP6_H_

#include "arm_math.h"

#define SEQ_LED_BLUE_ON GPIO_WriteBit(GPIOB, GPIO_Pin_9, 0)
#define SEQ_LED_RED_ON GPIO_WriteBit(GPIOE, GPIO_Pin_0, 0)
#define SEQ_LED_GREEN_ON GPIO_WriteBit(GPIOE, GPIO_Pin_1, 0)
#define SEQ_LED_BLUE_OFF GPIO_WriteBit(GPIOB, GPIO_Pin_9, 1)
#define SEQ_LED_RED_OFF GPIO_WriteBit(GPIOE, GPIO_Pin_0, 1)
#define SEQ_LED_GREEN_OFF GPIO_WriteBit(GPIOE, GPIO_Pin_1, 1)

#define MODE_LED_BLUE_ON GPIO_WriteBit(GPIOB, GPIO_Pin_4, 0)
#define MODE_LED_RED_ON GPIO_WriteBit(GPIOB, GPIO_Pin_5, 0)
#define MODE_LED_GREEN_ON GPIO_WriteBit(GPIOB, GPIO_Pin_8, 0)
#define MODE_LED_BLUE_OFF GPIO_WriteBit(GPIOB, GPIO_Pin_4, 1)
#define MODE_LED_RED_OFF GPIO_WriteBit(GPIOB, GPIO_Pin_5, 1)
#define MODE_LED_GREEN_OFF GPIO_WriteBit(GPIOB, GPIO_Pin_8, 1)

#define CLK_LED_BLUE_ON GPIO_WriteBit(GPIOA, GPIO_Pin_7, 0)
#define CLK_LED_RED_ON GPIO_WriteBit(GPIOC, GPIO_Pin_4, 0)
#define CLK_LED_GREEN_ON GPIO_WriteBit(GPIOC, GPIO_Pin_5, 0)
#define CLK_LED_BLUE_OFF GPIO_WriteBit(GPIOA, GPIO_Pin_7, 1)
#define CLK_LED_RED_OFF GPIO_WriteBit(GPIOC, GPIO_Pin_4, 1)
#define CLK_LED_GREEN_OFF GPIO_WriteBit(GPIOC, GPIO_Pin_5, 1)

#define CLK_SRC_INT 0
#define CLK_SRC_CV 1
#define CLK_SRC_MIDI 2

#define BLACK 0
#define RED 1
#define YELLOW 2
#define GREEN 3
#define CYAN 4
#define BLUE 5
#define MAGENTA 6

#define MIDI_CLOCK_TIMEOUT 20000  // around 1 second of not receiving midi clock
#define CV_CLOCK_TIMEOUT 100000  // a few seconds


typedef struct {

	// knobs
	uint8_t knob_touched[5]; 	// flag to see if knob is touched
	float32_t knob[5];   		// stores knob values 0-1

	// secret mode
	uint8_t secret_mode_enabled;

	// the actual notes the synth is playing
	uint8_t synth_note_start;   		// flag to start synth
	uint8_t synth_note_stop;   		// flag to stop synth
	uint8_t synth_playing;
	uint8_t synth_note;


	// both the built in keyboard and midi input generate these events
	uint8_t note_on_flag;		// note on flag (MIDI or keyboard)
	uint8_t note_off_flag; 		// note off flag
	uint8_t note_on;			// used in conjunction with the on and off flags to store recieved event (from keyboard or MIDI)
	uint8_t note_off;
	uint8_t physical_notes_on;    // the number of non sequenced notes currently on
	uint8_t note_state[128];	  // state of all the midi notes -- 0 for off, anything else for on
	uint8_t note_state_last[128]; // the previos time thru the main loop that notes were updated.  compared with above to get note events

	// keys
	uint32_t keys;
	uint32_t keys_last;
	uint8_t num_keys_down;

	// gate state
	uint8_t gate_state;

	// current mode
	uint32_t mode;

	// mode and aux button event flags
	uint8_t mode_button_pressed;	// mode button event flags
	uint8_t mode_button_released;
	uint8_t aux_button_pressed;		// aux button event flags
	uint8_t aux_button_released;

	// LEDs
	uint8_t seq_led;
	uint8_t mode_led;
	uint8_t clk_led;
	uint8_t mode_led_flash; 			// these count down to zero, then the flash is over
	uint8_t seq_led_flash;

	// MIDI clock
	uint8_t midi_start_flag;        // midi start command
	uint8_t midi_stop_flag;    		// midi stop command
	uint32_t midi_in_clock_last;   // stores the system time of the last received midi clock
	uint8_t midi_clock_present;  // if a midi clock is currently present
	uint32_t midi_clock_period;  // time in between midi clock ticks
	uint8_t midi_clock_tick_count;
	uint8_t midi_clock_flag;

	// cv clock
	uint8_t cv_clock_tick;

	// Global clock
	uint8_t clock_source;

} pocket_piano;

void pp6_init(void);

void pp6_set_trig(uint32_t stat);
void pp6_set_gate(uint32_t stat);

uint8_t pp6_get_gate(void);
uint8_t pp6_get_cv_clk(void);

void pp6_enable_secret_mode(void);

float32_t pp6_get_knob_1(void);
float32_t pp6_get_knob_2(void);
float32_t pp6_get_knob_3(void);
float32_t pp6_get_knob_4(void);
float32_t pp6_get_knob_5(void);
void pp6_set_knob_1(float32_t v);
void pp6_set_knob_2(float32_t v);
void pp6_set_knob_3(float32_t v);
float32_t * pp6_get_knob_array(void);
void pp6_set_knob_array(float32_t * knobs);
void pp6_check_knobs_touched (void);
uint8_t pp6_any_knobs_touched(void);
uint8_t pp6_knob_1_touched(void);
uint8_t pp6_knob_2_touched(void);
uint8_t pp6_knob_3_touched(void);

uint8_t pp6_get_synth_note_start(void);
void pp6_set_synth_note_start (void );
uint8_t pp6_get_synth_note_stop(void);
void pp6_set_synth_note_stop (void );
uint8_t pp6_get_synth_note(void);
void pp6_set_synth_note(uint8_t note);


void pp6_clear_flags(void);

uint8_t pp6_is_playing (void);

uint32_t pp6_get_keys(void);   // returns the current key status
uint8_t pp6_get_num_keys_down(void);
void pp6_get_key_events(void);  // checks keys for new events


void pp6_change_mode(void);
uint32_t pp6_get_mode(void);
void pp6_set_mode(uint32_t mode);

void pp6_init_digi_in(void);
void pp6_keys_update(void);   // scans and debounces keys
void pp6_init_digi_out(void);
void pp6_leds_update(uint8_t bank_led, uint8_t mode_led);
void pp6_knobs_init(void);
void pp6_knobs_update(void);
void pp6_smooth_knobs(void);



uint8_t pp6_get_seq_led(void);
uint8_t pp6_get_mode_led(void);
void pp6_set_seq_led(uint8_t led);
void pp6_set_mode_led(uint8_t bank_led);
void pp6_set_clk_led(uint8_t led) ;
uint8_t pp6_get_clk_led(void);

void pp6_flash_mode_led(uint8_t flash_time);
void pp6_flash_seq_led(uint8_t flash_time);
void pp6_flash_update(void);

void  pp6_set_mode_button_pressed(void);
void pp6_set_mode_button_released(void);
uint8_t pp6_mode_button_pressed(void);
uint8_t pp6_mode_button_released(void);

void  pp6_set_aux_button_pressed(void);
void pp6_set_aux_button_released(void);
uint8_t pp6_aux_button_pressed(void);
uint8_t pp6_aux_button_released(void);

void pp6_inc_physical_notes_on(void);
void pp6_dec_physical_notes_on(void);
uint8_t pp6_get_physical_notes_on(void);

// midi start stop events
void pp6_set_midi_start(void);
uint8_t pp6_get_midi_start(void);
void pp6_set_midi_stop(void);
uint8_t pp6_get_midi_stop(void);

/// MIDI clock stuff
void pp6_midi_clock_tick(void);
void pp6_check_for_midi_clock(void) ;
uint8_t pp6_midi_clock_present(void);
uint32_t pp6_get_midi_clock_period(void);
uint8_t pp6_get_midi_clock_tick(void);
void pp6_clear_midi_clock_tick(void);
uint8_t pp6_get_midi_clock_count(void);

// the note interface for the piano
uint8_t pp6_note_on_flag();
uint8_t pp6_note_off_flag();
void pp6_set_note_off(uint8_t note);
void pp6_set_note_on(uint8_t note);
uint8_t pp6_get_note_state(uint8_t note);
uint8_t pp6_get_note_state_last(uint8_t);
void pp6_set_current_note_state_to_last(void);
void pp6_turn_off_all_on_notes(void);


// Clock stuff
uint8_t pp6_get_clk_src(void);
void pp6_set_clk_src(uint8_t);

void pp6_set_cv_clock_tick(void);
uint8_t pp6_get_cv_clock_tick(void);


#endif /* PP6_H_ */
