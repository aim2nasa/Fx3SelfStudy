#include "Application.h"
#include "gpio\gpio.h"
#include "cyu3error.h"
#include "cyfx3_api.h"
#include "cyu3system.h"
#include "AppThread.h"

CyU3PThread ApplicationThreadHandle;

void CyFxApplicationDefine(void)
{
    void *StackPtr = NULL;
    uint32_t Status;

    StackPtr = CyU3PMemAlloc(APPLICATION_THREAD_STACK);
    Status = CyU3PThreadCreate(&ApplicationThreadHandle,	// Handle to my Application Thread
            "11:Step1",										// Thread ID and name
            ApplicationThread,								// Thread entry function
            1,												// Parameter passed to Thread
            StackPtr,										// Pointer to the allocated thread stack
            APPLICATION_THREAD_STACK,						// Allocated thread stack size
            APPLICATION_THREAD_PRIORITY,					// Thread priority
            APPLICATION_THREAD_PRIORITY,					// Thread priority so no preemption
            CYU3P_NO_TIME_SLICE,							// Time slice no supported
            CYU3P_AUTO_START								// Start the thread immediately
            );
    if (Status != CY_U3P_SUCCESS)
    {
        /* Thread creation failed with the Status = error code */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue. Loop indefinitely */
        while(1);
    }
}

int main (void)
{
    CyU3PIoMatrixConfig_t io_cfg;
    CyU3PReturnStatus_t Status = CY_U3P_SUCCESS;
    CyU3PSysClockConfig_t clkCfg;

	/* setSysClk400 clock configurations */
	clkCfg.setSysClk400 = CyTrue;   /* FX3 device's master clock is set to a frequency > 400 MHz */
	clkCfg.cpuClkDiv = 2;           /* CPU clock divider */
	clkCfg.dmaClkDiv = 2;           /* DMA clock divider */
	clkCfg.mmioClkDiv = 2;          /* MMIO clock divider */
	clkCfg.useStandbyClk = CyFalse; /* device has no 32KHz clock supplied */
	clkCfg.clkSrc = CY_U3P_SYS_CLK; /* Clock source for a peripheral block  */
    Status = CyU3PDeviceInit(&clkCfg);

    if (Status == CY_U3P_SUCCESS)
    {
		Status = CyU3PDeviceCacheControl(CyTrue, CyFalse, CyFalse);
		if (Status == CY_U3P_SUCCESS)
		{
			CyU3PMemSet((uint8_t *)&io_cfg, 0, sizeof(io_cfg));
			io_cfg.isDQ32Bit = CyTrue;
			io_cfg.useUart   = CyTrue;
			io_cfg.useI2C    = CyTrue;
			io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_DEFAULT;
			io_cfg.gpioSimpleEn[0]  = 0;
			io_cfg.gpioSimpleEn[1]  = 1<<(GPIO57-32); // TP2 in schematic
			Status = CyU3PDeviceConfigureIOMatrix(&io_cfg);
			if (Status == CY_U3P_SUCCESS) CyU3PKernelEntry();	// Start RTOS, this does not return
		}
	}
    // Get here on a failure, can't recover, just hang here
    while (1);
	return 0;	// Won't get here but compiler wants this!
}


