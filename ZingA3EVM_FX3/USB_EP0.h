#ifndef __USBEP0_H__
#define __USBEP0_H__

#include "cyu3types.h"
#include "cyu3os.h"

#define EVT_EP0			(1 << 1)

CyU3PEvent Ep0Event;			//Event group used to signal the thread
uint32_t HostReqNum;
uint8_t HostRxData[128];
uint32_t HostRxData_idx;
uint8_t HostTxData[128];
uint32_t HostTxData_idx;

void USBEP0RxThread(uint32_t Value);
CyU3PReturnStatus_t USBEP0RxThread_Create(void);

#endif
