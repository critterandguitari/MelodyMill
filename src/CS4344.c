/*
 * CS4344.c
 * STM34F4 Driver for Audio DAC
 * Created on: Jun 3, 2012
 *      Author: owen
 */

#include "CS4344.h"

unsigned int software_index = 0;
unsigned int hardware_index = 2;
uint16_t play_buf[512];
uint32_t sample_clock = 0;

uint32_t CS4344_init(void) {

	GPIO_InitTypeDef GPIO_InitStructure;
	I2S_InitTypeDef I2S_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure;

	// inialize GPIO for I2S
	/* Enable I2S GPIO clocks */
	RCC_AHB1PeriphClockCmd(CODEC_I2S_GPIO_CLOCK, ENABLE);

	/* CODEC_I2S pins configuration: WS, SCK and SD pins -----------------------------*/
	GPIO_InitStructure.GPIO_Pin = CODEC_I2S_SCK_PIN | CODEC_I2S_SD_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(CODEC_I2S_GPIO, &GPIO_InitStructure);

	/* Connect pins to I2S peripheral  */
	GPIO_PinAFConfig(CODEC_I2S_WS_GPIO, CODEC_I2S_WS_PINSRC, CODEC_I2S_GPIO_AF);
	GPIO_PinAFConfig(CODEC_I2S_GPIO, CODEC_I2S_SCK_PINSRC, CODEC_I2S_GPIO_AF);

	GPIO_InitStructure.GPIO_Pin = CODEC_I2S_WS_PIN ;
	GPIO_Init(CODEC_I2S_WS_GPIO, &GPIO_InitStructure);
	GPIO_PinAFConfig(CODEC_I2S_GPIO, CODEC_I2S_SD_PINSRC, CODEC_I2S_GPIO_AF);

	/* CODEC_I2S pins configuration: MCK pin */
	GPIO_InitStructure.GPIO_Pin = CODEC_I2S_MCK_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(CODEC_I2S_MCK_GPIO, &GPIO_InitStructure);
	/* Connect pins to I2S peripheral  */
	GPIO_PinAFConfig(CODEC_I2S_MCK_GPIO, CODEC_I2S_MCK_PINSRC, CODEC_I2S_GPIO_AF);

	// configure I2S
	/* Enable the CODEC_I2S peripheral clock */
	RCC_APB1PeriphClockCmd(CODEC_I2S_CLK, ENABLE);

	/* CODEC_I2S peripheral configuration */
	SPI_I2S_DeInit(CODEC_I2S);
	I2S_InitStructure.I2S_AudioFreq = I2S_AudioFreq_22k;
	I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;
	//I2S_InitStructure.I2S_Standard = I2S_Standard_MSB;
	I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;
	I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;
	I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;
	I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
	/* Initialize the I2S peripheral with the structure above */
	I2S_Init(CODEC_I2S, &I2S_InitStructure);

	// enable I2S interrupt
	NVIC_InitStructure.NVIC_IRQChannel = SPI3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	SPI_I2S_ITConfig(SPI3, SPI_I2S_IT_TXE, ENABLE);

	I2S_Cmd(SPI3, ENABLE);

	return 0;
}

/**
  * @brief  I2S interrupt management
  * @param  None
  * @retval None
  */

short int o = 0;

void Audio_I2S_IRQHandler(void) {

	uint16_t s;

	// can be used to determine SR, blinking every 128 periods
	/*count++;
	count &= 0xff;

	if (count > 128)
		GPIO_WriteBit(GPIOB, GPIO_Pin_4, 0);
	else
		GPIO_WriteBit(GPIOB, GPIO_Pin_4, 1);*/

	/* Check on the I2S TXE flag */
	if (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) != RESET) {
		s = play_buf[hardware_index];
		hardware_index++;
		hardware_index &= 0xf;
		SPI_I2S_SendData(CODEC_I2S, s);
	    sample_clock++;
	}
}

