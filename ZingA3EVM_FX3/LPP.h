#ifndef __LPP_H__
#define __LPP_H__

#include "cyu3types.h"

//GPIO
#define GPIO57						(57)
#define GPIF_BUSWIDTH_CTL0			(50)    // 50, 38
#define GPIF_BUSWIDTH_CTL1			(51)    // 51, 37

// I2C
#define CY_FX_USBI2C_I2C_BITRATE	(100000)

CyU3PReturnStatus_t SetupGPIO(void);
CyU3PReturnStatus_t I2C_Init(void);
void WriteI2C_test(void);
void ReadI2C_test(void);
CyU3PReturnStatus_t I2C_Write(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data_pt, uint32_t data_len);
CyU3PReturnStatus_t I2C_Read(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data_pt, uint32_t data_len);

#endif
