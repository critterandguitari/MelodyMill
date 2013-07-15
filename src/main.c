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
	uint32_t count = 0;
	uint8_t current_note = 0;

	uint8_t num_down;

	uint8_t my_octave = 0;

	uint8_t oct = 0;
	int8_t oct_delta;

	note_list nl;

	note_list transformed;

	float32_t v;

	uint16_t s;

	float32_t rate, range, tune, glide, dur;

	rate = range = tune = glide = dur = 0;


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

	        // update keys knobs
			pp6_keys_update();
			pp6_knobs_update();

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

			// trig, gate, clkin test
			/*if (pp6_get_num_keys_down()) {
				pp6_set_trig(1);
			}
			else {
				pp6_set_trig(0);
			}

			if (pp6_get_clkin())
				pp6_set_gate(1);
			else
				pp6_set_gate(0);*/


			// maintain LED flasher
			pp6_flash_update();


		// SEQUENCER GOES HERE
		//


			// check for events, and send midi and play synth
			// TODO :  don't sent midi from here, send it from the arps
			for (i = 0; i < 128; i++) {
				if (pp6_get_note_state(i) != pp6_get_note_state_last(i)) {
					if (pp6_get_note_state(i)) {
						sendNoteOn(1, i, 100);
						note_list_note_on(&nl, i);
						pp6_set_synth_note_start();
						pp6_set_synth_note(i);
					}
					else {
						sendNoteOff(1, i, 0);
						note_list_note_off(&nl, i);
						if (i ==  pp6_get_synth_note()) pp6_set_synth_note_stop();  // if it equals the currently playing note, shut it off
					}
				}
			}
			pp6_set_current_note_state_to_last();

			// simple up arp
			/*count++;
			period = rate * 1000;
			if (count > period) {
				count = 0;
				nl.index++;
				if (nl.index >= nl.len){
					nl.index=0;
					oct++;
					if (oct > (int)(range * 8)){
						oct = 0;
					}
				}

				pwm_set( (c_to_f_ratio((float32_t)(nl.note_list[nl.index]  + (oct * 12)) * 100) * 10 ) * (tune * 2 + 1));

				pp6_set_mode_led(nl.index & 0x7);
			}*/

			// up down
			/*count++;
			period = rate * 1000;
			note_list_copy_notes(&nl, &transformed);
			if (count > period) {
				count = 0;
				transformed.index++;
				if (transformed.index >= transformed.len){
					transformed.index=0;
					oct += oct_delta;
					if (oct > 8) oct = 0;
					if (oct > (int)(range * 8)){
						oct_delta = -1;
					}
					if (oct == 0){
						oct_delta = 1;
					}

				}
				pwm_set( (c_to_f_ratio((float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100) * 10 ) * (tune * 2 + 1));
				pp6_set_mode_led(transformed.index & 0x7);
			}*/


				/*	if (pp6_get_mode() == 1)  mode_wave_adder_control_process ();
					if (pp6_get_mode() == 2)  mode_analog_style_control_process();
					if (pp6_get_mode() == 3)  mode_filter_envelope_control_process();
					if (pp6_get_mode() == 4)  mode_simple_fm_control_process();
					if (pp6_get_mode() == 5)  mode_bass_delay_control_process();
					if (pp6_get_mode() == 6)  mode_plurden_control_process();   // SECRET MODE ??*/


			// UP with REVERSE DOWN
			if (pp6_get_mode() == 0){
				count++;
				period = rate * 200;
				note_list_copy_notes(&nl, &transformed);
				if (count > period) {
					count = 0;
					transformed.index++;
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
						pwm_set( (c_to_f_ratio((float32_t)(transformed.note_list[(transformed.len - 1) - transformed.index]  + (oct * 12)) * 100) * 10 ) * (tune * 2 + 1));
					else
						pwm_set( (c_to_f_ratio((float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100) * 10 ) * (tune * 2 + 1));
					pp6_set_clk_led(transformed.index & 0x7);
				}
			}
			// down
			// arp updated between notes
			/*	count++;
				period = rate * 1000;
				note_list_copy_notes(&nl, &transformed);
				if (count > period) {
					count = 0;
					transformed.index++;
					if (transformed.index >= transformed.len){
						transformed.index=0;
						oct -= 1;
						if (oct > 8) oct = 0;
						//if (oct > (int)(range * 8)){
						//	oct_delta = -1;
						//}
						if (oct == 0){
							oct = (int)(range * 8);
						}

					}
					pwm_set( (c_to_f_ratio((float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100) * 10 ) * (tune * 2 + 1));
					pp6_set_mode_led(transformed.index & 0x7);
				}*/


				// down
				//arp updated between octaves
			if (pp6_get_mode() == 1) {
				count++;
				period = rate * 200;
				if (nl.len > num_down){
					note_list_copy_notes(&nl, &transformed);
					note_list_octaves_down(&transformed, (int)(range * 8));
				}
				num_down = nl.len;


				if (count > period) {
					count = 0;

					if (!(transformed.index % nl.len)){   // every roll over of notes held down
						note_list_copy_notes(&nl, &transformed);
						note_list_octaves_down(&transformed, (int)(range * 8));
					}
					transformed.index++;
					if (transformed.index >= transformed.len){
						transformed.index=0;

					}
					pwm_set( (c_to_f_ratio((float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100) * 10 ) * (tune * 2 + 1));
					pp6_set_clk_led(transformed.index & 0x7);
				}
			}


			// up
			//arp updated between octaves
		if (pp6_get_mode() == 2) {
			count++;
			period = rate * 200;
			if (nl.len > num_down){
				note_list_copy_notes(&nl, &transformed);
				note_list_octaves_down(&transformed, (int)(range * 8));
			}
			num_down = nl.len;


			if (count > period) {
				count = 0;

				if (!(transformed.index % nl.len)){   // every roll over of notes held down
					note_list_copy_notes(&nl, &transformed);
					note_list_octaves_up(&transformed, (int)(range * 8));
				}
				transformed.index++;
				if (transformed.index >= transformed.len){
					transformed.index=0;

				}
				pwm_set( (c_to_f_ratio((float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100) * 10 ) * (tune * 2 + 1));
				pp6_set_clk_led(transformed.index & 0x7);
			}
		}

		// random
		//arp updated between octaves
	if (pp6_get_mode() == 3) {


		count++;
		period = rate * 200;
		if (nl.len > num_down){
			note_list_copy_notes(&nl, &transformed);
			note_list_octaves_down(&transformed, (int)(range * 8));
		}
		num_down = nl.len;


		if (count > period) {
			count = 0;

			if (!(transformed.index % nl.len)){   // every roll over of notes held down
				note_list_copy_notes(&nl, &transformed);
				note_list_octaves_up(&transformed, (int)(range * 8));
			}
			transformed.index++;
			if (transformed.index >= transformed.len){
				transformed.index=0;

			}
			pwm_set( (c_to_f_ratio((float32_t)(transformed.note_list[(RNG_GetRandomNumber() & 0xff) % transformed.len]  + (oct * 12)) * 100) * 10 ) * (tune * 2 + 1));
			pp6_set_clk_led(transformed.index & 0x7);
		}
	}

			// UP DOWN MIRROR  using note_list
			//arp updated between octaves
		/*	count++;
			period = rate * 1000;

			note_list up;
			note_list down;
			note_list_init(&up);
			note_list_init(&down);

			if (count > period) {
				count = 0;

				if (!(transformed.index % nl.len)){   // every roll over of notes held down
					note_list_copy_notes(&nl, &up);
					note_list_copy_notes(&nl, &down);

					note_list_octaves_up(&up, (int)(range * 8));

					note_list_mirror(&down);
					note_list_octaves_down(&down, ((int)(range * 8) - 2));
					note_list_transpose(&down, 12);


					note_list_copy_notes(&up, &transformed);
					note_list_append(&transformed, &down);

				}
				transformed.index++;
				if (transformed.index >= transformed.len){
					transformed.index=0;

				}
				pwm_set( (c_to_f_ratio((float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100) * 10 ) * (tune * 2 + 1));
				pp6_set_mode_led(transformed.index & 0x7);
			}*/



				// down by 3
		/*		count++;
				period = rate * 1000;
				//if (nl.len > num_down){
					note_list_copy_notes(&nl, &transformed);
					note_list_make_3(&transformed);
					note_list_octaves_down(&transformed, (int)(range * 8));
				//}
				num_down = nl.len;


				if (count > period) {
					count = 0;

					/*if (!(transformed.index % nl.len)){   // every roll over of notes held down
						note_list_copy_notes(&nl, &transformed);
						note_list_make_3(&transformed);
						note_list_octaves_down(&transformed, (int)(range * 8));
					}*/
				/*	transformed.index++;
					if (transformed.index >= transformed.len){
						transformed.index=0;

					}
					pwm_set( (c_to_f_ratio((float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100) * 10 ) * (tune * 2 + 1));
					pp6_set_mode_led(transformed.index & 0x7);
				}*/




			// up down
			//count++;
			//period = rate * 1000;
		/*	if (note_list_changed_length(&nl)){
				note_list_copy_notes(&nl, &transformed);
				note_list_octaves_down(&transformed, (int)(range * 8));
			}*/


		/*	if (count > period) {
				count = 0;
				transformed.index++;
				if (transformed.index >= transformed.len){
					transformed.index=0;
					note_list_copy_notes(&nl, &transformed);
					note_list_octaves_down(&transformed, (int)(range * 8));
					my_octave = 1;
				}
				pwm_set( (c_to_f_ratio((float32_t)(transformed.note_list[transformed.index]  + (oct * 12)) * 100) * 10 ) * (tune * 2 + 1));
				pp6_set_mode_led(transformed.index & 0x7);
			}*/



			// single shot
			if (note_list_most_recent(&nl) != current_note){
				current_note = note_list_most_recent(&nl);
			//	pwm_set( (c_to_f_ratio((float32_t)current_note * 100) * 50 ) * (tune * 2 + 1));
			}


			//pp6_set_mode_led(c);

			// if we got a note
			/*if (pp6_get_synth_note_start()){

				pwm_set( (c_to_f_ratio((float32_t)pp6_get_synth_note() * 100) * 50 ) * (tune * 2 + 1));
				pp6_set_gate(1);

			}
			if (pp6_get_synth_note_stop()){

				pp6_set_gate(0);
			}*/






		/*	for (i = 0; i< 16; i++) {
				if (! ((pp6_get_keys() >> i) & 1) ) {
					v = (i * (1.f / 12.f)) + 2.f;

					pwm_set( (c_to_f_ratio(i * 100) * 100 ) * (tune * 2 + 1));

					break;
				}
			}

			k++;
			k &= 0xff;
			if (k == 0) sendNoteOff(1, i+ 60, 0);
			if (k == 128) sendNoteOn(1, i + 60, 110);

			if (!k){
				c++;
				c &= 0x7;
				pp6_set_mode_led(c);
				pp6_set_seq_led(c);
				pp6_set_clk_led(c);


			}*/


			// test vout
			//if ( (!(( pp6_get_keys() >>16) & 1)) )
			//	v = v + 1.f;


			// clear all the events
			pp6_clear_flags();
			led_counter++;
			// ready for new note list
			note_list_set_current_to_last(&nl);
			note_list_set_current_to_last(&transformed);

			//v = 5.f;

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
		if (pp6_get_mode_led()){
			pp6_set_mode_led(0);
		}
		else {
			pp6_set_mode_led(1);
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
