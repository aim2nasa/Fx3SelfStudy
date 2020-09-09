#ifndef __USB_HANDLER_H__
#define __USB_HANDLER_H__

#include "cyu3usb.h"

CyBool_t USBSetupCB(uint32_t setupdat0, uint32_t setupdat1);
void USBEventCB(CyU3PUsbEventType_t evtype, uint16_t evdata);
CyBool_t USBLPMRqtCB(CyU3PUsbLinkPowerMode link_mode);
CyU3PReturnStatus_t USB_Init(void);
CyU3PReturnStatus_t USB_Connect(void);

#endif
