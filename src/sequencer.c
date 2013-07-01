/*
 * sequencer.c
 *
 *  Created on: Aug 4, 2012
 *      Author: owen
 */
#include "arm_math.h"
#include "pp6.h"
#include "sequencer.h"

static uint32_t seq_deltas[256];
static uint32_t seq_notes[256];
static uint32_t seq_events[256];
static uint32_t seq_index = 0;
static uint32_t seq_time = 0;
static uint32_t seq_length = 0;
static uint8_t seq_status = SEQ_STOPPED;
static uint8_t seq_playback_knobs_enabled = 1;
static uint8_t seq_knob_1_playback_enabled = 1;
static uint8_t seq_knob_2_playback_enabled = 1;
static uint8_t seq_knob_3_playback_enabled = 1;

static uint8_t seq_auto_stop = 0;

static float32_t knob_log [4096][3];   // 4096 logs of the pot at SR / 32 / 16


uint8_t seq_ready_for_recording(void){
	if ((seq_status == SEQ_STOPPED) || (seq_status == SEQ_PLAYING))
		return 1;
	else
		return 0;
}

void seq_set_status(uint8_t stat) {
	seq_status = stat;
}

uint8_t seq_get_status(void) {
	return seq_status;
}

void seq_start_recording(void) {
	seq_time = 0;
	seq_index = 0;
}

void seq_log_first_note_null(void){  // this is when a sequence starts externally (say from midi)
	seq_deltas[0] = 0;
	seq_notes[0] = 0;
	seq_events[0] = SEQ_NOTE_START_NULL;
	seq_index++;
	seq_log_knobs(pp6_get_knob_array());   // also start the knob logger TODO : is this tested ??
}

void seq_log_note_start(uint8_t note){
	seq_deltas[seq_index] = seq_time;
	seq_notes[seq_index] = note;
	seq_events[seq_index] = SEQ_NOTE_START;
	seq_index++;
	if (seq_index == 0xFF) seq_set_auto_stop();
}

void seq_log_note_stop(uint8_t note){
	seq_deltas[seq_index] = seq_time;
	seq_notes[seq_index] = note;
	seq_events[seq_index] = SEQ_NOTE_STOP ;
	seq_index++;
	if (seq_index == 0xFF) seq_set_auto_stop();
}

// checks for current events and logs them
void seq_log_events(void) {
	uint8_t i;
	for (i = 0; i < 128; i++) {
		if (pp6_get_note_state(i) != pp6_get_note_state_last(i)) {
			if (pp6_get_note_state(i)) {
				seq_log_note_start(i);
			}
			else {
				seq_log_note_stop(i);
			}
		}
	}
	seq_log_knobs(pp6_get_knob_array());
}

void seq_stop_recording(void) {

	// if a midi clock is present, quantize length to nearest quater note
	if (pp6_midi_clock_present() ) {

		// if over the halfway point to next quater note (12), round up, else round down
		if ( (seq_time - (seq_time / 24) * 24) > 12)
			seq_deltas[seq_index] = ((seq_time / 24) + 1) * 24;
		else
			seq_deltas[seq_index] = (seq_time / 24) * 24;

	}
	else {
		seq_deltas[seq_index] = seq_time;
	}

	seq_notes[seq_index] = seq_notes[0];
	seq_events[seq_index] = SEQ_SEQ_STOP;

	seq_length = seq_index;

	seq_index = 0; // go back to the begining
	seq_time = 0;
	seq_playback_knobs_enabled = 1;  // enable playback of knobs
	pp6_turn_off_all_on_notes();
}

void seq_rewind(void) {
	seq_index = 0; // go back to the begining
	seq_time = 0;
}

// TODO:  this should be set all events for this delta time on this tick  -- don't think this really matters
void seq_play_tick (void){

	if (seq_time >= seq_deltas[seq_index]){
		if (seq_events[seq_index] == SEQ_NOTE_START){
			// a physical note down will mute sequence
			if (!pp6_get_physical_notes_on()){
				pp6_set_note_on(seq_notes[seq_index]);
			}
		}
		if (seq_events[seq_index] == SEQ_NOTE_STOP){
			// stop it, but only if its sounding (its note on might have been muted)
				if (pp6_get_note_state(seq_notes[seq_index]))
					pp6_set_note_off(seq_notes[seq_index]);
		}
		seq_index++;
		if (seq_index > seq_length){
			seq_index = 0;
			seq_time = 0;
			pp6_turn_off_all_on_notes();// end all notes still playing at end of loop
		}
	}
}

void seq_tick(void){
	seq_time++;
}

uint32_t seq_get_time(void){
	return seq_time;
}

uint32_t seq_get_length(void) {
	return seq_length;
}


void seq_log_knobs(float32_t * knob){
	uint32_t knob_log_time;

	// if midi clock is present, the sequencer will be using that, so we don't reduce resolution
	if (pp6_midi_clock_present())
		knob_log_time = seq_time & 0xFFF;
	else
		knob_log_time = (seq_time >> 4) & 0xFFF;  // otherwise reduce time resolution since we don't have that much memory

	knob_log[knob_log_time][0] = knob[0];
	knob_log[knob_log_time][1] = knob[1];
	knob_log[knob_log_time][2] = knob[2];

	if (knob_log_time == 0xFFF) seq_set_auto_stop();    // auto stop after 4096 logs
}

void seq_play_knobs(void) {

	uint32_t knob_log_time;

	// if midi clock is present, the sequencer will be using that, so we don't reduce resolution
	if (pp6_midi_clock_present())
		knob_log_time = seq_time & 0xFFF;
	else
		knob_log_time = (seq_time >> 4) & 0xFFF;  // otherwise reduce time resolution since we don't have that much memory


	// check and set knob playback for each knob
	if (pp6_knob_1_touched()) {
		seq_knob_1_playback_enabled = 0;
	}
	if (pp6_knob_2_touched()) {
		seq_knob_2_playback_enabled = 0;
	}
	if (pp6_knob_3_touched()) {
		seq_knob_3_playback_enabled = 0;
	}

	if (seq_knob_1_playback_enabled) {
		pp6_set_knob_1(knob_log[knob_log_time][0]);
	}
	if (seq_knob_2_playback_enabled) {
		pp6_set_knob_2(knob_log[knob_log_time][1]);
	}
	if (seq_knob_3_playback_enabled) {
		pp6_set_knob_3(knob_log[knob_log_time][2]);
	}

	//return &knob_log[knob_log_time][0];
}

uint8_t seq_knob_playback_enabled(void){
	return seq_playback_knobs_enabled;
}

void seq_disable_knob_playback(void){
	seq_playback_knobs_enabled = 0;
}

void seq_enable_knob_playback(void){
	seq_playback_knobs_enabled = 1;
	seq_knob_1_playback_enabled = 1;
	seq_knob_2_playback_enabled = 1;
	seq_knob_3_playback_enabled = 1;
}

void seq_set_auto_stop(void){
	seq_auto_stop = 1;
}

uint8_t seq_get_auto_stop(void){
	return seq_auto_stop;
}

void seq_clear_auto_stop(void) {
	seq_auto_stop = 0;
}

