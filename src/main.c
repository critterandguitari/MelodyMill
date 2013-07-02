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

	float32_t v;
	q15_t out;

	uint16_t s;

	v = 0;
	uint8_t k, c;

	pwm_init();
	pwm_set(100);
//	pwm_test();
	midi_init(1);

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

			// check for new key events
			pp6_get_key_events();

			if (pp6_mode_button_pressed()){
				pp6_change_mode();
			}

			// trig, gate, clkin test
			if (pp6_get_num_keys_down()) {
				pp6_set_trig(1);
			}
			else {
				pp6_set_trig(0);
			}

			if (pp6_get_clkin())
				pp6_set_gate(1);
			else
				pp6_set_gate(0);


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
						pp6_set_synth_note_start();
						pp6_set_synth_note(i);
					}
					else {
						sendNoteOff(1, i, 0);
						if (i ==  pp6_get_synth_note()) pp6_set_synth_note_stop();  // if it equals the currently playing note, shut it off
					}
				}
			}
			pp6_set_current_note_state_to_last();

			// if we got a note
			if (pp6_get_synth_note_start()){

				pwm_set( (c_to_f_ratio((float32_t)pp6_get_synth_note() * 100) * 100 ) * (pp6_get_knob_4() * 2 + 1));

			}
			if (pp6_get_synth_note_stop()){


			}

		/*	for (i = 0; i< 16; i++) {
				if (! ((pp6_get_keys() >> i) & 1) ) {
					v = (i * (1.f / 12.f)) + 2.f;

					pwm_set( (c_to_f_ratio(i * 100) * 100 ) * (pp6_get_knob_4() * 2 + 1));

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
