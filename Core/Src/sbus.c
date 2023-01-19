#include "main.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// Sbus output is connected to TIM2_CH4 input PA3
// But we use CH3 also

#define	USART_PARSE_START_BIT	(0)
#define	USART_PARSE_DATA_BITS	(1)
#define	USART_PARSE_PARITY_BIT	(2)
#define	USART_PARSE_STOP_BITS	(3)


uint8_t		usart_parse_bit_step 	= 0;
uint8_t		sbus_arr_cnt			= 0;
uint8_t		sbus_msg_ok				= 1;


int 		pwm_arr_us_local	[400];
uint8_t		sbus_arr_tmp		[50];

int			bits_sum 			= 0;

uint8_t		sbus_failsafe		= 0;
uint8_t		sbus_lost_frames	= 0;


uint8_t		sbus_arr			[50];


int 		g_pwm_arr_us[400];
uint8_t		g_fl_convert 		= 0;



void TIM2_IRQHandler(void)
{
	uint16_t	tim_sr		= TIM2->SR;
	uint32_t	tim_ccr3	= 0;
	uint32_t	tim_ccr4	= 0;

	static int	index_sbus	= 0;

	if (tim_sr & TIM_SR_CC3IF)
	{
		// lo
		TIM2->CNT			= 2;
		tim_ccr3			= TIM2->CCR3;

		if (tim_ccr3 > 500)
			index_sbus = 0;

		g_pwm_arr_us[index_sbus++]	= tim_ccr3;
		g_fl_convert				= 0;
	}


	if (tim_sr & TIM_SR_CC4IF)
	{
		// hi
		TIM2->CNT			= 2;
		tim_ccr4			= TIM2->CCR4;

		if (tim_ccr4 > 500)
			index_sbus = 0;

		g_pwm_arr_us[index_sbus++]	= -tim_ccr4;
		g_fl_convert				= 0;
	}
}


int read_sbus(uint16_t* sbus_channels)
{
	memcpy(pwm_arr_us_local, g_pwm_arr_us, sizeof(g_pwm_arr_us));

	memset(g_pwm_arr_us, 0x00, sizeof(g_pwm_arr_us));
	memset(sbus_arr_tmp, 0x00, sizeof(sbus_arr_tmp));


	bits_sum = 0;
	for(int i = 1;i < 400;i++)
		bits_sum += abs(pwm_arr_us_local[i] / 10);

	if (bits_sum != 298)
		return -1;

	uint8_t	stop_bits_found = 0;
	uint8_t	bit_pos			= 8;

	uint8_t	bits_to_add 	= 0;
	uint8_t fl_inc_sbus 	= 0;

	sbus_arr_cnt			= 0;
	sbus_msg_ok				= 1;


	usart_parse_bit_step	= USART_PARSE_START_BIT;



	for(int i = 1;i < 200;i++)
	{
		int8_t  num_bits	= pwm_arr_us_local[i] / 10;
		uint8_t	fl_zero_bits= 0;

		uint8_t bits;

		if (0 == num_bits)
			break;

		if (num_bits < 0)
			fl_zero_bits = 1;

		num_bits = abs(num_bits);


		while (num_bits)
		{
			switch(usart_parse_bit_step)
			{
			case USART_PARSE_START_BIT:		// Start bit: always "0"
				if (num_bits)
					num_bits--;
				else
					while(1);	// Algorithm error catch

				usart_parse_bit_step = USART_PARSE_DATA_BITS;

				continue;


			case USART_PARSE_DATA_BITS:		// Data bits: 8

				if (bit_pos >= num_bits)
				{
					bit_pos     -= num_bits;
					bits_to_add  = num_bits;
					num_bits	 = 0;
				}
				else
				{
					bits_to_add  = bit_pos;
					num_bits	-= bit_pos;
					bit_pos		 = 0;

					fl_inc_sbus  	= 1;

					usart_parse_bit_step = USART_PARSE_PARITY_BIT;
				}

				if (0 == fl_zero_bits)
					bits	  = pow(2, bits_to_add) - 1;
				else
					bits	  = 0;

				sbus_arr_tmp[sbus_arr_cnt] |= bits << (bit_pos);


				if (fl_inc_sbus)
				{
					uint8_t new_byte = 0;
					for(int j = 0;j < 8;j++)
						new_byte |= ((sbus_arr_tmp[sbus_arr_cnt] >> j) & 0x01) << (7 - j);

					sbus_arr_tmp[sbus_arr_cnt] = new_byte;


					sbus_arr_cnt++;
					bit_pos 	= 8;
					fl_inc_sbus	= 0;
				}

				continue;


			case USART_PARSE_PARITY_BIT:	// Parity bit: "0" or "1"
				if (num_bits)
					num_bits--;
				else
					while(1);	// Algorithm error catch

				usart_parse_bit_step = USART_PARSE_STOP_BITS;

				continue;


			case USART_PARSE_STOP_BITS:		// Start bits: always "1"
				if (num_bits)
				{
					num_bits--;
					stop_bits_found++;


					// fl_zero_bits must be "1"
					if (fl_zero_bits)
						sbus_msg_ok = 0;

					if (fl_zero_bits || (stop_bits_found >= 2))
					{
						usart_parse_bit_step = USART_PARSE_START_BIT;
						stop_bits_found		 = 0;
					}
				}
				else
					while(1);	// Algorithm error catch

				continue;
			}
		}
	}


	if (sbus_msg_ok)
	{

		memcpy(sbus_arr, sbus_arr_tmp, sizeof(sbus_arr_tmp));

		sbus_channels[0]  = ((sbus_arr[1]    |sbus_arr[2]<<8)                 & 0x07FF);
		sbus_channels[1]  = ((sbus_arr[2]>>3 |sbus_arr[3]<<5)                 & 0x07FF);
		sbus_channels[2]  = ((sbus_arr[3]>>6 |sbus_arr[4]<<2 |sbus_arr[5]<<10)  & 0x07FF);
		sbus_channels[3]  = ((sbus_arr[5]>>1 |sbus_arr[6]<<7)                 & 0x07FF);
		sbus_channels[4]  = ((sbus_arr[6]>>4 |sbus_arr[7]<<4)                 & 0x07FF);
		sbus_channels[5]  = ((sbus_arr[7]>>7 |sbus_arr[8]<<1 |sbus_arr[9]<<9)   & 0x07FF);
		sbus_channels[6]  = ((sbus_arr[9]>>2 |sbus_arr[10]<<6)                & 0x07FF);
		sbus_channels[7]  = ((sbus_arr[10]>>5|sbus_arr[11]<<3)                & 0x07FF);
		sbus_channels[8]  = ((sbus_arr[12]   |sbus_arr[13]<<8)                & 0x07FF);
		sbus_channels[9]  = ((sbus_arr[13]>>3|sbus_arr[14]<<5)                & 0x07FF);
		sbus_channels[10] = ((sbus_arr[14]>>6|sbus_arr[15]<<2|sbus_arr[16]<<10) & 0x07FF);
		sbus_channels[11] = ((sbus_arr[16]>>1|sbus_arr[17]<<7)                & 0x07FF);
		sbus_channels[12] = ((sbus_arr[17]>>4|sbus_arr[18]<<4)                & 0x07FF);
		sbus_channels[13] = ((sbus_arr[18]>>7|sbus_arr[19]<<1|sbus_arr[20]<<9)  & 0x07FF);
		sbus_channels[14] = ((sbus_arr[20]>>2|sbus_arr[21]<<6)                & 0x07FF);
		sbus_channels[15] = ((sbus_arr[21]>>5|sbus_arr[22]<<3)                & 0x07FF);

		if((sbus_arr[23])      & 0x0001)
			sbus_channels[16] = 2047;
		else
			sbus_channels[16] = 0;

		if((sbus_arr[23] >> 1) & 0x0001)
			sbus_channels[17] = 2047;
		else
			sbus_channels[17] = 0;

		if ((sbus_arr[23] >> 3) & 0x0001) {
			sbus_failsafe = 1;
		} else {
			sbus_failsafe = 0;
		}

		if ((sbus_arr[23] >> 2) & 0x0001) {
			sbus_lost_frames++;
		}
	}

	return 0;
}

