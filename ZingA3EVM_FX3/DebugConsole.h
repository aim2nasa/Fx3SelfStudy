#ifndef __DEBUG_CONSOLE_H__
#define __DEBUG_CONSOLE_H__

#include "cyu3types.h"
#include "cyu3uart.h"

void CheckStatus(char* StringPtr, CyU3PReturnStatus_t Status);
void CyFxAppErrorHandler(char* StringPtr,CyU3PReturnStatus_t Status);
void UartCallback(CyU3PUartEvt_t Event, CyU3PUartError_t Error);
CyU3PReturnStatus_t InitializeDebugConsole(uint8_t TraceLevel);

#endif
