#include "gpio.h"
#include "cyu3gpio.h"
#include "cyu3error.h"

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
