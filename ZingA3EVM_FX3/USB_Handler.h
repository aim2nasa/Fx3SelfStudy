
#ifndef _USB_HANDLER_
#define _USB_HANDLER_

extern CyU3PReturnStatus_t USBEP0RxThread_Create(void);
extern CyBool_t
USBSetupCB (
        uint32_t setupdat0, /* SETUP Data 0 */
        uint32_t setupdat1  /* SETUP Data 1 */
    );
extern void
USBEventCB (
    CyU3PUsbEventType_t evtype, /* Event type */
    uint16_t            evdata  /* Event data */
    );
extern CyBool_t
USBLPMRqtCB (
        CyU3PUsbLinkPowerMode link_mode);
extern CyU3PReturnStatus_t USB_Init(void);

extern CyU3PReturnStatus_t USB_Connect(void);








#endif   /* _INCLUDED__ */
