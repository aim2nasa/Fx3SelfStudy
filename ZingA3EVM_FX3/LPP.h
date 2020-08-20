#ifndef _LPP_
#define _LPP_

extern CyU3PReturnStatus_t SetupGPIO(void);
extern CyU3PReturnStatus_t I2C_Init(void);
extern void WriteI2C_test(void);
extern void ReadI2C_test(void);
extern CyU3PReturnStatus_t I2C_Write(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data_pt, uint32_t data_len);
extern CyU3PReturnStatus_t I2C_Read(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data_pt, uint32_t data_len);




#endif   /* _INCLUDED__ */
