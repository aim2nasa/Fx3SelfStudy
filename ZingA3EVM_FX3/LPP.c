#include "Application.h"

CyU3PReturnStatus_t SetupGPIO(void)
{
	CyU3PReturnStatus_t Status;
	CyU3PGpioClock_t GpioClock;
	CyU3PGpioSimpleConfig_t GpioConfig;

	// Startup the GPIO module
	GpioClock.fastClkDiv = 2;
	GpioClock.slowClkDiv = 0;
	GpioClock.simpleDiv = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
	GpioClock.clkSrc = CY_U3P_SYS_CLK;
	GpioClock.halfDiv = 0;
	Status = CyU3PGpioInit(&GpioClock, 0);
	if(Status!=CY_U3P_SUCCESS) return Status;

	// GPIO57 - TP2
	Status = CyU3PDeviceGpioOverride(GPIO57, CyTrue);
	if(Status!=CY_U3P_SUCCESS) return Status;

	CyU3PMemSet((uint8_t *)&GpioConfig, 0, sizeof(GpioConfig));
	GpioConfig.outValue = CyFalse;
	GpioConfig.driveLowEn = CyTrue;
	GpioConfig.driveHighEn = CyTrue;
	Status = CyU3PGpioSetSimpleConfig(GPIO57, &GpioConfig);
	if(Status!=CY_U3P_SUCCESS) return Status;

	// GPIF Bus Width
	Status = CyU3PDeviceGpioOverride(GPIF_BUSWIDTH_CTL0, CyTrue);
	if(Status!=CY_U3P_SUCCESS) return Status;

	CyU3PMemSet((uint8_t *)&GpioConfig, 0, sizeof(GpioConfig));
	GpioConfig.outValue = CyFalse;
	GpioConfig.driveLowEn = CyTrue;
	GpioConfig.driveHighEn = CyTrue;
	Status = CyU3PGpioSetSimpleConfig(GPIF_BUSWIDTH_CTL0, &GpioConfig);
	if(Status!=CY_U3P_SUCCESS) return Status;

	Status = CyU3PDeviceGpioOverride(GPIF_BUSWIDTH_CTL1, CyTrue);
	if(Status!=CY_U3P_SUCCESS) return Status;

	CyU3PMemSet((uint8_t *)&GpioConfig, 0, sizeof(GpioConfig));
	GpioConfig.outValue = CyFalse;
	GpioConfig.driveLowEn = CyTrue;
	GpioConfig.driveHighEn = CyTrue;
	return CyU3PGpioSetSimpleConfig(GPIF_BUSWIDTH_CTL1, &GpioConfig);
}

CyU3PReturnStatus_t I2C_Init(void)
{
	CyU3PI2cConfig_t i2cConfig;
	CyU3PReturnStatus_t Status;

	Status = CyU3PI2cInit();	// Start the I2C driver
	if(Status!=CY_U3P_SUCCESS) return Status;

	i2cConfig.bitRate    = CY_FX_USBI2C_I2C_BITRATE;
	i2cConfig.busTimeout = 0xFFFFFFFF;
	i2cConfig.dmaTimeout = 0xFFFF;
	i2cConfig.isDma      = CyFalse;
	return CyU3PI2cSetConfig(&i2cConfig, NULL);
}

void WriteI2C_test(void)
{
	uint8_t   buffer[] = {0x77,0x07,0x00,0x00};

	CyU3PI2cPreamble_t preamble;
	preamble.length    = 2;
	preamble.buffer[0] = 0x08<<1;
	preamble.buffer[1] = 0x00;
	preamble.ctrlMask  = 0x0000;

	CyU3PI2cTransmitBytes(&preamble, buffer, sizeof(buffer), 0);
}

void ReadI2C_test(void)
{
	CyU3PI2cPreamble_t preamble;
	uint8_t   buffer[20] = {0,};
	preamble.length    = 1;
	preamble.buffer[0] = (0x08<<1) | 1;
	preamble.ctrlMask  = 0x0000;

	CyU3PI2cReceiveBytes(&preamble, buffer, 4, 0);
}

CyU3PReturnStatus_t I2C_Write(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data_pt, uint32_t data_len)
{
	CyU3PI2cPreamble_t preamble;
	preamble.length    = 2;
	preamble.buffer[0] = dev_addr<<1;
	preamble.buffer[1] = reg_addr;
	preamble.ctrlMask  = 0x0000;

	return CyU3PI2cTransmitBytes(&preamble, data_pt, data_len, 0);
}

CyU3PReturnStatus_t I2C_Read(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data_pt, uint32_t data_len)
{
	CyU3PI2cPreamble_t preamble;
	preamble.length    = 3;
	preamble.buffer[0] = dev_addr<<1;
	preamble.buffer[1] = reg_addr;
	preamble.buffer[2] = ((dev_addr<<1) | 0x01);
	preamble.ctrlMask  = 0x0002;	//start after 2nd byte (bit position)

	return CyU3PI2cReceiveBytes(&preamble, data_pt, data_len, 0);
}
