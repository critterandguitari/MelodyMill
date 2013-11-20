/*
 * uart.c
 *
 *  Created on: Jul 20, 2012
 *      Author: owen
 */

#include "uart.h"
#include "midi.h"

// UART  buffer
static uint8_t  uart_tx_buf[64];
static uint8_t  uart_tx_buf_write = 0;
static uint8_t  uart_tx_buf_read = 0;

uint8_t  uart_recv_buf[32];
uint8_t  uart_recv_buf_write = 0;
uint8_t  uart_recv_buf_read = 0;


void uart_init(void){
	GPIO_InitTypeDef GPIO_InitStructure;

	USART_InitTypeDef USART_InitStructure;

	NVIC_InitTypeDef NVIC_InitStructure; // this is used to configure the NVIC (nested vector interrupt controller)

	//configure clock for USART, GPIO
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_USART1, ENABLE);
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);

	/*-------------------------- GPIO Configuration ----------------------------*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Connect USART pins to AF */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1); // USART1_TX
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1); // USART1_RX

	/* UART 1 config */
	USART_InitStructure.USART_BaudRate = 31250;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init (USART1, &USART_InitStructure);

	/* Here the USART1 receive interrupt is enabled
	 * and the interrupt controller is configured
	 * to jump to the USART1_IRQHandler() function
	 * if the USART1 receive interrupt occurs
	 */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // enable the USART1 receive interrupt

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		 // we want to configure the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;// this sets the priority group of the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		 // this sets the subpriority inside the group
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			 // the USART1 interrupts are globally enabled
	NVIC_Init(&NVIC_InitStructure);							 // the properties are passed to the NVIC_Init function which takes care of the low level stuff


	/* Enable USART */
	USART_Cmd (USART1, ENABLE);
}

void put_char (uint8_t data) {
	uart_tx_buf[uart_tx_buf_write] = data;
	uart_tx_buf_write++;
	uart_tx_buf_write &= 0x3f;  // 64 bytes
}

void uart_service_tx_buf (void) {
    if (uart_tx_buf_read != uart_tx_buf_write){
    	// if there is nothing in the register already
    	if (!(USART_GetFlagStatus (USART1, USART_FLAG_TXE) == RESET))  {
    		USART_SendData (USART1, uart_tx_buf[uart_tx_buf_read]);
            uart_tx_buf_read++;
            uart_tx_buf_read &= 0x3f;
    	}
    }
}

// this is the interrupt request handler (IRQ) for ALL USART1 interrupts
void USART1_IRQHandler(void){

	// check if the USART1 receive interrupt flag was set
	if( USART_GetITStatus(USART1, USART_IT_RXNE) ){
    	uart_recv_buf[uart_recv_buf_write] = USART_ReceiveData(USART1);

    	// if its a sync, send it thru immediately  to avoid jitter
    	if (uart_recv_buf[uart_recv_buf_write] == STATUS_SYNC) {
	    	uart_tx_buf[uart_tx_buf_write] = STATUS_SYNC;
	    	uart_tx_buf_write++;
	    	uart_tx_buf_write &= 0x3f;  // 64 bytes
    	}

        uart_recv_buf_write++;
        uart_recv_buf_write &= 0x1f;  // 32 bytes

	}
}
