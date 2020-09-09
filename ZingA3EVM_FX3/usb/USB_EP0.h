#ifndef __USBEP0_H__
#define __USBEP0_H__

#include "cyu3types.h"
#include "cyu3os.h"

#define EVT_EP0			(1 << 1)

typedef struct UsbEp0Ctx_t{
	CyU3PEvent Event_;
	uint32_t HostReqNum_;
	uint8_t HostRxData_[128];
	uint32_t HostRxData_idx_;
	uint8_t HostTxData_[128];
	uint32_t HostTxData_idx_;
}UsbEp0Ctx_t;

UsbEp0Ctx_t UsbEp0Ctx;

void USBEP0RxThread(uint32_t Value);
CyU3PReturnStatus_t USBEP0RxThread_Create(void);

#endif
