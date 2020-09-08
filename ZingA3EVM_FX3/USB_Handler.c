#include "Application.h"
#include "DebugConsole.h"
#include "USB_Handler.h"
#include "gpio.h"
#include "cyu3error.h"
#include "cyu3utils.h"
#include "cyu3gpio.h"
#include "dma.h"
#include "USB_EP0.h"

/* Extern definitions for the USB Descriptors */
extern const uint8_t CyFxUSB20DeviceDscr[];
extern const uint8_t CyFxUSB30DeviceDscr[];
extern const uint8_t CyFxUSBDeviceQualDscr[];
extern const uint8_t CyFxUSBFSConfigDscr[];
extern const uint8_t CyFxUSBHSConfigDscr[];
extern const uint8_t CyFxUSBBOSDscr[];
extern const uint8_t CyFxUSBSSConfigDscr[];
extern const uint8_t CyFxUSBStringLangIDDscr[];
extern const uint8_t CyFxUSBManufactureDscr[];
extern const uint8_t CyFxUSBProductDscr[];
extern const uint8_t CyFxUSBSerialNumDscr[];

// setup data
#define STANDARD_REQUEST	(0)			// My values are not shifted
#define CLASS_REQUEST		(1)
#define VENDOR_REQUEST		(2)

/* Callback to handle the USB setup requests. */
CyBool_t USBSetupCB(uint32_t setupdat0,uint32_t setupdat1)
{
    static uint8_t buffer[128] = {0,};
    uint16_t readcount;

	CyBool_t isHandled = CyFalse;
	union { uint32_t SetupData[2];
			uint8_t RawBytes[8];
			struct { uint8_t Target:5; uint8_t Type:2; uint8_t Direction:1;
				 	 uint8_t Request; uint16_t Value; uint16_t Index; uint16_t Length; };
		  } Setup;
	// Copy the incoming Setup Packet into my Setup union which will "unpack" the variables
	Setup.SetupData[0] = setupdat0;
	Setup.SetupData[1] = setupdat1;

    if (Setup.Type == STANDARD_REQUEST)
    {
        /* Handle SET_FEATURE(FUNCTION_SUSPEND) and CLEAR_FEATURE(FUNCTION_SUSPEND)
         * requests here. It should be allowed to pass if the device is in configured
         * state and failed otherwise. */
        if ((Setup.Target == CY_U3P_USB_TARGET_INTF) && ((Setup.Request == CY_U3P_USB_SC_SET_FEATURE)
                    || (Setup.Request == CY_U3P_USB_SC_CLEAR_FEATURE)) && (Setup.Value == 0))
        {
            if (IsApplnActive)
                CyU3PUsbAckSetup ();
            else
            	CyU3PUsbStall(0, CyTrue, CyFalse);

            isHandled = CyTrue;
        }

        /* CLEAR_FEATURE request for endpoint is always passed to the setup callback
		 * regardless of the enumeration model used. When a clear feature is received,
		 * the previous transfer has to be flushed and cleaned up. This is done at the
		 * protocol level. Since this is just a loopback operation, there is no higher
		 * level protocol. So flush the EP memory and reset the DMA channel associated
		 * with it. If there are more than one EP associated with the channel reset both
		 * the EPs. The endpoint stall and toggle / sequence number is also expected to be
		 * reset. Return CyFalse to make the library clear the stall and reset the endpoint
		 * toggle. Or invoke the CyU3PUsbStall (ep, CyFalse, CyTrue) and return CyTrue.
		 * Here we are clearing the stall. */
		if ((Setup.Target == CY_U3P_USB_TARGET_ENDPT) && (Setup.Request == CY_U3P_USB_SC_CLEAR_FEATURE)
				&& (Setup.Value == CY_U3P_USBX_FS_EP_HALT))
		{
            if (IsApplnActive)
            {
                if (Setup.Index == CY_FX_EP_PRODUCER)
                {
                    CyU3PUsbSetEpNak (CY_FX_EP_PRODUCER, CyTrue);
                    CyU3PBusyWait (125);

                    CyU3PDmaChannelReset (&Dma.ControlOut_);
                    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
                    CyU3PUsbResetEp (CY_FX_EP_PRODUCER);
                    CyU3PDmaChannelSetXfer (&Dma.ControlOut_, 0);

                    CyU3PUsbSetEpNak (CY_FX_EP_PRODUCER, CyFalse);
                }
                else if (Setup.Index == CY_FX_EP_CONSUMER)
                {
                    CyU3PUsbSetEpNak (CY_FX_EP_CONSUMER, CyTrue);
                    CyU3PBusyWait (125);

                    CyU3PDmaChannelReset (&Dma.ControlIn_);
                    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);
                    CyU3PUsbResetEp (CY_FX_EP_CONSUMER);
                    CyU3PDmaChannelSetXfer (&Dma.ControlIn_, 0);

                    CyU3PUsbSetEpNak (CY_FX_EP_CONSUMER, CyFalse);
                }
                else if (Setup.Index == CY_FX_EP_PRODUCER_2)
                {
                    CyU3PUsbSetEpNak (CY_FX_EP_PRODUCER_2, CyTrue);
                    CyU3PBusyWait (125);

                    CyU3PDmaChannelReset (&Dma.DataOut_);
                    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER_2);
                    CyU3PUsbResetEp (CY_FX_EP_PRODUCER_2);
                    CyU3PDmaChannelSetXfer (&Dma.DataOut_, 0);

                    CyU3PUsbSetEpNak (CY_FX_EP_PRODUCER_2, CyFalse);
                }
                else if (Setup.Index == CY_FX_EP_CONSUMER_2)
                {
                    CyU3PUsbSetEpNak (CY_FX_EP_CONSUMER_2, CyTrue);
                    CyU3PBusyWait (125);

                    CyU3PDmaChannelReset (&Dma.DataIn_);
                    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER_2);
                    CyU3PUsbResetEp (CY_FX_EP_CONSUMER_2);
                    CyU3PDmaChannelSetXfer (&Dma.DataIn_, 0);

                    CyU3PUsbSetEpNak (CY_FX_EP_CONSUMER_2, CyFalse);
                }

                CyU3PUsbStall (Setup.Index, CyFalse, CyTrue);

                CyU3PUsbAckSetup ();
                isHandled = CyTrue;
            }
		}

    }
    else if (Setup.Type == VENDOR_REQUEST)
    {
    	if(Setup.Request > 100) {

    	}
    	else if(Setup.Request == 1) { // buffer write/ read command
    		if (Setup.Direction == 0) { // host to dev
    	        CyU3PUsbGetEP0Data(sizeof (buffer), (uint8_t *)buffer, &readcount);
    	        CyU3PUsbFlushEp(0);
    		}
    		else { // dev to host
    			readcount = Setup.Length;
    	        CyU3PUsbSendEP0Data(readcount,(uint8_t *)buffer);
    		}
    	}
    	else if(Setup.Request == 2) { // fx3 reset command
    		CyU3PDeviceReset(CyFalse);
    	}
    	else if(Setup.Request >= 3 && Setup.Request <= 100) {
        	if (Setup.Direction == 0) { // host to dev, store data
        	    CyU3PUsbGetEP0Data(sizeof (buffer), (uint8_t *)buffer, &readcount);
        	    CyU3PUsbFlushEp(0);
        	    memcpy(UsbEp0Ctx.HostRxData_,buffer,readcount);
        	    UsbEp0Ctx.HostRxData_idx_ = readcount;

        	    UsbEp0Ctx.HostReqNum_ = Setup.Request;
        		CyU3PEventSet (&UsbEp0Ctx.Event_, EVT_EP0, CYU3P_EVENT_OR);
        	}
        	else { // dev to host
    			if(Setup.Length==UsbEp0Ctx.HostTxData_idx_)
    				CyU3PUsbSendEP0Data(UsbEp0Ctx.HostTxData_idx_,(uint8_t *)UsbEp0Ctx.HostTxData_);
        	}
    	}

//    	CyU3PUsbAckSetup ();
//    	isHandled = CyTrue;
    }

    return isHandled;
}

/* This is the callback function to handle the USB events. */
void USBEventCB(CyU3PUsbEventType_t evtype,uint16_t evdata)
{
    switch (evtype)
    {
        case CY_U3P_USB_EVENT_SETCONF:
            /* Stop the application before re-starting. */
            if (IsApplnActive)
            {
                AppStop ();
            }
            /* Start the app */
            AppStart ();
            break;

        case CY_U3P_USB_EVENT_RESET:
        case CY_U3P_USB_EVENT_DISCONNECT:
            /* Stop the app */
            if (IsApplnActive)
            {
            	AppStop ();
            }
            break;

        case CY_U3P_USB_EVENT_SUSPEND:
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
            CyU3PDebugPrint (4, "Suspend is not supported\n\r");
#endif
            break;

        default:
            break;
    }

}

/* Callback function to handle LPM requests from the USB 3.0 host. This function is invoked by the API
   whenever a state change from U0 -> U1 or U0 -> U2 happens. If we return CyTrue from this function, the
   FX3 device is retained in the low power state. If we return CyFalse, the FX3 device immediately tries
   to trigger an exit back to U0.

   This application does not have any state in which we should not allow U1/U2 transitions; and therefore
   the function always return CyTrue.
   Changed into CyFalse , always active
 */
CyBool_t USBLPMRqtCB(CyU3PUsbLinkPowerMode link_mode)
{
    return CyFalse;
}

CyU3PReturnStatus_t USB_Init(void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Start the USB functionality. */
    apiRetStatus = CyU3PUsbStart();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler("CyU3PUsbStart",apiRetStatus);
    }

    /* The fast enumeration is the easiest way to setup a USB connection,
     * where all enumeration phase is handled by the library. Only the
     * class / vendor requests need to be handled by the application. */
    CyU3PUsbRegisterSetupCallback(USBSetupCB, CyTrue);

    /* Setup the callback to handle the USB events. */
    CyU3PUsbRegisterEventCallback(USBEventCB);

    /* Register a callback to handle LPM requests from the USB 3.0 host. */
    CyU3PUsbRegisterLPMRequestCallback(USBLPMRqtCB);

    /* Set the USB Enumeration descriptors */

    /* Super speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSB30DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler("CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR)",apiRetStatus);
    }

    /* High speed device descriptor. */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, 0, (uint8_t *)CyFxUSB20DeviceDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxAppErrorHandler("CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR)",apiRetStatus);
    }

    /* BOS descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, 0, (uint8_t *)CyFxUSBBOSDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxAppErrorHandler("CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR)",apiRetStatus);
    }

    /* Device qualifier descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, 0, (uint8_t *)CyFxUSBDeviceQualDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxAppErrorHandler("CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR)",apiRetStatus);
    }

    /* Super speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBSSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxAppErrorHandler("CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR)",apiRetStatus);
    }

    /* High speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBHSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxAppErrorHandler("CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR)",apiRetStatus);
    }

    /* Full speed configuration descriptor */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, 0, (uint8_t *)CyFxUSBFSConfigDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxAppErrorHandler("CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR)",apiRetStatus);
    }

    /* String descriptor 0 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxAppErrorHandler("CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR)",apiRetStatus);
    }

    /* String descriptor 1 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxAppErrorHandler("CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR)",apiRetStatus);
    }

    /* String descriptor 2 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxAppErrorHandler("CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR)",apiRetStatus);
    }

    /* String descriptor 3 */
    apiRetStatus = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 3, (uint8_t *)CyFxUSBSerialNumDscr);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
    	CyFxAppErrorHandler("CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR)",apiRetStatus);
    }
    return apiRetStatus;
}

CyU3PReturnStatus_t USB_Connect(void)
{
	 CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

	// type-c connector
	/* Check the enumeration MuxControl_GPIO to High */
	apiRetStatus = CyU3PConnectState(CyTrue, CyTrue);
	if(CyU3PUsbGetSpeed() != CY_U3P_SUPER_SPEED)
	{
	  apiRetStatus = CyU3PConnectState(CyFalse, CyFalse);
	  CyU3PDebugPrint(4, "First attempt failed = %d\n", apiRetStatus);

		/* Check in other orientation */
	  CyU3PGpioSetValue(GPIO57, CyTrue);
	  apiRetStatus = CyU3PUsbControlUsb2Support (CyTrue);
	  apiRetStatus = CyU3PConnectState(CyTrue, CyTrue);
	  if (apiRetStatus != CY_U3P_SUCCESS)
	  {
		  CyU3PDebugPrint(4, "USB Connect failed, Error code = %d\n", apiRetStatus);
	  }
	}

	return apiRetStatus;
}
