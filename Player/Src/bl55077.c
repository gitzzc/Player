#include <stdint.h>

#include "stm32f0xx_hal.h"
#include "main.h"
#include "bl55077.h"
#include "bike.h"

unsigned char BL_Data[25];
volatile uint32_t  BLTimeout = BL_TIMEOUT;
extern I2C_HandleTypeDef hi2c1;

void BL55077_Config(unsigned char allon)
{
	uint32_t i;

	BL_Data[0] = 0xCC;	//mode set:normal,enable,1/3bias,1:4
	BL_Data[1] = 0xE0;	//device select: 0x07
	BL_Data[2] = 0xF0;	//Blink ctrl:normal,off
	BL_Data[3] = 0xE8;	//sleep ctrl:normal
	BL_Data[4] = 0x00;	//data pointer:0x00
  HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)BL_ADDR, (uint8_t*)BL_Data, 5, 1000);

	for(i=5;i<sizeof(BL_Data);i++)
		BL_Data[i] = allon?0xFF:0x00;
	HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)BL_ADDR, (uint8_t*)BL_Data, sizeof(BL_Data), 1000);
}

HAL_StatusTypeDef BL_Write_Data(unsigned char ADSTART,unsigned char LEN, unsigned char * p)
{
	uint32_t i;
	
	if ( ADSTART + LEN + 5 > sizeof(BL_Data) ) return HAL_ERROR;
	
	for(i=0;i<LEN;i++)
		BL_Data[i+5] = p[i];
		//BL_Data[i+5] = 0xff;
	
	return HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)BL_ADDR, (uint8_t*)BL_Data, LEN+5, 1000);
}


	


