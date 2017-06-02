#ifndef __BL55077_H__
#define __BL55077_H__

#define BL_ADDR               (uint8_t )(0x70)
#define BL_TIMEOUT            (uint16_t)(0x1000)

void BL55077_Config(unsigned char allon);
HAL_StatusTypeDef BL_Write_Data(unsigned char ADSTART,unsigned char LEN, unsigned char * p);

#endif
