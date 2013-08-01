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

// from the DAC driver CS4344.c
extern unsigned int software_index;
extern unsigned int hardware_index;
extern short play_buf[];
extern uint32_t sample_clock;

// for keeping track of time
static uint32_t sample_clock_last = 0;

static void Delay(__IO uint32_t nCount);
static void flash_led_record_enable(void);

// led stuff
static uint32_t led_counter = 0;  // for the above flash function
static uint8_t aux_led_color = BLACK;

// pocket piano object  TODO:  the whole point is  this should not be here as extern ?  use getters setters
extern pocket_piano pp6;

// MIDI buffer
uint8_t  uart_recv_buf[32];
uint8_t  uart_recv_buf_write = 0;
uint8_t  uart_recv_buf_read = 0;
uint8_t tmp8;



int main(void)
{

	uint8_t i;

	uint32_t period = 0;
	uint32_t arp_count = 0;
	uint32_t arp_tick = 0;
	uint8_t current_note = 0;


	uint8_t oct = 0;
	int8_t oct_delta;

	note_list nl;

	note_list transformed;

	float32_t v, f, cents;

	uint16_t s;

	float32_t rate, range, tune, glide, dur;

	uint32_t gate_time = 0;
	uint32_t gate_reset = 0;

	rate = range = tune = glide = dur = 0;

	uint32_t aux_button_depress_time = 0;

	v = 0;

	Delay(20000);
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

	note_list_init(&nl);
	note_list_init(&transformed);

	transformed.len = 1;
	transformed.note_list[0] = 60;


	//pp6_knobs_init();
	// go!
	while (1)	{

	    /* Update WWDG counter */
	    //WWDG_SetCounter(127);

		// check for new midi
	    // buffer midi reception
	    if (!(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)){
	    	uart_recv_buf[uart_recv_buf_write] = USART_ReceiveData(USART1);

	    	// if its a sync, send it thru immediately  to avoid jitter
	    	if (uart_recv_buf[uart_recv_buf_write] == STATUS_SYNC) {
		    	sendSync();
	    	}

	        uart_recv_buf_write++;
	        uart_recv_buf_write &= 0x1f;  // 32 bytes
	    }

        // empty the tx buffer
        uart_service_tx_buf();

		/*
		 * Control Rate, 64 sample periods
		 */
		if ((!(sample_clock & 0x3F)) && (sample_clock != sample_clock_last)){

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
			rate = pp6_get_knob_5();
			range = pp6_get_knob_4();
			tune = pp6_get_knob_1();
			glide = pp6_get_knob_2();
			dur = pp6_get_knob_3();

			// check for new key events
			pp6_get_key_events();

			if (pp6_mode_button_pressed()){
				pp6_change_mode();
			}

			// maintain LED flasher
			pp6_flash_update();

			// determine clock source
			if (pp6_midi_clock_present()){
				pp6_set_clk_src(CLK_SRC_MIDI);
				pp6_set_clk_led(GREEN);

			}
			if (!pp6_midi_clock_present()){
				pp6_set_clk_src(CLK_SRC_INT);
				pp6_set_clk_led(BLUE);
			}

			// SEQUENCER GOES HERE
			//			// BEGIN SEQUENCER
			// sequencer states
			// TODO add HOLDING state
			if (seq_get_status() == SEQ_STOPPED){

				pp6_set_seq_led(BLACK);

				// aux button gets pressed and held
				if ( !pp6_get_physical_notes_on() ) {  // only if there are not notes held down, and a hold is not already set

					if ( (!(( pp6_get_keys() >> 17) & 1)) ) {
						aux_button_depress_time++;
						if (aux_button_depress_time > 500){
							aux_button_depress_time = 0;
							seq_set_status(SEQ_RECORD_ENABLE);
						}
					}

					if (pp6_aux_button_pressed() || pp6_get_midi_start()) {
						if (seq_get_length()) {  // only play if positive length

							// TODO :: ?? can't have this in here  (atleast have a reset_arps() function)
							//RESET ARPS
							transformed.index=0;
							oct = 0;
							oct_delta = 1;

							seq_enable_knob_playback();
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
			}
			else if (seq_get_status() == SEQ_RECORD_ENABLE){
				flash_led_record_enable();
				if (pp6_aux_button_pressed()) {
					seq_set_status(SEQ_STOPPED);
				}
				if (pp6_note_on_flag()){
					seq_set_status(SEQ_RECORDING);
					seq_start_recording();
					seq_log_events();
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
						pp6_set_synth_note_stop();  // stop the synth
						pp6_turn_off_all_on_notes();
					}
				}
				// aux button swithes to stop
				if (pp6_aux_button_pressed() || pp6_get_midi_stop()) {
					seq_set_status(SEQ_STOPPED);
					aux_button_depress_time = 0;
					pp6_set_synth_note_stop();   // stop the synth
					pp6_turn_off_all_on_notes();
					sendStop();  // send MIDI stop
				}
			}


	        // tick the sequencer with midi clock if it is present, otherwise use internal
			arp_count++;
			period = rate * 200;
			if ((arp_count > period) ) {
				arp_tick = 1;
				arp_count = 0;
				seq_tick();
			}


			// END SEQUENCER


			// check for a new key press that releases hold condition
			if (seq_get_status() == SEQ_HOLDING ){
				for (i = 0; i < 128; i++) {
					if (pp6_get_note_state(i) != pp6_get_note_state_last(i)) {
						if (pp6_get_note_state(i)) {
							seq_set_status(SEQ_STOPPED);
						}
					}
				}
			}
			// check for events and update the note list, only if we are not holding
			if (seq_get_status() != SEQ_HOLDING ){
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




			// UP with REVERSE DOWN
			if (pp6_get_mode() == 0){
				note_list_copy_notes(&nl, &transformed);

				if (nl.len > 0) { // if notes are down

					if (arp_tick ) { // got an arp tick
						arp_tick = 0;
						if (transformed.index >= transformed.len){
							transformed.index=0;
							oct += oct_delta;
							if (oct > 8) oct = 0;
							if (oct > ((int)(range * 8))){
								oct_delta = -1;
							}
							if (oct == 0){
								oct_delta = 1;
							}
						}


						if (oct_delta == -1)
							cents = (float32_t)(transformed.note_list[(transformed.len - 1) - transformed.index]  + (oct * 12)) * 100;

						else
							cents = (float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100;

						v = cents / 1200.f;
						pwm_set( (c_to_f_ratio(cents) * 10 ) * (tune * 2 + 1));
						gate_time = (int)(dur * 200);
						gate_reset = 4;  // 4 control periods of reset

						transformed.index++;
					} // click
				}
				else {   // no notes down, reset arp
					if (arp_tick ) arp_tick = 0; // keep this unchecked
					transformed.index=0;
					oct = 0;
					oct_delta = 1;
				}
			} // mode 0

			// gate goes low for 2 ms before going high (so we always have a note)
			if (gate_reset){
				gate_reset--;
				pp6_set_gate(0);
			}
			else {  // after gate has been low for a couple ms, bring it high for the specified dur
				if(gate_time) {
					gate_time--;
					pp6_set_gate(1);
				}
				else {
					pp6_set_gate(0);
				}
			}



			// single shot
			if (note_list_most_recent(&nl) != current_note){
				current_note = note_list_most_recent(&nl);
			//	pwm_set( (c_to_f_ratio((float32_t)current_note * 100) * 50 ) * (tune * 2 + 1));
			}

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

			if (software_index & 1){   // channel


					/*sig += .01f;
					if (sig > 1)
						sig = 0;

				arm_float_to_q15(&sig, &out, 1);*/


				s = (uint16_t) (65536.f * (v / 10.f)) ;
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
