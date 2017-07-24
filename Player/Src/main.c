/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "main.h"
#include "stm32f0xx_hal.h"
#include "main.h"


/* USER CODE BEGIN Includes */
#include "bike.h"
#include "bl55077.h"
#include "display.h"
#include "pcf8563.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

I2C_HandleTypeDef hi2c1;

IWDG_HandleTypeDef hiwdg;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
const uint16_t BatStatus24[8] = {205,211,218,225,232,239,245,251};
const uint16_t BatStatus48[8] = {420,426,434,443,452,461,470,480};

volatile uint32_t hall_count = 0;
uint32_t keycode=0;
uint32_t tick_100ms=0,tick_10ms=0,tick_1s=0;
uint32_t speed_buf[16];
uint32_t vol_buf[32];
int32_t temp_buf[10];
uint8_t uart_buf[8];
const uint16_t* BatStatus;
uint8_t uart1_rx[32];

BIKE_STATUS bike;
BIKE_CONFIG config;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC_Init(void);
static void MX_IWDG_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

uint32_t Adc_Get(uint32_t channel)
{
	/* Variable used to get converted value */
	__IO uint16_t uhADCxConvertedValue = 0;
	ADC_ChannelConfTypeDef 	sConfig;

  /*##-1- Configure the ADC peripheral #######################################*/

  hadc.Instance          = ADC1;
  
  if (HAL_ADC_DeInit(&hadc) != HAL_OK) { Error_Handler(); }
  
  hadc.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc.Init.Resolution            = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode          = DISABLE;                       /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
  hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait      = DISABLE;
  hadc.Init.LowPowerAutoPowerOff  = DISABLE;
  hadc.Init.ContinuousConvMode    = DISABLE;                       /* Continuous mode disabled to have only 1 conversion at each conversion trig */
  hadc.Init.DiscontinuousConvMode = DISABLE;                       /* Parameter discarded because sequencer is disabled */
  hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;            /* Software start to trig the 1st conversion manually, without external event */
  hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = ENABLE;
  hadc.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;

  if (HAL_ADC_Init(&hadc) != HAL_OK) { Error_Handler(); }

  /*##-2- Configure ADC regular channel ######################################*/
  //sConfig.Channel      = ADCx_CHANNEL;
  sConfig.Channel      = channel;
  sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;

  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK) { Error_Handler(); }
	
	HAL_ADCEx_Calibration_Start(&hadc);

  /*##-3- Start the conversion process #######################################*/
  if (HAL_ADC_Start(&hadc) != HAL_OK) { Error_Handler(); }

  /*##-4- Wait for the end of conversion #####################################*/
  /*  For simplicity reasons, this example is just waiting till the end of the
      conversion, but application may perform other tasks while conversion
      operation is ongoing. */
  if (HAL_ADC_PollForConversion(&hadc, 10) != HAL_OK)
  {
    /* End Of Conversion flag not set on time */
    Error_Handler();
  }
  else
  {
    /* ADC conversion completed */
    /*##-5- Get the converted value of regular channel  ########################*/
    uhADCxConvertedValue = HAL_ADC_GetValue(&hadc);
  }
  return uhADCxConvertedValue;
}

/**
  * @brief  Configures the IWDG to generate a Reset if it is not refreshed at the
  *         correct time. 
  * @param  None
  * @retval None
  */
static void IWDG_Config(void)
{
  /*##-1- Check if the system has resumed from IWDG reset ####################*/
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
  {
  }

  /* Clear reset flags in any cases */
  __HAL_RCC_CLEAR_RESET_FLAGS();

  /*##-3- Configure & Start the IWDG peripheral #########################################*/
  /* Set counter reload value to obtain 1 sec. IWDG TimeOut.
     IWDG counter clock Frequency = uwLsiFreq
     Set Prescaler to 32 (IWDG_PRESCALER_32)
     Timeout Period = (Reload Counter Value * 32) / LSI_VALUE
     So Set Reload Counter Value = (1 * LSI_VALUE) / 32 */
  hiwdg.Instance 				= IWDG;
  hiwdg.Init.Prescaler 	= IWDG_PRESCALER_64;
  hiwdg.Init.Reload 		= (LSI_VALUE / 32);
  hiwdg.Init.Window 		= IWDG_WINDOW_DISABLE;

  if(HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

void IWDG_Feed(void)
{
	if(HAL_IWDG_Refresh(&hiwdg) != HAL_OK)
	{
		/* Refresh Error */
		Error_Handler();
	}
}

uint32_t Get_ElapseTick(uint32_t pre_tick)
{
	uint32_t tick = HAL_GetTick();

	if ( tick >= pre_tick )	
		return (tick - pre_tick); 
	else 
		return (0xFFFF - pre_tick + tick);
}


void KeyTask()
{
	static uint32_t pre_key=0;
	static uint32_t counter=0;
	uint32_t key=0;

	MX_GPIO_Init();
	
	key |=!HAL_GPIO_ReadPin(Next_PORT	, Next_PIN		)?(KEY_NEXT		):0;
	key |=!HAL_GPIO_ReadPin(Pre_PORT	, Pre_PIN		)?(KEY_PRE		):0;
	key |=!HAL_GPIO_ReadPin(VolUp_PORT	, VolUp_PIN		)?(KEY_VOLUP	):0;
	key |=!HAL_GPIO_ReadPin(VolDown_PORT, VolDown_PIN	)?(KEY_VOLDOWN	):0;
	key |=!HAL_GPIO_ReadPin(Play_PORT	, Play_PIN		)?(KEY_PLAY		):0;
	//key |=!HAL_GPIO_ReadPin(CRZLight_PORT,CRZLight_PIN)?(KEY_CRZ		):0;
	key |=!HAL_GPIO_ReadPin(TurnLeft_PORT,TurnLeft_PIN	)?(KEY_TURNLEFT	):0;
	key |=!HAL_GPIO_ReadPin(TurnRight_PORT,TurnRight_PIN)?(KEY_TURNRIGHT):0;
	key |=!HAL_GPIO_ReadPin(LRFlash_PORT, LRFlash_PIN	)?(KEY_LRFLASH	):0;
	key |=!HAL_GPIO_ReadPin(NearLight_PORT,NearLight_PIN)?(KEY_NEARLIGHT):0;
	key |=!HAL_GPIO_ReadPin(Beep_PORT	, Beep_PIN		)?(KEY_BEEP		):0;
	key |=!HAL_GPIO_ReadPin(Brake_PORT	, Brake_PIN		)?(KEY_BRAKE	):0;
	key |=!HAL_GPIO_ReadPin(FM_PORT		, FM_PIN		)?(KEY_FM		):0;
	
	if ( key == pre_key ){
		if ( counter ++ >= 10 )
			keycode = key;
	} else {
		counter = 0;
	}
	pre_key = key;
}

uint32_t GetKey(uint32_t key)
{
	return (keycode & key);
}

const int32_t NTC_B3950[][2] = 
{
	401860,	-400,	281577,	-350,	200204,	-300,	144317,	-250,	105385,	-200,
	77898,		-150,	58246,	-100,	44026,	-50,	33621,	0,		25925,	50,
	20175,		100,	15837,	150,	12535,	200,	10000,	250,	8037,	300,
	6506,		350,	5301,	400,	4348,	450,	3588,	500,	2978,	550,
	2978,		600,	2086,	650,	1760,	700,	1492,	750,	1270,	800,
	1087,		850,	934,	900,	805,	950,	698,	1000
};

int NTCtoTemp(unsigned int ntc)
{
	int i,j;

	if ( ntc > NTC_B3950[0][0] ){
		return -999;
	} else {
		for(i=0;i<sizeof(NTC_B3950)/sizeof(NTC_B3950[0][0])/2-1;i++){
			if ( ntc <= NTC_B3950[i][0] && ntc > NTC_B3950[i+1][0] )
				break;
			}
			if ( i == sizeof(NTC_B3950)/sizeof(NTC_B3950[0][0])/2-1 ){
				return 999;
			} else {
			for(j=0;j<50;j++){
				if ( NTC_B3950[i][0] - (j*(NTC_B3950[i][0] - NTC_B3950[i+1][0])/50) <= ntc )
					return NTC_B3950[i][1] + j;
			}
			return NTC_B3950[i+1][1];
		}
	}
}


int32_t GetTemp(void)
{
	static uint32_t index = 0;
  int32_t temp;
  uint32_t i;
	
  temp = Adc_Get(ADC_TEMP_CH);
	temp_buf[index++] = temp;
	if ( index >= COUNTOF(temp_buf) )
		index = 0;
	for(i=0,temp=0;i<COUNTOF(temp_buf);i++)
		temp += temp_buf[i];
	temp /= COUNTOF(temp_buf);

	temp = 10000*4096/(4096-temp)-10000;
	temp = NTCtoTemp(temp);
	
	//temp = ((3600- (long)temp * 2905/4096)/10);
	
  return temp;
}

uint32_t GetVol(void)
{
	static uint32_t index = 0;
  uint32_t vol;
  uint32_t i;

  vol = Adc_Get(ADC_VOL_CH);
	vol_buf[index++] = vol;
	if ( index >= COUNTOF(vol_buf) )
		index = 0;
	for(i=0,vol=0;i<COUNTOF(vol_buf);i++)
		vol += vol_buf[i];
	vol /= COUNTOF(vol_buf);
	//adc*3.3V*21/4095 V
  vol = vol*693/4095 ;
	
  return vol;
}

uint32_t GetSpeed(void)
{
	static uint32_t index = 0;
	uint32_t speed;
	uint32_t i;

	speed = Adc_Get(ADC_SPEED_CH);
	speed_buf[index++] = speed;
	if ( index >= COUNTOF(speed_buf) )
		index = 0;

	for(i=0,speed=0;i<COUNTOF(speed_buf);i++)
		speed += speed_buf[i];
	speed /= COUNTOF(speed_buf);
	
	if ( config.SysVoltage	== 48 )	// speed*5V*21/1024/24V*45 KM/H
		speed = speed*875/4096;	//24V->50KM/H
	else if ( config.SysVoltage	== 60 )	// speed*5V*21/1024/30V*45 KM/H
		speed = speed*350/2048;	//30V->50KM/H
	else if ( config.SysVoltage	== 72 )	// speed*5V*21/1024/36V*45 KM/H
		speed = speed*875/6144;	//36V->50KM/H
	else if ( config.SysVoltage	== 24 )	// speed*5V*21/1024/12V*25 KM/H
		speed = speed*875/4096;	//12V->25KM/H
	
	if ( speed > 99 )
		speed = 99;
	
  return speed;
}

void SpeedTask(void)
{
	bike.Speed = hall_count * PERIMETER * 60 * 60 / 1000 / 1000;
	hall_count = 0;
}	

void InitConfig(void)
{
	uint32_t *cbuf = (uint32_t*)&config;
	uint32_t i,sum;
	
	for(i=0;i<sizeof(BIKE_CONFIG)/4;i++)
		cbuf[i] = *(__IO uint32_t *)(FLASH_USER_START_ADDR + i*4);

	for(sum=0,i=0;i<sizeof(BIKE_CONFIG)/4-1;i++)
		sum += cbuf[i];
		
	if ( config.bike[0] != 'b' || 
			 config.bike[1] != 'i' || 
			 config.bike[2] != 'k' || 
			 config.bike[3] != 'e' || 
			sum != config.Sum ){
		config.VolScale  	= 1000;
		config.TempScale 	= 1000;
		config.SpeedScale	= 1000;
		config.Mile			= 0;
		config.PlayMedia	= PM_FM;
	}

	memset((uint8_t*)&bike,sizeof(bike),0);
	bike.Mile 	= config.Mile;
	bike.YXTERR = 1;
	bike.Media |= PM_FM;
	bike.PlayMedia = config.PlayMedia;
	bike.Play 	= 0;
	bike.Codec 	= 0;
	
	config.SysVoltage = SYS_VOLTAGE;
	BatStatus = BAT_STATUS;
}

void WriteConfig(void)
{
	uint32_t Address = 0, PageError = 0;
	uint32_t *dat = (uint32_t *)&config;
	uint32_t i;
	FLASH_EraseInitTypeDef EraseInitStruct;

	/* Unlock the Flash to enable the flash control register access *************/
	HAL_FLASH_Unlock();

	/* Erase the user Flash area
	(area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase 	= FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress 	= FLASH_USER_START_ADDR;
	EraseInitStruct.NbPages 		= (FLASH_USER_END_ADDR - FLASH_USER_START_ADDR + 1) / FLASH_PAGE_SIZE;

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK){}
		
	config.bike[0] = 'b';
	config.bike[1] = 'i';
	config.bike[2] = 'k';
	config.bike[3] = 'e';
	for(config.Sum=0,i=0;i<sizeof(BIKE_CONFIG)/4-1;i++)
		config.Sum += dat[i];
		
	Address = FLASH_USER_START_ADDR;
	for(i=0;i<sizeof(BIKE_CONFIG)/4;i++){
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address+i*4, dat[i]);
	}
	
	/* Lock the Flash to disable the flash control register access (recommended
	to protect the FLASH memory against possible unwanted operation) *********/
	HAL_FLASH_Lock();
}

uint32_t GetBatStatus(uint32_t vol)
{
	uint32_t i;
	
	for(i=0;i<COUNTOF(BAT_STATUS);i++)
		if ( vol < BAT_STATUS[i] ) break;
	return i;
}


void MileTask(void)
{
	static uint32_t Fmile = 0;
	static uint32_t time = 0;
	uint32_t speed;
	
	speed = bike.Speed;
	if ( speed > DISPLAY_MAX_SPEED ) speed = DISPLAY_MAX_SPEED;

#ifdef SINGLE_TRIP
	time ++;
	if ( time < 20 ) {	//2s
		bike.Mile = config.Mile;
	} else if ( time < 50 ) { 	//5s
		if ( speed ) {
			time = 50;
			bike.Mile = 0;
		}
	} else if ( time == 50 ){
		bike.Mile = 0;
	} else 
#endif	
	{
		time = 50;
		
		Fmile = Fmile + speed;
		if(Fmile >= 36000)
		{
			Fmile = 0;
			bike.Mile++;
			if ( bike.Mile > 99999 )	bike.Mile = 0;
			config.Mile ++;
			if ( config.Mile > 99999 )	config.Mile = 0;
			WriteConfig();
		}  
	}
}

void BikeTask(void)
{
	uint32_t speed_mode=0;
	static uint32_t count=0;
	static uint32_t pre_NearLight=0,pre_TurnRight=0,pre_TurnLeft=0,pre_LRFlash=0,pre_Braked=0;

	if ( GetKey(KEY_NEARLIGHT) ){
		if ( pre_NearLight == 0 ){
			if ( bike.NearLight ) bike.NearLight = 0; else bike.NearLight = 1;
		}
		pre_NearLight = 1;
	} else 
		pre_NearLight = 0;
	
	if ( GetKey(KEY_BRAKE) ){
		if ( pre_Braked == 0 ){
			if ( bike.Braked ) bike.Braked = 0; else bike.Braked = 1;
		}
		pre_Braked = 1;
	} else 
		pre_Braked = 0;

	if ( GetKey(KEY_LRFLASH) ){
		if ( pre_LRFlash == 0 ){
			if ( bike.LRFlash ) {
				bike.LRFlash 	= 0;
				bike.TurnLeft = 0;
				bike.TurnRight= 0;
			} else {
				bike.LRFlash 	= 1;
				bike.TurnLeft = 1;
				bike.TurnRight= 1;
			}
		}
		pre_LRFlash = 1;
	} else 
		pre_LRFlash = 0;

	if ( bike.LRFlash == 0 ){
		if ( GetKey(KEY_TURNRIGHT) 	) bike.TurnRight 	= 1; else bike.TurnRight 	= 0;
		if ( GetKey(KEY_TURNLEFT) 	) bike.TurnLeft 	= 1; else bike.TurnLeft 	= 0;
	}
	
  //bike.CRZLight = GetKey(KEY_CRZ);
	if ( GetKey(KEY_BEEP) 	) bike.Beep 	= 1; else bike.Beep 	= 0;

	HAL_GPIO_WritePin (BrakeOut_PORT,BrakeOut_PIN	,bike.Braked?GPIO_PIN_SET:GPIO_PIN_RESET);
	HAL_GPIO_WritePin (BeepOut_PORT	,BeepOut_PIN	,bike.Beep	?GPIO_PIN_SET:GPIO_PIN_RESET);

	bike.Voltage 	= GetVol()*1000UL/config.VolScale;
	//bike.Temperature= GetTemp()	*1000UL/config.TempScale;
	bike.Temperature= GetTemp();
	bike.BatStatus 	= GetBatStatus(bike.Voltage);

	#if 0
	if ( ++count >= 100 ) count = 0;
	bike.Voltage 	= count/10 + count/10*10UL + count/10*100UL + count/10*1000UL;
	bike.Temperature= count/10 + count/10*10UL + count/10*100UL;
	bike.Speed		= count/10 + count/10*10UL;
	bike.Mile		= count/10 + count/10*10UL + count/10*100UL + count/10*1000UL + count/10*10000UL;

	bike.Hour		= count/10 + count/10*10UL;
	bike.Minute		= count/10 + count/10*10UL;
	bike.SpeedMode  = ((count/10)%4)+1;
	bike.BatStatus	= ((count/10)%9);
	#endif
	
	//bike.Temperature = 250;

	
	if ( bike.YXTERR ){
		//bike.Speed = GetSpeed()*1000UL/config.SpeedScale;

		speed_mode = 0;
//		if ( HAL_GPIO_ReadPin(SPMODE1_PORT, SPMODE1_PIN	) ) speed_mode |= 1<<0;
//		if ( HAL_GPIO_ReadPin(SPMODE2_PORT, SPMODE2_PIN	) ) speed_mode |= 1<<1;
//		if ( HAL_GPIO_ReadPin(SPMODE3_PORT, SPMODE3_PIN	) ) speed_mode |= 1<<2;
//		//if ( HAL_GPIO_ReadPin(SPMODE4_PORT, SPMODE4_PIN	) ) speed_mode |= 1<<3;
//		
//		switch(speed_mode){
//			case 0x01: bike.SpeedMode = 1; break;
//			case 0x02: bike.SpeedMode = 2; break;
//			case 0x04: bike.SpeedMode = 3; break;
//			case 0x08: bike.SpeedMode = 4; break;
//			default:	 bike.SpeedMode = 0; break;
//		}
	}
	
	MileTask();
}

void TimeTask(void)
{
	static uint32_t pre_tick=0,time_set_enable = 1;
	static uint32_t pre_key=0;
	uint32_t key;
	
	if (!bike.HasTimer )
		return ;
	
	if ( bike.time_set == 0 && time_set_enable ){
		if ( GetKey(KEY_PLAY) ) {
			if ( Get_ElapseTick(pre_tick) > 6000 ){
				bike.time_pos = 0;
				bike.time_set = 1; 
				pre_tick = HAL_GetTick();
			}		
		} else {
			if ( GetKey(KEY_ALL) || Get_ElapseTick(0) > 3000 ) {
				time_set_enable = 0;
			}
			pre_tick = HAL_GetTick();
		}
	} 
	if ( bike.time_set ) {		
		key = GetKey(KEY_ALL);
		if ( key == 0 && pre_key == KEY_PRE) {
			pre_tick = HAL_GetTick();
			bike.time_pos ++;
			bike.time_pos %= 4;
		} else if ( key == 0 && pre_key == KEY_NEXT ) {
			pre_tick = HAL_GetTick();
			if ( bike.time_pos ) 
				bike.time_pos--;
			else	
				bike.time_pos = 3;
		}
		
		if ( key == 0 && pre_key == KEY_VOLUP  ) {
			pre_tick = HAL_GetTick();
			switch ( bike.time_pos ){
			case 0:	bike.Hour += 10; if ( (bike.Hour % 10) > 3 ) bike.Hour %= 20; else bike.Hour %= 30;	break;
			case 1:	if ( bike.Hour >= 20 ) {
						if ( (bike.Hour % 10) < 3 ) bike.Hour ++; else bike.Hour -= 3; 
						} else {
							if ( (bike.Hour % 10) < 9 ) bike.Hour ++; else bike.Hour -= 9;
						}
						break;
			case 2:	bike.Minute += 10; bike.Minute %= 60;	break;
			case 3:	if ( bike.Minute % 10 < 9 ) bike.Minute ++; else bike.Minute -= 9;break;
			default:bike.time_set = 0; break;
			}
			RtcTime.RTC_Hours 	= bike.Hour;
			RtcTime.RTC_Minutes = bike.Minute;
			PCF8563_SetTime(PCF_Format_BIN,&RtcTime);
		} else if ( key == 0 && pre_key == KEY_VOLDOWN ) {
			pre_tick = HAL_GetTick();
			switch ( bike.time_pos ){
			case 0:	if ( bike.Hour	>= 10 )	bike.Hour -= 10; 	
					else { 
						if ( (bike.Hour % 10 ) > 3 ) bike.Hour += 10; else bike.Hour += 20; 
					}
			break;
			case 1:	if ( bike.Hour % 10 )	bike.Hour --;			
					else {
						if ( bike.Hour >= 20 ) bike.Hour = bike.Hour/10*10 + 3; else bike.Hour = bike.Hour/10*10+9; 
					}
					break;
			case 2: if ( bike.Minute>= 10 ) bike.Minute	-= 10;	else bike.Minute+= 50; break;
			case 3:	if ( bike.Minute%  10 ) bike.Minute --; 	else bike.Minute = bike.Minute/10*10 + 9; break;
			default:bike.time_set = 0; break;
			}
			RtcTime.RTC_Hours 	= bike.Hour;
			RtcTime.RTC_Minutes = bike.Minute;
			PCF8563_SetTime(PCF_Format_BIN,&RtcTime);
		}
		if ( Get_ElapseTick(pre_tick) > 30000 ){
			bike.time_set = 0;
		}
		if ( GetKey(KEY_PLAY) ) {
			if ( Get_ElapseTick(pre_tick) > 6000 ){
				time_set_enable = 0;
				bike.time_set = 0; 
				pre_tick = HAL_GetTick();
			}		
		} else {
			if ( GetKey(KEY_TURNLEFT | KEY_TURNRIGHT | KEY_LRFLASH | KEY_NEARLIGHT | KEY_BEEP | KEY_BRAKE | KEY_FM ) ) 
				bike.time_set = 0;
			pre_tick = HAL_GetTick();
		}
		
		pre_key = key;
	}	else {
		PCF8563_GetTime(PCF_Format_BIN,&RtcTime);
		bike.Hour 		= RtcTime.RTC_Hours;
		bike.Minute 	= RtcTime.RTC_Minutes;
	}
}

void UartTask(void)
{   
//  static uint32_t index = 0;
//	uint8_t dat;
//  
//  if ( HAL_UART_Receive(&huart1, &dat, 1, 0) == HAL_OK ){
//    uart_buf[index++] = dat;
//    if ( index >= sizeof(uart_buf) ) index = 0;
//		if ( index >= 1 && uart_buf[index-1] == '\n' ){
//			if ( index >= 7 ){
//				if ( uart_buf[0] == 'T' /*&& uart_buf[1] == 'i' && uart_buf[2] == 'm' && uart_buf[3] == 'e' */) {
//					RtcTime.RTC_Hours 	= uart_buf[4];
//					RtcTime.RTC_Minutes = uart_buf[5];
//					RtcTime.RTC_Minutes = uart_buf[6];
//					PCF8563_SetTime(PCF_Format_BIN,&RtcTime);
//				}
//			} else if ( uart_buf[0] == 'C' /*&& uart_buf[1] == 'a' && uart_buf[2] == 'l' && uart_buf[3] == 'i' */){
//				bike.Voltage 		= GetVol();
//				bike.Temperature= GetTemp();
//				bike.Speed			= GetSpeed();

//				config.VolScale	= (unsigned long)bike.Voltage*1000UL/VOL_CALIBRATIOIN;					
//				//config.TempScale= (long)bike.Temperature*1000UL/TEMP_CALIBRATIOIN;	
//				config.SpeedScale = (unsigned long)bike.Speed*1000UL/SPEED_CALIBRATIOIN;				
//				WriteConfig();
//			}
//			index = 0;
//    }
//  }
}

void Calibration(void)
{
	uint32_t i;
  GPIO_InitTypeDef GPIO_InitStruct;
#if 0
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : PC13 PC14 PC15 */
  GPIO_InitStruct.Pin  = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	
	for(i=0;i<16;i++){
		HAL_GPIO_WritePin (GPIOD,GPIO_PIN_1,GPIO_PIN_RESET);
		HAL_Delay(10);
		if( HAL_GPIO_ReadPin(SPMODE1_PORT	, SPMODE1_PIN)  == SET ) return;
		HAL_GPIO_WritePin (GPIOD,GPIO_PIN_1,GPIO_PIN_SET);
		HAL_Delay(10);
		if( HAL_GPIO_ReadPin(SPMODE1_PORT	, SPMODE1_PIN)  == RESET ) return;
	}

	bike.Voltage 		= GetVol();
	//bike.Temperature	= GetTemp();
	//bike.Speed		= GetSpeed();

	config.VolScale	= (unsigned long)bike.Voltage*1000UL/VOL_CALIBRATIOIN;					//60.00V
	//config.TempScale= (long)bike.Temperature*1000UL/TEMP_CALIBRATIOIN;	//25.0C
	//config.SpeedScale = (unsigned long)bike.Speed*1000UL/SPEED_CALIBRATIOIN;				//30km/h
	WriteConfig();
#endif
}

void MediaTask(void)
{
	static uint32_t pre_key=0;
	static uint32_t index =0;
	static uint8_t  value=0;
	static uint16_t FM_count=0;
	
	uint32_t key;
	uint8_t cmd_buf[16];
	uint8_t dat;
	
	key = GetKey(KEY_NEXT|KEY_PRE|KEY_VOLUP|KEY_VOLDOWN|KEY_PLAY|KEY_FM);

	cmd_buf[0] = 0xAA;cmd_buf[1] = 0x00;cmd_buf[2] = 0x00;cmd_buf[3] = 0x00;cmd_buf[4] = 0xEF;
	
	if ( bike.Codec == 0 )
		return ;
		
	if ( key == 0 ) {
		if ( pre_key == KEY_PLAY ){
			if ( bike.Play == 0 ) {
				if ( bike.PlayMedia == PM_USB ){
					cmd_buf[0] = 0xAA;cmd_buf[1] = 0x40;cmd_buf[2] = 0x00;cmd_buf[3] = 0x01;cmd_buf[4] = 0xEF;
					if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
					HAL_Delay(100);
				} else if ( bike.PlayMedia == PM_FM ){
					cmd_buf[0] = 0xAA;cmd_buf[1] = 0x40;cmd_buf[2] = 0x00;cmd_buf[3] = 0x02;cmd_buf[4] = 0xEF;
					if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
					HAL_Delay(100);
				}
				bike.Play = 1;
				cmd_buf[1] = 0x01;cmd_buf[3] = 0x00;
				if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
			} else {
				bike.Play = 0;
				cmd_buf[1] = 0x02;cmd_buf[3] = 0x00;
				if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
			}
		} else if ( pre_key == KEY_NEXT ){
			cmd_buf[1] = 0x03;
			if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
		} else if ( pre_key == KEY_PRE ){
			cmd_buf[1] = 0x04;
			if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
		} else if ( pre_key == KEY_VOLUP ){
			if ( ++value > 9 ) value = 9;			
			cmd_buf[1] = 0x30;
			cmd_buf[3] = value;
			if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
		} else if ( pre_key == KEY_VOLDOWN ){
			if ( value ) value --;			
			cmd_buf[1] = 0x30;
			cmd_buf[3] = value;
			if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
		} else if ( pre_key == KEY_FM ){
			if ( bike.PlayMedia == PM_FM && (bike.Media & PM_USB) ){ 
				config.PlayMedia = bike.PlayMedia = PM_USB;
				if ( bike.Play ){
					cmd_buf[0] = 0xAA;cmd_buf[1] = 0x40;cmd_buf[2] = 0x00;cmd_buf[3] = 0x01;cmd_buf[4] = 0xEF;
					if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
				}
			} else {
				config.PlayMedia = bike.PlayMedia = PM_FM;
				if ( bike.Play ){
					cmd_buf[0] = 0xAA;cmd_buf[1] = 0x40;cmd_buf[2] = 0x00;cmd_buf[3] = 0x02;cmd_buf[4] = 0xEF;
					if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
				}
			}
		}
		FM_count = 0;
	}	else if ( key == KEY_FM && bike.PlayMedia == PM_FM ){
		if ( FM_count++ == 30 ){	//3s
			key = 0;
			bike.FMSearch = 1;
			cmd_buf[0] = 0xAA;cmd_buf[1] = 0x05;cmd_buf[2] = 0x00;cmd_buf[3] = 0x01;cmd_buf[4] = 0xEF;
			if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
		} else if ( FM_count > 30 ){
			key = 0;
			FM_count = 31;
		}
	}
	pre_key = key;
}

void MediaStatusTask(void)
{	
	uint8_t* stbuf;
	uint8_t cmd_buf[16];
	uint8_t len,i;
	static uint16_t fm_freq_index=0,fm_search=0;
	static uint8_t head=0;

	//bike.BT		= 1;

	len = huart1.RxXferSize - huart1.RxXferCount;
	if ( len == 0 )
		return ;
	
	if ( head == len ){
		huart1.RxState = HAL_UART_STATE_READY;
		head = 0;
		if(HAL_UART_Receive_IT(&huart1, (uint8_t *)uart1_rx, sizeof(uart1_rx)) != HAL_OK) 
			Error_Handler();
	} else {
		while( len - head >= 5 ){
			if ( uart1_rx[head] == 0xAB && uart1_rx[head+4] == 0xEF ){
				stbuf = uart1_rx+head;
				head += 5;
				switch(stbuf[1]){
					case 0x00:
					case 0x01:
					case 0x02:
					case 0x03:
					case 0x04:
					case 0x05:
						bike.Number = ((uint16_t)stbuf[2]<<8) | stbuf[3];
						if ( bike.FMSearch ){
							if ( bike.Number == 0 ){
								fm_freq_index = 0;
							} else if ( bike.Number == 0xFFFF || fm_freq_index == sizeof(config.FM_Freq)/sizeof(config.FM_Freq[0])) {
								bike.FMSearch = 0;
								//cmd_buf[0] = 0xAA;cmd_buf[1] = 0x01;cmd_buf[2] = config.FM_Freq[0]>>8;cmd_buf[3] = config.FM_Freq[0];cmd_buf[4] = 0xEF;
								cmd_buf[0] = 0xAA;cmd_buf[1] = 0x01;cmd_buf[2] = 0x00;cmd_buf[3] = 0x01;cmd_buf[4] = 0xEF;
								if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK )	
									Error_Handler();
							} else {
								if ( fm_freq_index >= sizeof(config.FM_Freq) )
									fm_freq_index = 0;
								//config.FM_Freq[fm_freq_index++] = bike.Number;
							}
						} else if ( bike.Number == 0 ){
 							bike.Codec = 1;
						}				
						break;
					case 0x30:break;
					case 0x40:
						if 		  ( stbuf[2] == 0x00 && stbuf[3] == 0x01 ){ 
							bike.Media |= PM_USB;
						} else if ( stbuf[2] == 0x01 && stbuf[3] == 0x01 ){ 
							bike.Media &= ~PM_USB;
						} else if ( stbuf[2] == 0x00 && stbuf[3] == 0x00 ){ 
							bike.Media |= PM_FLASH;
						}
						break;
					default: 	break;
				}
			}	else {
				head++;
				continue;
			}
		}
	}
 }


/**
  * @brief  Configures EXTI line 0 (connected to PA.00 pin) in interrupt mode
  * @param  None
  * @retval None
  */
static void EXTI4_15_IRQHandler_Config(void)
{
  GPIO_InitTypeDef   GPIO_InitStructure;

  /* Enable GPIOA clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Configure PA.00 pin as input floating */
  GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Enable and set EXTI line 0 Interrupt to the lowest priority */
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
#if 0
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_6)
  {
    hall_count++;
  }
}
#else
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	static uint32_t pre_tick=0;
	uint32_t speed,tick;
	
	if (GPIO_Pin == GPIO_PIN_6)
	{
		hall_count++;
		tick = HAL_GetTick();
		if ( tick >= pre_tick ) speed = tick - pre_tick;
		else speed = UINT32_MAX - pre_tick + tick;
		pre_tick = tick;

		if ( speed )
			bike.Speed = PERIMETER * 60 * 60 / 1000 / speed / PULSE_C;	
		else
			bike.Speed = 0;
	}
}
#endif
/* USER CODE END 0 */

int main(void)
{

	/* USER CODE BEGIN 1 */
	uint32_t i;
	uint32_t tick;
	uint8_t cmd_buf[5];
	/* USER CODE END 1 */

	/* MCU Configuration----------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_I2C1_Init();
	//MX_TIM3_Init();
	MX_USART1_UART_Init();
	//MX_USART2_UART_Init();
	MX_ADC_Init();
	//MX_IWDG_Init();
	EXTI4_15_IRQHandler_Config();

	/* USER CODE BEGIN 2 */
	InitConfig();
	BL55077_Config(1);
	for(i=0;i<64;i++){
		GetVol();
		GetTemp();
		GetSpeed();
		//IWDG_Feed();  
	}
	//Calibration();

	//bike.HasTimer = !PCF8563_Check();
	//PCF8563_Check();
	bike.HasTimer = PCF8563_GetTime(PCF_Format_BIN,&RtcTime);
	// bike.HasTimer = 1;
	// bike.Hour = 0;
	// bike.Minute = 1;

	//YXT_Init();  

	while ( HAL_GetTick() < PON_ALLON_TIME ) ;//IWDG_Feed();
	BL55077_Config(0);
	//while ( HAL_GetTick() < PON_ALLON_TIME*2 ) ;//IWDG_Feed();

	cmd_buf[0] = 0xAA;cmd_buf[1] = 0x30;cmd_buf[2] = 0x00;cmd_buf[3] = 0x01;cmd_buf[4] = 0xEF;
	if ( HAL_UART_Transmit(&huart1, cmd_buf, 5, 5000)!= HAL_OK)	Error_Handler();
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		tick = HAL_GetTick();
 
		if ( (tick >= tick_10ms && (tick - tick_10ms) > 10 ) || \
			 (tick <  tick_10ms && (0xFFFF - tick_10ms + tick) > 10 ) ) {
			tick_10ms = tick;
			KeyTask();
		}

		if ( (tick >= tick_100ms && (tick - tick_100ms) > 100 ) || \
			 (tick <  tick_100ms && (0xFFFF - tick_100ms + tick) > 100 ) ) {
			tick_100ms = tick;

			BikeTask();    
			//YXT_Task(&bike);  
			TimeTask();   
			MediaTask();
			MediaStatusTask();
			MenuUpdate(&bike);

			/* Reload IWDG counter */
			//IWDG_Feed();
		}

		if ( (tick >= tick_1s && (tick - tick_1s) > 1000 ) || \
		(tick <  tick_1s && (0xFFFF - tick_1s + tick) > 1000 ) ) {
			tick_1s = tick;
			//SpeedTask();
		}

		//UartTask();
	}
	/* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14
                              |RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* ADC init function */
static void MX_ADC_Init(void)
{

  ADC_ChannelConfTypeDef sConfig;

    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) 
    */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure for the selected ADC regular channel to be converted. 
    */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure for the selected ADC regular channel to be converted. 
    */
  sConfig.Channel = ADC_CHANNEL_7;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure for the selected ADC regular channel to be converted. 
    */
//  sConfig.Channel = ADC_CHANNEL_8;
//  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
//  {
//    Error_Handler();
//  }

}

/* I2C1 init function */
static void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure Analogue filter 
    */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure Digital filter 
    */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }

}

/* IWDG init function */
static void MX_IWDG_Init(void)
{

  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }

}

/* TIM3 init function */
static void MX_TIM3_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_SlaveConfigTypeDef sSlaveConfig;
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_IC_InitTypeDef sConfigIC;

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 0;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_IC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
  sSlaveConfig.InputTrigger = TIM_TS_TI1FP1;
  sSlaveConfig.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
  sSlaveConfig.TriggerFilter = 0;
  if (HAL_TIM_SlaveConfigSynchronization(&htim3, &sSlaveConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if(HAL_UART_DeInit(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

  if(HAL_UART_Receive_IT(&huart1, (uint8_t *)uart1_rx, sizeof(uart1_rx)) != HAL_OK)
  {
    Error_Handler();
  }
	
}

/**
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report end of DMA Rx transfer, and 
  *         you can add your own implementation.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  /* Set transmission flag: trasfer complete*/
	if ( UartHandle == &huart1 ){
		if(HAL_UART_Receive_IT(UartHandle, (uint8_t *)uart1_rx, sizeof(uart1_rx)) != HAL_OK) 
			Error_Handler();
	}
}

/**
  * @brief  UART error callbacks
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
 void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
		if(HAL_UART_Receive_IT(UartHandle, (uint8_t *)uart1_rx, sizeof(uart1_rx)) != HAL_OK) Error_Handler();
}

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2|GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_SET);

  /*Configure GPIO pins : PC13 PC14 PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PF0 PF1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA15*/
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB4 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_8 
                          |GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10 PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
