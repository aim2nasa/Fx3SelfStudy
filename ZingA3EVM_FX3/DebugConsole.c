/*
 * DebugConsole.c
 *
 * I2C_DebugConsole.c
 *
 *  This module implements the DebugPrint portion of cyu3debug.c for an I2C-based console
 *	The LOG function is not implemented which makes this code simpler
 */

#include "Application.h"

//
static CyU3PQueue       Uart_DebugQueue;
static CyU3PDmaBuffer_t Uart_Queue[10];
static CyU3PThread      Uart_DebugThread;


CyBool_t glDebugTxEnabled = CyFalse;	// Set true once I can output messages to the Console
CyU3PDmaChannel glUARTtoCPU_Handle;		// Handle needed by Uart Callback routine
uint8_t glConsoleInBuffer[200];				// Buffer for user Console Input
uint32_t glConsoleInIndex=0;				// Index into ConsoleIn buffer



void UartCallback(CyU3PUartEvt_t Event, CyU3PUartError_t Error)
// Handle characters typed in by the developer
// Later we will respond to commands terminated with a <CR>
{
	if (Event == CY_U3P_UART_EVENT_RX_DONE)
    {
    }
}

void dma_uart_cb(
        CyU3PDmaChannel *handle,        /**< Handle to the DMA channel. */
        CyU3PDmaCbType_t type,          /**< The type of callback notification being generated. */
        CyU3PDmaCBInput_t *input        /**< Union that contains data related to the notification.*/
)
{
	CyU3PDmaBuffer_t ConsoleInDmaBuffer;

	uint8_t buf[16+1];
	uint8_t buf_idx=0;

    switch (type)
    {

        case CY_U3P_DMA_CB_PROD_EVENT:

        	CyU3PDmaChannelGetBuffer(&glUARTtoCPU_Handle, &ConsoleInDmaBuffer, CYU3P_NO_WAIT);
        	CyU3PDmaChannelDiscardBuffer(&glUARTtoCPU_Handle);

        	if(ConsoleInDmaBuffer.count > 0) {
        		memcpy(buf,ConsoleInDmaBuffer.buffer,ConsoleInDmaBuffer.count);
        		buf_idx = ConsoleInDmaBuffer.count;
				buf[buf_idx]=0;
#if DBG_LEVEL >= DBG_TYPE_UART
				CyU3PDebugPrint(4, "[serial/dma] %s\r\n", buf);
#endif

				CyU3PQueueSend(&Uart_DebugQueue, &ConsoleInDmaBuffer, CYU3P_NO_WAIT);
        	}

            break;

        default:
            break;
    }

}


void Uart_ConsoleThread(uint32_t Value)
{
    // Value passed to this thread is a Semaphore that thread should signal once it is ready process buffers
    CyU3PReturnStatus_t Status;
    CyU3PDmaBuffer_t FilledBuffer;


    // Tell InitDebug that the thread is ready for work
    Status = CyU3PSemaphorePut((CyU3PSemaphore*)Value);

    //
	uint8_t buf[16+1];
	uint8_t buf_idx=0;
	uint8_t* pInputChar;
	unsigned char CharCount;
	int i;
	unsigned char result_CR_LF = 0;
	unsigned char result_CR_LF_idx = 0;
    //
    while (1)
    {
    	Status = CyU3PQueueReceive(&Uart_DebugQueue, &FilledBuffer, 100);
        //CyU3PDebugPrint(4, "[heart beat]\r\n");

        CyU3PDmaChannelSetWrapUp(&glUARTtoCPU_Handle);
        CyU3PThreadSleep (100);

        if (Status == CY_U3P_SUCCESS)
        {
    		memcpy(buf,FilledBuffer.buffer,FilledBuffer.count);
    		buf_idx = FilledBuffer.count;
			buf[buf_idx]=0;
#if DBG_LEVEL >= DBG_TYPE_UART
			CyU3PDebugPrint(4, "[serial/queue] %s\r\n", buf);
#endif

			// buffering
			CharCount = buf_idx;
			pInputChar = buf;
			if (glConsoleInIndex+CharCount>=sizeof(glConsoleInBuffer)) {
				glConsoleInIndex = 0;
			}
			memcpy(glConsoleInBuffer+glConsoleInIndex,pInputChar,CharCount);
			glConsoleInIndex += CharCount;
			glConsoleInBuffer[glConsoleInIndex] = 0;

			// search CR LF
			for(i=0;i<glConsoleInIndex-1;i++) {
				if(glConsoleInBuffer[i] == 0x0d && glConsoleInBuffer[i+1] == 0x0a) {
					result_CR_LF = 1;
					result_CR_LF_idx = i;
				}
			}

			// packet parsing & processing
			if (result_CR_LF == 1)
			{
				glConsoleInBuffer[result_CR_LF_idx] = 0;
				glConsoleInBuffer[result_CR_LF_idx+1] = 0;

				if (strcmp("test1", (const char*)glConsoleInBuffer) == 0) {
					CyU3PDebugPrint(4, "test1\r\n");
				}
				else if (strcmp("reset", (const char*)glConsoleInBuffer) == 0)
				{
					CyU3PDebugPrint(4, "reset\r\n");
					CyU3PThreadSleep(100);
					CyU3PDeviceReset(CyFalse);
				}
				else CyU3PDebugPrint(4, "Unknown Command: '%s'\r\n", &glConsoleInBuffer[0]);

				memset(glConsoleInBuffer,0,glConsoleInIndex);
				glConsoleInIndex = 0;
			}

        }
    }
}

void InitializeDebugConsole(uint8_t TraceLevel)
{
	CyU3PUartConfig_t uartConfig;
	CyU3PDmaChannelConfig_t dmaConfig;
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
	uint32_t ret = CY_U3P_SUCCESS;
	CyU3PSemaphore ThreadSignal;
	void* StackPtr;

	apiRetStatus = CyU3PUartInit();		// Start the UART driver
	if (apiRetStatus != CY_U3P_SUCCESS)
		CyFxAppErrorHandler(apiRetStatus);

	CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
	uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
	uartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;
	uartConfig.parity   = CY_U3P_UART_NO_PARITY;
	uartConfig.txEnable = CyTrue;
	uartConfig.rxEnable = CyTrue;
	uartConfig.flowCtrl = CyFalse;
	uartConfig.isDma    = CyTrue;

	apiRetStatus = CyU3PUartSetConfig(&uartConfig, UartCallback);			// Configure the UART hardware
	if (apiRetStatus != CY_U3P_SUCCESS)
		CyFxAppErrorHandler(apiRetStatus);

	apiRetStatus = CyU3PUartTxSetBlockXfer(0xFFFFFFFF);						// Send as much data as I need to
	if (apiRetStatus != CY_U3P_SUCCESS)
		CyFxAppErrorHandler(apiRetStatus);

	apiRetStatus = CyU3PDebugInit(CY_U3P_LPP_SOCKET_UART_CONS, TraceLevel);	// Attach the Debug driver above the UART driver
	if (apiRetStatus == CY_U3P_SUCCESS)
		glDebugTxEnabled = CyTrue;
	else
		CyFxAppErrorHandler(apiRetStatus);

	CyU3PDebugPreamble(CyFalse);											// Skip preamble, debug info is targeted for a person

	// Now setup a DMA channel to receive characters from the Uart Rx
	apiRetStatus = CyU3PUartRxSetBlockXfer(0xFFFFFFFFU);
	if (apiRetStatus != CY_U3P_SUCCESS)
		CyFxAppErrorHandler(apiRetStatus);

	CyU3PMemSet((uint8_t *)&dmaConfig, 0, sizeof(dmaConfig));
	dmaConfig.size  		= 16;											// Minimum size allowed, I only need 1 byte
	dmaConfig.count 		= 10;											// I can't type faster than the Uart Callback routine!
	dmaConfig.prodSckId		= CY_U3P_LPP_SOCKET_UART_PROD;
	dmaConfig.consSckId 	= CY_U3P_CPU_SOCKET_CONS;
	dmaConfig.dmaMode 		= CY_U3P_DMA_MODE_BYTE;
	dmaConfig.notification	= CY_U3P_DMA_CB_PROD_EVENT;
	dmaConfig.cb 			= dma_uart_cb;
	dmaConfig.prodHeader = 0;
	dmaConfig.prodFooter = 0;
	dmaConfig.consHeader = 0;
	dmaConfig.prodAvailCount = 0;

	apiRetStatus = CyU3PDmaChannelCreate(&glUARTtoCPU_Handle, CY_U3P_DMA_TYPE_MANUAL_IN, &dmaConfig);
	if (apiRetStatus != CY_U3P_SUCCESS){
		CyU3PDebugPrint (4, "CyU3PDmaChannelCreate failed, Error code = %d\n", apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

	apiRetStatus = CyU3PDmaChannelSetXfer(&glUARTtoCPU_Handle, 0);
	if (apiRetStatus != CY_U3P_SUCCESS){
		CyU3PDebugPrint (4, "CyU3PDmaChannelSetXfer failed, Error code = %d\n", apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

	CyU3PThreadSleep (1000);
	CyU3PDmaChannelDiscardBuffer(&glUARTtoCPU_Handle);
	CyU3PThreadSleep (100);
	CyU3PDmaChannelReset(&glUARTtoCPU_Handle);

	// Create a Queue for the Debug_Console to use
	ret = CyU3PQueueCreate(&Uart_DebugQueue, sizeof (CyU3PDmaBuffer_t), Uart_Queue, sizeof(Uart_Queue));
	if (ret != CY_U3P_SUCCESS){
		CyU3PDebugPrint (4, "CyU3PQueueCreate failed, Error code = %d\n", apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

	// I need to create a thread that will manage the Queue
	// I also need a signal to let me know that this thread is running
	ret = CyU3PSemaphoreCreate(&ThreadSignal, 0);
	if (apiRetStatus != CY_U3P_SUCCESS){
		CyU3PDebugPrint (4, "CyU3PSemaphoreCreate failed, Error code = %d\n", apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

	StackPtr = CyU3PMemAlloc(DEBUG_THREAD_STACK_SIZE);
	ret = CyU3PThreadCreate(&Uart_DebugThread,          // Handle to my Application Thread
			"30:Uart_Debug_Thread",                     // Thread ID and name
			Uart_ConsoleThread,                         // Thread entry function
			(uint32_t)&ThreadSignal,                    // Parameter passed to Thread
			StackPtr,                                   // Pointer to the allocated thread stack
			DEBUG_THREAD_STACK_SIZE,                    // Allocated thread stack size
			DEBUG_THREAD_PRIORITY,                      // Thread priority
			DEBUG_THREAD_PRIORITY,                      // = Thread priority so no preemption
			CYU3P_NO_TIME_SLICE,                        // Time slice no supported
			CYU3P_AUTO_START                            // Start the thread immediately
			);

	// Wait for the thread to be set up
	ret = CyU3PSemaphoreGet(&ThreadSignal, CYU3P_WAIT_FOREVER);
	if (ret != CY_U3P_SUCCESS){
		CyU3PDebugPrint (4, "CyU3PSemaphoreGet failed, Error code = %d\n", apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}
}
