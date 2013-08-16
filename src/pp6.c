/*
 * pp6.c
 *
 *  Created on: Jun 23, 2012
 *      Author: owen
 */


#ifdef __USE_CMSIS
#include "stm32f4xx.h"
#endif
#include "pp6.h"


#define ABS(a)	   (((a) < 0) ? -(a) : (a))

// from DAC in CS4344.c driver
extern uint32_t sample_clock;

pocket_piano pp6;

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* You can monitor the converted value by adding the variable "ADC1ConvertedValue"
   to the debugger watch window */


// keypad values
//uint32_t keys = 0xFFFFFFFF;

uint32_t keys_history[] = {
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
};

void pp6_init(void) {
	uint8_t i;
	pp6_init_digi_out();
	pp6_knobs_init();
	pp6_init_digi_in();

	pp6_set_mode_led(BLACK);

	pp6_set_mode(0);

	pp6.secret_mode_enabled = 0;

	pp6.knob_touched[0] = 0;
	pp6.knob_touched[1] = 0;
	pp6.knob_touched[2] = 0;
	pp6.physical_notes_on = 0;
	pp6.midi_start_flag = 0;
	pp6.midi_stop_flag = 0;
	pp6.keys =  0xFFFFFFFF;
	pp6.keys_last =  0xFFFFFFFF;

	pp6.midi_clock_period = 0;
	pp6.midi_clock_present = 0;
	pp6.midi_in_clock_last = 0;
	pp6.midi_clock_tick_count = 0;

	pp6.seq_led_flash = 0;
	pp6.mode_led_flash = 0;

	pp6.clock_source = CLK_SRC_INT;

	pp6.gate_state = 0;

	// init the note on arrays
	for (i = 0; i < 128; i++) {
		pp6.note_state[i] = 0;
		pp6.note_state_last[i] = 0;
	}
}


void pp6_set_trig(uint32_t stat){
	if (stat)
		GPIO_WriteBit(GPIOD, GPIO_Pin_3, 1);
	else
		GPIO_WriteBit(GPIOD, GPIO_Pin_3, 0);
}

void pp6_set_gate(uint32_t stat){
	pp6.gate_state = stat;
	if (stat)
		GPIO_WriteBit(GPIOD, GPIO_Pin_4, 1);
	else
		GPIO_WriteBit(GPIOD, GPIO_Pin_4, 0);
}

uint8_t pp6_get_gate(void){
	return pp6.gate_state ;
}


uint8_t pp6_get_cv_clk(void){
	if (!(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6))) return 0;
	else return 1;
}

uint8_t pp6_get_clk_src(void){
	return pp6.clock_source;
}
void pp6_set_clk_src(uint8_t s){
	pp6.clock_source = s;
}

void pp6_enable_secret_mode(void) {
	pp6.secret_mode_enabled = 1;
}

/// MIDI clock stuff
void pp6_midi_clock_tick(void) {

	pp6.midi_clock_flag = 1; // set the flag used elsewhere

	pp6.midi_clock_period = sample_clock - pp6.midi_in_clock_last;
	pp6.midi_in_clock_last = sample_clock;

	pp6.midi_clock_tick_count++;
	if (pp6.midi_clock_tick_count == 24){
		pp6.midi_clock_tick_count = 0;
//		pp6_flash_mode_led(40);
	}
}

uint8_t pp6_get_midi_clock_tick(void) {
	return pp6.midi_clock_flag;
}

uint8_t pp6_get_midi_clock_count(void) {
	return pp6.midi_clock_tick_count;
}

void pp6_clear_midi_clock_tick(void){
	pp6.midi_clock_flag = 0;
}

void pp6_check_for_midi_clock(void) {
    // check for presence of midi clock signal
    if ((sample_clock - pp6.midi_in_clock_last) > MIDI_CLOCK_TIMEOUT) {
        pp6.midi_clock_present = 0;
    }
    else {
        pp6.midi_clock_present = 1;
    }
}

uint8_t pp6_midi_clock_present(void) {
	return pp6.midi_clock_present;
}

uint32_t pp6_get_midi_clock_period(void) {
	return pp6.midi_clock_period;
}


float32_t pp6_get_knob_1(void){
	return pp6.knob[0];
}
float32_t pp6_get_knob_2(void){
	return pp6.knob[1];
}
float32_t pp6_get_knob_3(void){
	return pp6.knob[2];
}
float32_t pp6_get_knob_4(void){
	return pp6.knob[3];
}
float32_t pp6_get_knob_5(void){
	return pp6.knob[4];
}

void pp6_set_knob_1(float32_t v){
	pp6.knob[0] = v;
}
void pp6_set_knob_2(float32_t v){
	pp6.knob[1] = v;
}
void pp6_set_knob_3(float32_t v){
	pp6.knob[2] = v;
}

float32_t * pp6_get_knob_array(void){
	return pp6.knob;
}

void pp6_set_knob_array(float32_t * knobs){
	pp6.knob[0] = knobs[0];
	pp6.knob[1] = knobs[1];
	pp6.knob[2] = knobs[2];
}

// mode and aux buttons
void  pp6_set_mode_button_pressed(void){
	pp6.mode_button_pressed = 1;
}
void pp6_set_mode_button_released(void){
	pp6.mode_button_released = 1;
}
uint8_t pp6_mode_button_pressed(void){
	return pp6.mode_button_pressed;
}
uint8_t pp6_mode_button_released(void){
	return pp6.mode_button_released;
}

void  pp6_set_aux_button_pressed(void){
	pp6.aux_button_pressed = 1;
}
void pp6_set_aux_button_released(void){
	pp6.aux_button_released = 1;
}
uint8_t pp6_aux_button_pressed(void){
	return pp6.aux_button_pressed;
}
uint8_t pp6_aux_button_released(void){
	return pp6.aux_button_released;
}

// MODE
void pp6_change_mode(void){
	pp6.mode++;
	if (pp6.secret_mode_enabled){
		if (pp6.mode == 7) pp6.mode = 0;
	}
	else {
		if (pp6.mode == 6) pp6.mode = 0;
	}
		pp6_set_mode_led(pp6.mode + 1);
}

void pp6_set_mode(uint32_t mode){
	pp6.mode = mode;
	pp6_set_mode_led(pp6.mode + 1);
}

uint32_t pp6_get_mode(void){
	return pp6.mode;
}

// seq LED
void pp6_set_seq_led(uint8_t led) {
	pp6.seq_led = led;
	if (!pp6.seq_led_flash) {
		if (led == 0) {SEQ_LED_RED_OFF;SEQ_LED_GREEN_OFF;SEQ_LED_BLUE_OFF;}
		if (led == 1) {SEQ_LED_RED_ON;SEQ_LED_GREEN_OFF;SEQ_LED_BLUE_OFF;}
		if (led == 2) {SEQ_LED_RED_ON;SEQ_LED_GREEN_ON;SEQ_LED_BLUE_OFF;}
		if (led == 3) {SEQ_LED_RED_OFF;SEQ_LED_GREEN_ON;SEQ_LED_BLUE_OFF;}
		if (led == 4) {SEQ_LED_RED_OFF;SEQ_LED_GREEN_ON;SEQ_LED_BLUE_ON;}
		if (led == 5) {SEQ_LED_RED_OFF;SEQ_LED_GREEN_OFF;SEQ_LED_BLUE_ON;}
		if (led == 6) {SEQ_LED_RED_ON;SEQ_LED_GREEN_OFF;SEQ_LED_BLUE_ON;}
		if (led == 7) {SEQ_LED_RED_ON;SEQ_LED_GREEN_ON;SEQ_LED_BLUE_ON;}
	}
}

uint8_t pp6_get_seq_led(void){
	return pp6.seq_led;
}


// mode LED
void pp6_set_mode_led(uint8_t led) {
	pp6.mode_led = led;
	if (!pp6.mode_led_flash) {
		if (led == 0) {MODE_LED_RED_OFF;MODE_LED_GREEN_OFF;MODE_LED_BLUE_OFF;}
		if (led == 1) {MODE_LED_RED_ON;MODE_LED_GREEN_OFF;MODE_LED_BLUE_OFF;}
		if (led == 2) {MODE_LED_RED_ON;MODE_LED_GREEN_ON;MODE_LED_BLUE_OFF;}
		if (led == 3) {MODE_LED_RED_OFF;MODE_LED_GREEN_ON;MODE_LED_BLUE_OFF;}
		if (led == 4) {MODE_LED_RED_OFF;MODE_LED_GREEN_ON;MODE_LED_BLUE_ON;}
		if (led == 5) {MODE_LED_RED_OFF;MODE_LED_GREEN_OFF;MODE_LED_BLUE_ON;}
		if (led == 6) {MODE_LED_RED_ON;MODE_LED_GREEN_OFF;MODE_LED_BLUE_ON;}
		if (led == 7) {MODE_LED_RED_ON;MODE_LED_GREEN_ON;MODE_LED_BLUE_ON;}
	}
}

uint8_t pp6_get_mode_led(void) {
	return pp6.mode_led;
}

// clk LED
void pp6_set_clk_led(uint8_t led) {
	pp6.clk_led = led;
	//if (!pp6.clk_led_flash) {
		if (led == 0) {CLK_LED_RED_OFF;CLK_LED_GREEN_OFF;CLK_LED_BLUE_OFF;}
		if (led == 1) {CLK_LED_RED_ON;CLK_LED_GREEN_OFF;CLK_LED_BLUE_OFF;}
		if (led == 2) {CLK_LED_RED_ON;CLK_LED_GREEN_ON;CLK_LED_BLUE_OFF;}
		if (led == 3) {CLK_LED_RED_OFF;CLK_LED_GREEN_ON;CLK_LED_BLUE_OFF;}
		if (led == 4) {CLK_LED_RED_OFF;CLK_LED_GREEN_ON;CLK_LED_BLUE_ON;}
		if (led == 5) {CLK_LED_RED_OFF;CLK_LED_GREEN_OFF;CLK_LED_BLUE_ON;}
		if (led == 6) {CLK_LED_RED_ON;CLK_LED_GREEN_OFF;CLK_LED_BLUE_ON;}
		if (led == 7) {CLK_LED_RED_ON;CLK_LED_GREEN_ON;CLK_LED_BLUE_ON;}
	//}
}

uint8_t pp6_get_clk_led(void) {
	return pp6.clk_led;
}

// LED flashing
void pp6_flash_seq_led(uint8_t flash_time) {
	pp6.seq_led_flash = flash_time;
}

void pp6_flash_mode_led(uint8_t flash_time) {
	pp6.mode_led_flash = flash_time;
}



void pp6_flash_update(void) {

	if (pp6.mode_led_flash) {
		MODE_LED_RED_ON;MODE_LED_GREEN_ON;MODE_LED_BLUE_ON;
		pp6.mode_led_flash--;
		if (pp6.mode_led_flash == 0) {
			pp6_set_mode_led(pp6.mode_led);
		}
	}

	if (pp6.seq_led_flash) {
		SEQ_LED_RED_ON;SEQ_LED_GREEN_ON;SEQ_LED_BLUE_ON;
		pp6.seq_led_flash--;
		if (pp6.seq_led_flash == 0) {
			pp6_set_seq_led(pp6.seq_led);
		}
	}
}


// keys

uint32_t pp6_get_keys(void) {
		return pp6.keys;
}


// key scanning and assignment, this must be called after pp6_keys_update()
void pp6_get_key_events(void) {

	uint32_t k, k_last;

	uint32_t i = 0;

	// scan keys 16 keys


	k =  pp6.keys;
	k_last = pp6.keys_last;

	pp6.num_keys_down = 0;
	for (i = 0; i < 16; i++) {
		if ( !((k>>i) & 1) ) {
			pp6.num_keys_down++;
		}
		if ( (!((k>>i) & 1)) &&  (((k_last>>i) & 1))  )  {  // new key down
			pp6_set_note_on(i + 36);   // keyboard starts at midi note 36
			pp6_inc_physical_notes_on();
		}
		if ( ((k>>i) & 1) &&  (!((k_last>>i) & 1))  )  {  // key up
			pp6_set_note_off(i + 36);   // keyboard starts at midi note 36
			pp6_dec_physical_notes_on();
		}
	}

	// check mode and aux button  (we only care about a mode press, but need press and release events for aux button)
	if ( (!((k>>18) & 1)) &&  (((k_last>>18) & 1)) ){
		pp6_set_mode_button_pressed();
	}
	if ( (!((k>>17) & 1)) &&  (((k_last>>17) & 1)) ){
		pp6_set_aux_button_pressed();
	}
	if ( (((k>>17) & 1)) &&  (!((k_last>>17) & 1)) ){
		pp6_set_aux_button_released();
	}

	// store keys for next time
	pp6.keys_last = pp6.keys;
}

// the note interface for the piano
// TODO :  WHAT THE HELL ARE ALL THESE ?????  CLEAN IT UP
// TODO :  ahhh, what if more then one key is pressed at same time ??, set_note_on can only handle 1 note per 'tick'
uint8_t pp6_note_on_flag() {
	return pp6.note_on_flag;
}
uint8_t pp6_note_off_flag() {
	return pp6.note_off_flag;
}

void pp6_set_note_off(uint8_t note){
	pp6.note_state[note & 0x7f] = 0;
	pp6.note_off = note;
	pp6.note_off_flag = 1;
}
void pp6_set_note_on(uint8_t note){
	pp6.note_state[note & 0x7f] = 1;
	pp6.note_on = note;
	pp6.note_on_flag = 1;
}
uint8_t pp6_get_note_state(uint8_t note){
	return pp6.note_state[note & 0x7f];
}
uint8_t pp6_get_note_state_last(uint8_t note){
	return pp6.note_state_last[note & 0x7f];
}
void pp6_set_current_note_state_to_last(void){
	uint8_t i;
	for (i = 0; i < 128; i++){
		pp6.note_state_last[i] = pp6.note_state[i];
	}
}
void pp6_turn_off_all_on_notes(void) {
	uint8_t i;
	for (i = 0; i < 128; i++){
		if (pp6.note_state[i]) {
			pp6.note_state[i] = 0;
		}
	}
}

// The actual mono synth voice
void pp6_set_synth_note(uint8_t note) {
	pp6.synth_note = note;
}
uint8_t pp6_get_synth_note(void) {
	return pp6.synth_note;

}
void pp6_set_synth_note_start (void ) {
	pp6.synth_note_stop = 0;
	pp6.synth_playing = 1;
	pp6.synth_note_start = 1;
}
uint8_t pp6_get_synth_note_start(void) {
	return pp6.synth_note_start;
}
void pp6_set_synth_note_stop(void){
	pp6.synth_note_start = 0;
	pp6.synth_note_stop = 1;
}
uint8_t pp6_get_synth_note_stop(void){
	return pp6.synth_note_stop;
}

//MIDI Events
void pp6_set_midi_start(void) {
	pp6.midi_start_flag = 1;
	pp6.midi_clock_tick_count = 0;
}
uint8_t pp6_get_midi_start(void){
	return pp6.midi_start_flag;
}
void pp6_set_midi_stop(void) {
	pp6.midi_stop_flag = 1;
}
uint8_t pp6_get_midi_stop(void){
	return pp6.midi_stop_flag;
}

void pp6_clear_flags(void){
	pp6.synth_note_stop = 0;
	pp6.synth_note_start = 0;
	pp6.aux_button_pressed = 0;
	pp6.aux_button_released = 0;
	pp6.mode_button_pressed = 0;
	pp6.mode_button_released = 0;
	pp6.knob_touched[0] = 0;
	pp6.knob_touched[1] = 0;
	pp6.knob_touched[2] = 0;
	pp6.midi_start_flag = 0;
	pp6.midi_stop_flag = 0;
	pp6.midi_clock_flag = 0;

	pp6.note_on_flag = 0;
	pp6.note_off_flag = 0;
}

uint8_t pp6_get_num_keys_down(void){
	return pp6.num_keys_down;
}

void pp6_inc_physical_notes_on(void){
	pp6.physical_notes_on++;
}
void pp6_dec_physical_notes_on(void){
	if (pp6.physical_notes_on) pp6.physical_notes_on--;   // check it is positive in case it turns on while a key is pressed or during midi note
}

uint8_t pp6_get_physical_notes_on(void){
	return pp6.physical_notes_on;
}

/**
  * @brief  ADC1 channel12 with DMA configuration
  * @param  None
  * @retval None
  */
void pp6_knobs_init(void)
{
	ADC_InitTypeDef       ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	GPIO_InitTypeDef      GPIO_InitStructure;

	/* Enable ADC1 GPIO clocks ****************************************/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOCEN,ENABLE);//Clock for the ADC port!! Do not forget about this one ;)


	/* Configure ADC1 Channel12, 11, 10, 9, 8 pin as analog input ******************************/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_1 | GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	// adc 9, 8 on pb0, pb1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_0;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	//ADC_DeInit();
	/* ADC Common Init **********************************************************/
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div6;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	/* ADC1 Init ****************************************************************/
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;// conversion is synchronous with TIM1 and CC1 (actually I'm not sure about this one :/)
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	/* ADC1 regular configuration *************************************/
	ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 1, ADC_SampleTime_144Cycles);

	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);
	ADC_SoftwareStartConv(ADC1);


/*
	 ADC_InitTypeDef ADC_init_structure; //Structure for adc confguration
	 GPIO_InitTypeDef GPIO_initStructre; //Structure for analog input pin
	 //Clock configuration
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);//The ADC1 is connected the APB2 peripheral bus thus we will use its clock source
	 RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOCEN,ENABLE);//Clock for the ADC port!! Do not forget about this one ;)
	 //Analog pin configuration
	 GPIO_initStructre.GPIO_Pin = GPIO_Pin_0;//The channel 10 is connected to PC0
	 GPIO_initStructre.GPIO_Mode = GPIO_Mode_AN; //The PC0 pin is configured in analog mode
	 GPIO_initStructre.GPIO_PuPd = GPIO_PuPd_NOPULL; //We don't need any pull up or pull down
	 GPIO_Init(GPIOC,&GPIO_initStructre);

	 //ADC structure configuration
	 ADC_DeInit();
	 ADC_init_structure.ADC_DataAlign = ADC_DataAlign_Right;//data converted will be shifted to right
	 ADC_init_structure.ADC_Resolution = ADC_Resolution_12b;//Input voltage is converted into a 12bit number giving a maximum value of 4096
	 ADC_init_structure.ADC_ContinuousConvMode = DISABLE; //the conversion is continuous, the input data is converted more than once
	 ADC_init_structure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;// conversion is synchronous with TIM1 and CC1 (actually I'm not sure about this one :/)
	 ADC_init_structure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//no trigger for conversion
	 ADC_init_structure.ADC_NbrOfConversion = 1;//I think this one is clear :p
	 ADC_init_structure.ADC_ScanConvMode = DISABLE;//The scan is configured in one channel
	 ADC_Init(ADC1,&ADC_init_structure);
	 //Enable ADC conversion
	 ADC_Cmd(ADC1,ENABLE);
	 //Select the channel to be read from
	 ADC_RegularChannelConfig(ADC1,ADC_Channel_10,1,ADC_SampleTime_144Cycles);

	 ADC_SoftwareStartConv(ADC1);//Start the conversion
	// while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));//Processing the conversion
*/


}

void pp6_knobs_update(void) {
	static uint8_t channel = 0;
	static uint32_t knobs[5];

	knobs[channel] = ADC_GetConversionValue(ADC1);


	/// ahhh so it was 65536 ,, then it started needing 4096 (the expected value) ??
	//pp6.knob[0] = knobs[0] / 65536.f;
	//pp6.knob[1] = knobs[1] / 65536.f;
	//pp6.knob[2] = knobs[2] / 65536.f;

	pp6.knob[0] = knobs[0] / 4096.f;
	pp6.knob[1] = knobs[1] / 4096.f;
	pp6.knob[2] = knobs[2] / 4096.f;
	pp6.knob[3] = knobs[3] / 4096.f;
	pp6.knob[4] = knobs[4] / 4096.f;


	channel++;
	if (channel > 4) channel = 0;

	if (channel == 0) ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 1, ADC_SampleTime_144Cycles);
	if (channel == 1) ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 1, ADC_SampleTime_144Cycles);
	if (channel == 2) ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_144Cycles);
	if (channel == 3) ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 1, ADC_SampleTime_144Cycles);
	if (channel == 4) ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_144Cycles);

	ADC_SoftwareStartConv(ADC1);
}


// this should be called every half second or so
void pp6_check_knobs_touched (void) {
	static float32_t knobs_last[3] = {0, 0, 0};

	if (ABS(pp6.knob[0] - knobs_last[0]) > .1f) {
		pp6.knob_touched[0] = 1;
		knobs_last[0] = pp6.knob[0];
	}

	if (ABS(pp6.knob[1] - knobs_last[1]) > .1f) {
		pp6.knob_touched[1] = 1;
		knobs_last[1] = pp6.knob[1];
	}

	if (ABS(pp6.knob[2] - knobs_last[2]) > .1f) {
		pp6.knob_touched[2] = 1;
		knobs_last[2] = pp6.knob[2];
	}
}

uint8_t pp6_any_knobs_touched(void) {
	if (pp6.knob_touched[0] || pp6.knob_touched[1] || pp6.knob_touched[2])
		return 1;
	else
		return 0;
}
uint8_t pp6_knob_1_touched(void){
	return pp6.knob_touched[0];
}
uint8_t pp6_knob_2_touched(void){
	return pp6.knob_touched[1];
}
uint8_t pp6_knob_3_touched(void){
	return pp6.knob_touched[2];
}

void pp6_smooth_knobs(void){


	static float32_t knob1 = 0;
	static float32_t knob2 = 0;
	static float32_t knob3 = 0;
	float32_t kFilteringFactor = .05f; // the smoothing factor 1 is no smoothing

	knob1 = (pp6.knob[0] * kFilteringFactor) + (knob1 * (1.0 - kFilteringFactor));
	pp6.knob[0] = knob1;

	knob2 = (pp6.knob[1] * kFilteringFactor) + (knob2 * (1.0 - kFilteringFactor));
	pp6.knob[1] = knob2;

	knob3 = (pp6.knob[2] * kFilteringFactor) + (knob3 * (1.0 - kFilteringFactor));
	pp6.knob[2] = knob3;
}


void pp6_init_digi_out(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	/* GPIOE Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);


	/* Configure PE0, PE1 in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// trig out pd3, gate out pd4
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

}



void pp6_init_digi_in(void) {

	GPIO_InitTypeDef  GPIO_InitStructure;

	pp6.keys = 0xFFFFFFFF;

	/* Periph clocks enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);


	  /* Configure Buttons pin as input */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_15 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_8;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_15 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_0 | GPIO_Pin_14 | GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	// this is the clock input
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

}


void pp6_keys_update(void){

	static uint8_t history_index;

	keys_history[(history_index++) & 0x7] =
			GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) |
			(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) << 1) |
			(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) << 2) |
			(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) << 3) |
			(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_8) << 4) |
			(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_9) << 5) |
			(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_15) << 6) |
			(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_10) << 7) |
			(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6) << 8) |
			(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_11) << 9) |
			(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8) << 10) |
			(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_12) << 11) |
			(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_13) << 12) |
			(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_0) << 13) |
			(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_14) << 14) |
			(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_1) << 15) |
			(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_3) << 16) |
			(GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_2) << 17) |
			(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2) << 18);

	// debounce keys
	if ((keys_history[0] == keys_history[1]) &&
		(keys_history[0] == keys_history[2]) &&
		(keys_history[0] == keys_history[3]) &&
		(keys_history[0] == keys_history[4]) &&
		(keys_history[0] == keys_history[5]) &&
		(keys_history[0] == keys_history[6]) &&
		(keys_history[0] == keys_history[7]))
	{
		pp6.keys = keys_history[0];
	}
}
