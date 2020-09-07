#ifndef __DEBUG_CONSOLE_H__
#define __DEBUG_CONSOLE_H__

void CheckStatus(char* StringPtr, CyU3PReturnStatus_t Status);
void CyFxAppErrorHandler(char* StringPtr,CyU3PReturnStatus_t Status);
void UartCallback(CyU3PUartEvt_t Event, CyU3PUartError_t Error);
void dma_uart_cb(CyU3PDmaChannel *handle,CyU3PDmaCbType_t type,CyU3PDmaCBInput_t *input);
void Uart_ConsoleThread(uint32_t Value);
CyU3PReturnStatus_t InitializeDebugConsole(uint8_t TraceLevel);

#endif   /* _INCLUDED__ */
