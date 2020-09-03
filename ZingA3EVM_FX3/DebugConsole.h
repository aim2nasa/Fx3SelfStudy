#ifndef _DBGCONSOLE_
#define _DBGCONSOLE_

extern void UartCallback(CyU3PUartEvt_t Event, CyU3PUartError_t Error);

extern void dma_uart_cb(
        CyU3PDmaChannel *handle,        /**< Handle to the DMA channel. */
        CyU3PDmaCbType_t type,          /**< The type of callback notification being generated. */
        CyU3PDmaCBInput_t *input        /**< Union that contains data related to the notification.*/
);

extern void Uart_ConsoleThread(uint32_t Value);
extern void InitializeDebugConsole(uint8_t TraceLevel);

#endif   /* _INCLUDED__ */
