#include <stdlib.h>
#include <stdint.h>

#include "stm32f0xx_hal.h"
#include "main.h"
#include "bike.h"
#include "bl55077.h"

unsigned char flashflag = 0;

const unsigned char SegDataTime[10] 	= {0xCF,0x06,0xAD,0x2F,0x66,0x6B,0xEB,0x07,0xEF,0x6F};
const unsigned char SegDataVoltage[10]= {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x0E,0x7F,0x6F};
const unsigned char SegDataMile[10]		= {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x0E,0x7F,0x6F};
const unsigned char SegDataSpeed[10] 	= {0xCF,0x06,0xAD,0x2F,0x66,0x6B,0xEB,0x07,0xEF,0x6F};
const unsigned char SegDataTemp[10] 	= {0x3F,0x06,0x6D,0x4F,0x56,0x5B,0x7B,0x07,0x7F,0x5F};
const unsigned char SegDataNumber[10]			= {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x0E,0x7F,0x6F};


void MenuUpdate(BIKE_STATUS* bike)
{
	unsigned char i = 0;
	unsigned char BL_Data[20];
  
	flashflag ++;
	flashflag %= 10;
	
	for(i=0;i<sizeof(BL_Data);i++)
		BL_Data[i] = 0x00;
   
	if( bike->TurnLeft && flashflag <= 5 ){
		BL_Data[ 0] |= 0x10;	//S0
		HAL_GPIO_WritePin (TurnLeftOut_PORT	,TurnLeftOut_PIN	,GPIO_PIN_SET);
	} else 
		HAL_GPIO_WritePin (TurnLeftOut_PORT	,TurnLeftOut_PIN	,GPIO_PIN_RESET);

	if( bike->TurnRight&& flashflag <= 5 ){
		BL_Data[10]	|= 0x80;	//S7
		HAL_GPIO_WritePin (TurnRightOut_PORT,TurnRightOut_PIN	,GPIO_PIN_SET);
	} else
		HAL_GPIO_WritePin (TurnRightOut_PORT,TurnRightOut_PIN	,GPIO_PIN_RESET);
	
	if( bike->NearLight ){
		BL_Data[ 3] |= 0x10;	//S2
		HAL_GPIO_WritePin (NearLightOut_PORT,NearLightOut_PIN	,GPIO_PIN_SET);
	} else
		HAL_GPIO_WritePin (NearLightOut_PORT,NearLightOut_PIN	,GPIO_PIN_RESET);

	if( bike->CRZLight	) BL_Data[ 1] 	|= 0x10;	//S1
	if( bike->ECO 		) BL_Data[ 4] 	|= 0x01;	//S3
	if( bike->R 		) BL_Data[ 5] 	|= 0x01;	//S4
	//if( bike->HallERR	) BL_Data[5] 	|= 0x08;	//S		电机霍尔故障
	//if( bike->WheelERR) BL_Data[5] 	|= 0x04;	//S		手把故障
	//if( bike->ECUERR 	) BL_Data[15]	|= 0x20;	//S 	电机控制器故障
	//if( bike->PhaseERR  ) BL_Data[12]	|= 0x80;	//S 	电机缺相故障
	if( bike->Braked	) BL_Data[12]	|= 0x80;	//S9 
	
	if( bike->Charge	) BL_Data[18]	|= 0x08;	//S10
	if		( bike->PlayMedia == PM_FM 	) BL_Data[16] |= 0x04;	//S16
	else if ( bike->PlayMedia == PM_USB ) BL_Data[16] |= 0x08;	//S14
	else if ( bike->PlayMedia == PM_BT	) BL_Data[16] |= 0x02;	//S15
	if( bike->Play		) BL_Data[15]	|= 0x80;	//S19
	else				  BL_Data[16]	|= 0x80;	//S20

	/***************************Battery Area Display**********************************/
	BL_Data[13] |= 0x80;	//Q1
	switch ( bike->BatStatus ){
    case 0:
		if ( flashflag > 5 ) BL_Data[13] &= ~0x80; //Q1
			break;
    case 1: BL_Data[19] |= 0x01;break;
    case 2: BL_Data[19] |= 0x05;break;
    case 3: BL_Data[19] |= 0x07;break;
    case 4: BL_Data[19] |= 0x0F;break;
    case 5: BL_Data[19] |= 0x8F;break;
    case 6: BL_Data[19] |= 0xAF;break;      
    case 7: BL_Data[19] |= 0xEF;break;
    case 8: BL_Data[19] |= 0xFF;break;          
    default:break; 
	}

	/***************************Temp Area Display**********************************/
	BL_Data[ 7] |= (SegDataTemp[abs(bike->Temperature/10)%10]>>4);
	BL_Data[ 8] |= (SegDataTemp[abs(bike->Temperature/10)%10]<<4);
	BL_Data[ 6] |= (SegDataTemp[abs(bike->Temperature/10)/10]>>4);       
	BL_Data[ 7] |= (SegDataTemp[abs(bike->Temperature/10)/10]<<4);  
	BL_Data[ 7] |= 0x08;	//S6
	if (bike->Temperature < 0)	
	BL_Data[ 6] |= 0x08;  //S5    
		
	/***************************Time Area Display**********************************/
	if ( bike->HasTimer )	{
		BL_Data[ 0] |= SegDataTime[bike->Hour	/10];
		BL_Data[ 1] |= SegDataTime[bike->Hour	%10];
		BL_Data[ 2] |= SegDataTime[bike->Minute	/10];
		BL_Data[ 3] |= SegDataTime[bike->Minute	%10];       
		if ( bike->time_set ){
			if ( flashflag > 5 ) {
				switch ( bike->time_pos ){
				case 0:BL_Data[ 0] &= 0x10; break;
				case 1:BL_Data[ 1] &= 0x10; break;
				case 2:BL_Data[ 2] &= 0x10; break;
				case 3:BL_Data[ 3] &= 0x10; break;
				default:break;		
				}
			}
			BL_Data[ 2] |= 0x10;	//S8 col
		} else if ( flashflag <= 5 ) BL_Data[ 2] |= 0x10;	//S8 col
	}
	
	/*************************** Voltage Display**********************************/
	BL_Data[17] |= (SegDataVoltage[(bike->Voltage/10	)%10]) >> 4;
	BL_Data[17] |= (SegDataVoltage[(bike->Voltage/10	)%10]) << 4;
	BL_Data[18] |= (SegDataVoltage[(bike->Voltage/100	)%10]) >> 4;
	BL_Data[18] |= (SegDataVoltage[(bike->Voltage/100	)%10]) << 4;
	BL_Data[17] |= 0x08;	//S11

	/*************************** Mile Display**********************************/  
	BL_Data[ 8] |= (SegDataMile[(bike->Mile      )%10]) & 0x0F;
	BL_Data[ 9] |= (SegDataMile[(bike->Mile      )%10]) & 0xF0; 
	BL_Data[ 9] |= (SegDataMile[(bike->Mile/10   )%10]) & 0x0F; 
	BL_Data[10] |= (SegDataMile[(bike->Mile/10   )%10]) & 0xF0;
	BL_Data[10] |= (SegDataMile[(bike->Mile/100  )%10]) & 0x0F; 
	BL_Data[11] |= (SegDataMile[(bike->Mile/100  )%10]) & 0xF0; 
	BL_Data[11] |= (SegDataMile[(bike->Mile/1000 )%10]) & 0x0F;
	BL_Data[12] |= (SegDataMile[(bike->Mile/1000 )%10]) & 0xF0;
	BL_Data[12] |= (SegDataMile[(bike->Mile/10000)%10]) & 0x0F;
	BL_Data[13] |= (SegDataMile[(bike->Mile/10000)%10]) & 0xF0;
	BL_Data[ 9] |= 0x80;	//S12

	/*************************** Speed Display**********************************/
	BL_Data[ 5] |= (SegDataSpeed[ bike->Speed		%10])>>4;
	BL_Data[ 6] |= (SegDataSpeed[ bike->Speed		%10])<<4;
	BL_Data[ 4] |= (SegDataSpeed[(bike->Speed/10)	%10])>>4; 
	BL_Data[ 5] |= (SegDataSpeed[(bike->Speed/10)	%10])<<4; 
	BL_Data[11] |= 0x80;	//S13
  
	/*************************** Mode Display**********************************/ 
	switch (bike->SpeedMode){
	case 1: BL_Data[ 4] |= 0x80;break;
	case 2: BL_Data[ 4] |= 0x20;break;
	case 3: BL_Data[ 4] |= 0x40;break;
	case 4: BL_Data[ 4] |= 0x10;break;
	default:BL_Data[ 4] &=~0xF0;break;
	}
	
	BL_Data[13] |= (SegDataNumber[(bike->Number		)%10]) & 0x0F;
	BL_Data[14] |= (SegDataNumber[(bike->Number		)%10]) & 0xF0;
	BL_Data[14] |= (SegDataNumber[(bike->Number/10	)%10]) & 0x0F;
	BL_Data[15] |= (SegDataNumber[(bike->Number/10	)%10]) & 0xF0;
	BL_Data[15] |= (SegDataNumber[(bike->Number/100	)%10]) & 0x0F;
	BL_Data[16] |= (SegDataNumber[(bike->Number/100	)%10]) & 0xF0;
	if ( bike->Number >= 1000  		) BL_Data[16] |= 0x01;	//S17
	if ( bike->PlayMedia == PM_FM 	) BL_Data[14] |= 0x80;	//S18

  
	BL_Write_Data(0,sizeof(BL_Data),BL_Data);
}


void Delay(unsigned long nCount)
{
  for(; nCount != 0; nCount--);
}

