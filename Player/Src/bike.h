/**
  ******************************************************************************
  * @file    
  * @author  ahzzc@sina.com
  * @version V2.00
  * @date    
  * @brief   
   ******************************************************************************
  * @Changlog
  * V2.00
	* 功能：
	* 1.系统电压通过MODE1,MODE2自动检测；
	* 2.时钟功能自动检测，无芯片不显示；
	* 3.通过左右转向进行时间调整模式；
	* 4.通过串口可进行参数标定,	短接低速、SWIM信号进行参数标定；
	* 5.参数保存于EEPROM;
	* 6.一线通功能；
	* 7.档位信息优先读取一线通数据，实现四档信息判断，一线通中断后采用档把数据；
	* 8.开启开门狗功能；
	* 9.通过PCB_VER定义不同的硬件版本，支持0011、0012、0022、0041；
	*10.通过YXT_XX定义不同的控制器版本；
  *
  ******************************************************************************
  */
/******************************************************************************/

#ifndef __BIKE_H__
#define __BIKE_H__

/******************************************************************************/
typedef struct {
	uint32_t NearLight:1;
	uint32_t CRZLight	:1;
	uint32_t TurnLeft	:1;
	uint32_t TurnRight:1;
	uint32_t LRFlash	:1;
	uint32_t Cruise		:1;
	uint32_t ECUERR		:1;
	uint32_t Braked		:1;
	uint32_t Beep			:1;
	uint32_t PhaseERR	:1;
	uint32_t HallERR	:1;
	uint32_t WheelERR	:1;
	uint32_t YXTERR		:1;
	uint32_t ECO			:1;
	uint32_t R				:1;
	uint32_t Charge		:1;
	uint32_t MP3			:1;
	uint32_t BT				:1;
	uint32_t FM				:1;
	uint32_t Play			:1;
	uint32_t Pause		:1;
	uint32_t HasTimer	:1;
	uint32_t time_set	:1;
	
	uint32_t SpeedMode	;
	int32_t  Temperature;
	uint32_t Voltage;	
	uint32_t BatStatus;
	uint32_t Speed;
	uint32_t Mile;
	
	uint32_t Hour;
	uint32_t Minute;
	uint32_t Second;
	uint32_t time_pos;
	
} BIKE_STATUS,*pBIKE_STATUS;
	
typedef struct {
	uint8_t  bike[4];
	uint32_t SysVoltage	;
	uint32_t VolScale	;
	uint32_t TempScale	;
	uint32_t SpeedScale;
	uint32_t Mile;
	uint32_t Sum;
} BIKE_CONFIG,*pBIKE_CONFIG;
	
extern BIKE_STATUS bike;
extern BIKE_CONFIG config;

unsigned int Get_ElapseTick(unsigned int pre_tick);

/******************************************************************************/

#define VOL_CALIBRATIOIN		240UL		//24.0V
#define TEMP_CALIBRATIOIN		250UL		//25.0C
#define SPEED_CALIBRATIOIN	30UL		//30km/h

#define PON_ALLON_TIME			1000UL	//1000ms

#define DISPLAY_MAX_SPEED		40UL		//40km/h
#define SPEEDMODE_DEFAULT		1				//1档

#define PERIMETER						1256		//mm
//#define PERIMETER						1099		//mm
/******************************************************************************/

//define PCB_VER	0011
//#define PCB_VER	0012
#define PCB_VER	0022
//#define PCB_VER	0041

#define ADC_TEMP_CH			ADC_CHANNEL_8
#define ADC_VOL_CH			ADC_CHANNEL_7
#define ADC_SPEED_CH		ADC_CHANNEL_0

#define SPMODE1_PORT		GPIOA
#define SPMODE1_PIN			GPIO_PIN_1
#define SPMODE2_PORT		GPIOA
#define SPMODE2_PIN			GPIO_PIN_2
#define SPMODE3_PORT		GPIOA
#define SPMODE3_PIN			GPIO_PIN_4
#define SPMODE4_PORT		GPIOB
#define SPMODE4_PIN			GPIO_PIN_1

#define NearLight_PORT	GPIOB
#define NearLight_PIN		GPIO_PIN_9
#define TurnRight_PORT	GPIOB
#define TurnRight_PIN		GPIO_PIN_3
#define TurnLeft_PORT		GPIOA
#define TurnLeft_PIN		GPIO_PIN_15
#define Beep_PORT				GPIOB
#define Beep_PIN				GPIO_PIN_4
#define Brake_PORT			GPIOA
#define Brake_PIN				GPIO_PIN_0
//#define CRZLight_PORT		GPIOB
//#define CRZLight_PIN		GPIO_PIN_1
#define LRFlash_PORT		GPIOB
#define LRFlash_PIN			GPIO_PIN_8

#define NearLightOut_PORT	GPIOB
#define NearLightOut_PIN	GPIO_PIN_0
#define TurnLeftOut_PORT	GPIOB
#define TurnLeftOut_PIN		GPIO_PIN_2
#define TurnRightOut_PORT	GPIOB
#define TurnRightOut_PIN	GPIO_PIN_10
#define BeepOut_PORT			GPIOB
#define BeepOut_PIN				GPIO_PIN_1
#define BrakeOut_PORT			GPIOB
#define BrakeOut_PIN			GPIO_PIN_11


#define FM_PORT				GPIOA
#define FM_PIN				GPIO_PIN_1
		
#define Next_PORT			GPIOC
#define Next_PIN			GPIO_PIN_13
#define Pre_PORT			GPIOC
#define Pre_PIN				GPIO_PIN_14
#define Play_PORT			GPIOF
#define Play_PIN			GPIO_PIN_1
#define VolUp_PORT		GPIOC
#define VolUp_PIN			GPIO_PIN_15
#define VolDown_PORT	GPIOF
#define VolDown_PIN		GPIO_PIN_0

#define KEY_NEXT			(1<<0)
#define KEY_PRE				(1<<1)
#define KEY_VOLUP			(1<<2)
#define KEY_VOLDOWN		(1<<3)
#define KEY_PLAY			(1<<4)
#define KEY_CRZ				(1<<5)
#define KEY_TURNLEFT	(1<<6)
#define KEY_TURNRIGHT	(1<<7)
#define KEY_LRFLASH		(1<<8)
#define KEY_NEARLIGHT	(1<<9)
#define KEY_BEEP			(1<<10)
#define KEY_BRAKE			(1<<11)
#define KEY_FM				(1<<12)

#define KEY_ALL		(0xFFFFFFFF)



/******************************************************************************/

#endif
