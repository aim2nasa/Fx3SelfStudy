// Keyboard.c - demonstrate USB by enumerating as a keyboard
//
// john@usb-by-example.com
//

#include "Application.h"



// app thread, flag
CyU3PThread ApplicationThreadHandle;	// Handle to my Application Thread
CyBool_t glIsApplnActive = CyFalse;      /* Whether the application is active or not. */

// DMA ch
CyU3PDmaChannel glDMAControlOut;   /* DMA channel : USB to GPIF, Control OUT */
CyU3PDmaChannel glDMAControlIn;    /* DMA channel : USB to GPIF, Control IN */
CyU3PDmaChannel glDMADataOut;      /* DMA channel : USB to GPIF, Data OUT */
CyU3PDmaChannel glDMADataIn;       /* DMA channel : USB to GPIF, Data IN */

dma_mode_t glDMA_mode = DMA_NORMAL;

// Ep0 vendor ch
CyU3PEvent      glEp0Event;                 /* Event group used to signal the thread. */
uint32_t glHostReqNum;
uint8_t glHostRxData[128];
uint32_t glHostRxData_idx;
uint8_t glHostTxData[128];
uint32_t glHostTxData_idx;

// monitoring
uint32_t glDataOutInjected=0;
uint32_t glDataInInjected=0;
uint32_t glControlOutInjected=0;
uint32_t glControlInInjected=0;

// zing control ch thread
CyU3PThread ControlChThreadHandle;	// Handle to my Application Thread
CyU3PEvent      glControlChEvent;
uint8_t glControlChData[512];
uint32_t glControlChData_idx;

uint32_t glMngtData;




/* App Error Handler */
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    /* API return status */
        )
{
    /* Application failed with the error code apiRetStatus */

    /* Add custom debug or recovery actions here */

    /* Loop Indefinitely */
    for (;;)
    {
        /* Thread sleep : 100 ms */
        CyU3PThreadSleep (100);

        CyU3PDebugPrint(4,"[ErrorHandler] Help me\r\n");
    }
}

void setDmaChannelCfg(CyU3PDmaChannelConfig_t *pDmaCfg, uint16_t size, uint16_t count, CyU3PDmaSocketId_t prodSckId,
		CyU3PDmaSocketId_t consSckId, uint32_t notification, CyU3PDmaCallback_t cb)
{
	pDmaCfg->size  = size;
	pDmaCfg->count = count;
	pDmaCfg->prodSckId = prodSckId;
	pDmaCfg->consSckId = consSckId;
	pDmaCfg->dmaMode = CY_U3P_DMA_MODE_BYTE;
	pDmaCfg->notification = notification;
	pDmaCfg->cb = cb;
	pDmaCfg->prodHeader = 0;
	pDmaCfg->prodFooter = 0;
	pDmaCfg->consHeader = 0;
	pDmaCfg->prodAvailCount = 0;
}

void createChannel(const char* name,
		CyU3PDmaChannelConfig_t *pDmaCfg,uint16_t size,uint16_t count,CyU3PDmaSocketId_t prodSckId,
		CyU3PDmaSocketId_t consSckId,uint32_t notification,CyU3PDmaCallback_t cb,
		CyU3PDmaChannel *handle,CyU3PDmaType_t type)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

	if(strcmp(name,"DmaNormal.DataIn")==0) {
#if PACKET_SUSPEND == 1
		notification |= CY_U3P_DMA_CB_PROD_SUSP | CY_U3P_DMA_CB_CONS_SUSP;
#endif
	}

	setDmaChannelCfg(pDmaCfg, size, count, prodSckId, consSckId, notification, cb);
	apiRetStatus = CyU3PDmaChannelCreate (handle, type, pDmaCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "(%s) CyU3PDmaChannelCreate failed, Error code = 0x%x\n", name,apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
	}

	if(strcmp(name,"DmaNormal.DataIn")==0) {
#if PACKET_SUSPEND == 1
		CyU3PDmaChannelSetSuspend(&handle,CY_U3P_DMA_SCK_SUSP_EOP,CY_U3P_DMA_SCK_SUSP_EOP);
#endif
	}

    /* Set DMA Channel transfer size to INFINITE */
    apiRetStatus = CyU3PDmaChannelSetXfer (handle, 0);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
		CyU3PDebugPrint (4, "(%s) CyU3PDmaChannelSetXfer failed, Error code = 0x%x\n", name,apiRetStatus);
		CyFxAppErrorHandler(apiRetStatus);
    }
}

void channelReset()
{
	//Abort & destroy DMAs
	CyU3PDmaChannelAbort(&glDMAControlOut);
	CyU3PDmaChannelAbort(&glDMAControlIn);
	CyU3PDmaChannelAbort(&glDMADataOut);
	CyU3PDmaChannelAbort(&glDMADataIn);

	CyU3PDmaChannelDestroy(&glDMAControlOut);
	CyU3PDmaChannelDestroy(&glDMAControlIn);
	CyU3PDmaChannelDestroy(&glDMADataOut);
	CyU3PDmaChannelDestroy(&glDMADataIn);

	//Flush the Endpoint memory
	CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
	CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);
	CyU3PUsbFlushEp(CY_FX_EP_PRODUCER_2);
	CyU3PUsbFlushEp(CY_FX_EP_CONSUMER_2);
}

void DMA_Sync_mode(void)
{
	CyU3PDebugPrint(4,"DMA_Sync_mode start=%d\r\n", glDMA_mode);

	CyU3PDmaChannelConfig_t dmaCfg;
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
	uint16_t size = 0;

	channelReset();

	// reconfig DMAs

	size = 1024; // super speed <- assumed condition , temporary code

	/* Control OUT channel
	 * Create a DMA Auto Channel between two sockets of the U port and P port
	 * DMA size is set based on the USB speed. */
	createChannel("DmaSync.ControlOut",
			&dmaCfg,size,8,CY_U3P_CPU_SOCKET_PROD,CY_U3P_PIB_SOCKET_0,CY_U3P_DMA_CB_PROD_EVENT,0,
			&glDMAControlOut,CY_U3P_DMA_TYPE_MANUAL_OUT);

	/* Control IN channel
	 * Create a DMA Auto Channel between two sockets of the U port and P port
	 * DMA size is set based on the USB speed. */
	createChannel("DmaSync.ControlIn",
			&dmaCfg,size,8,CY_U3P_PIB_SOCKET_1,CY_U3P_CPU_SOCKET_CONS,CY_U3P_DMA_CB_PROD_EVENT,0,
			&glDMAControlIn,CY_U3P_DMA_TYPE_MANUAL_IN);

	/* Data OUT Channel
	 * Create a DMA Auto channel for U Port -> P Port transfer. */
	createChannel("DmaSync.DataOut",
			&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_CPU_SOCKET_PROD,CY_U3P_PIB_SOCKET_2,CY_U3P_DMA_CB_PROD_EVENT,0,
			&glDMADataOut,CY_U3P_DMA_TYPE_MANUAL_OUT);

    /* Data IN Channel
    	 * Create a DMA Auto channel for U Port -> P Port transfer. */
	createChannel("DmaSync.DataIn",
			&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_PIB_SOCKET_3,CY_U3P_CPU_SOCKET_CONS,CY_U3P_DMA_CB_PROD_EVENT,0,
			&glDMADataIn,CY_U3P_DMA_TYPE_MANUAL_IN);

	glDMA_mode = DMA_SYNC;
	CyU3PDebugPrint(4,"DMA_Sync_mode end=%d\r\n", glDMA_mode);
}

void
DMA_Normal_CtrlOut_Cb (
        CyU3PDmaChannel     *handle,
        CyU3PDmaCbType_t     evtype,
        CyU3PDmaCBInput_t   *input)
{
    switch (evtype)
    {
        case CY_U3P_DMA_CB_PROD_EVENT:
        	glControlOutInjected++;
//            CyU3PDmaChannelDiscardBuffer (handle);
            break;

        case CY_U3P_DMA_CB_SEND_CPLT:	/* override mode */
            break;

        case CY_U3P_DMA_CB_CONS_EVENT:	/* normal mode */
            break;

        default:
            break;
    }
}

void
DMA_Normal_CtrlIn_Cb (
        CyU3PDmaChannel     *handle,
        CyU3PDmaCbType_t     evtype,
        CyU3PDmaCBInput_t   *input)
{
    switch (evtype)
    {
        case CY_U3P_DMA_CB_PROD_EVENT:
        	glControlInInjected++;
//            CyU3PDmaChannelDiscardBuffer (handle);
            break;

        case CY_U3P_DMA_CB_SEND_CPLT:	/* override mode */
            break;

        case CY_U3P_DMA_CB_CONS_EVENT:	/* normal mode */
            break;

        default:
            break;
    }
}

void
DMA_Normal_DataOut_Cb (
        CyU3PDmaChannel     *handle,
        CyU3PDmaCbType_t     evtype,
        CyU3PDmaCBInput_t   *input)
{
    switch (evtype)
    {
        case CY_U3P_DMA_CB_PROD_EVENT:
        	glDataOutInjected++;
//            CyU3PDmaChannelDiscardBuffer (handle);
            break;

        case CY_U3P_DMA_CB_SEND_CPLT:	/* override mode */
            break;

        case CY_U3P_DMA_CB_CONS_EVENT:	/* normal mode */
            break;

        default:
            break;
    }
}

void
DMA_Normal_DataIn_Cb (
        CyU3PDmaChannel     *handle,
        CyU3PDmaCbType_t     evtype,
        CyU3PDmaCBInput_t   *input)
{
    switch (evtype)
    {
        case CY_U3P_DMA_CB_PROD_EVENT:
        	glDataInInjected++;
//            CyU3PDmaChannelDiscardBuffer (handle);
            break;

        case CY_U3P_DMA_CB_SEND_CPLT:	/* override mode */
            break;

        case CY_U3P_DMA_CB_CONS_EVENT:	/* normal mode */
            break;

#if PACKET_SUSPEND == 1
	   case CY_U3P_DMA_CB_PROD_SUSP:
	   CyU3PDmaChannelResume(handle,CyTrue,CyTrue);
		   break;

	   case CY_U3P_DMA_CB_CONS_SUSP:
	   CyU3PDmaChannelResume(handle,CyTrue,CyTrue);
		   break;
#endif

        default:
            break;
    }
}

void DMA_Normal_mode(void)
{
	CyU3PDebugPrint(4,"DMA_Normal_mode start=%d\r\n", glDMA_mode);
	CyU3PDmaChannelConfig_t dmaCfg;
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
	uint16_t size = 0;

	channelReset();

	// reconfig DMAs

	size = 1024; // super speed <- assumed condition , temporary code

	/* Control OUT channel
	 * Create a DMA Auto Channel between two sockets of the U port and P port
	 * DMA size is set based on the USB speed. */
	createChannel("DmaNormal.ControlOut",
			&dmaCfg,size,8,CY_U3P_UIB_SOCKET_PROD_1,CY_U3P_PIB_SOCKET_0,CY_U3P_DMA_CB_PROD_EVENT,DMA_Normal_CtrlOut_Cb,
			&glDMAControlOut,CY_U3P_DMA_TYPE_AUTO_SIGNAL);

	/* Control IN channel
	 * Create a DMA Auto Channel between two sockets of the U port and P port
	 * DMA size is set based on the USB speed. */
	createChannel("DmaNormal.ControlIn",
			&dmaCfg,size,8,CY_U3P_PIB_SOCKET_1,CY_U3P_UIB_SOCKET_CONS_1,CY_U3P_DMA_CB_PROD_EVENT,DMA_Normal_CtrlIn_Cb,
			&glDMAControlIn,CY_U3P_DMA_TYPE_AUTO_SIGNAL);

	/* Data OUT Channel
	 * Create a DMA Auto channel for U Port -> P Port transfer. */
	createChannel("DmaNormal.DataOut",
			&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_UIB_SOCKET_PROD_2,CY_U3P_PIB_SOCKET_2,CY_U3P_DMA_CB_PROD_EVENT,DMA_Normal_DataOut_Cb,
			&glDMADataOut,CY_U3P_DMA_TYPE_AUTO_SIGNAL);

    /* Data IN Channel
    	 * Create a DMA Auto channel for U Port -> P Port transfer. */
	createChannel("DmaNormal.DataIn",
			&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_PIB_SOCKET_3,CY_U3P_UIB_SOCKET_CONS_2,CY_U3P_DMA_CB_PROD_EVENT,DMA_Normal_DataIn_Cb,
			&glDMADataIn,CY_U3P_DMA_TYPE_AUTO_SIGNAL);

	glDMA_mode = DMA_NORMAL;
	CyU3PDebugPrint(4,"DMA_Normal_mode end=%d\r\n", glDMA_mode);
}



// config dma lpbk mode
void DMA_LoopBack_mode(void)
{
	CyU3PDebugPrint(4,"DMA_LoopBack_mode start=%d\r\n", glDMA_mode);
	CyU3PDmaChannelConfig_t dmaCfg;
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
	uint16_t size = 0;

	channelReset();

	// reconfig DMAs

	size = 1024; // super speed <- assumed condition , temporary code

	/* Control OUT channel
	 * Create a DMA Auto Channel between two sockets of the U port and P port
	 * DMA size is set based on the USB speed. */
	createChannel("DmaLoopback.ControlOut",
			&dmaCfg,size,8,CY_U3P_UIB_SOCKET_PROD_1,CY_U3P_UIB_SOCKET_CONS_1,0,0,
			&glDMAControlOut,CY_U3P_DMA_TYPE_AUTO);

	/* Data OUT Channel
	 * Create a DMA Auto channel for U Port -> P Port transfer. */
	createChannel("DmaLoopback.DataOut",
			&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_UIB_SOCKET_PROD_2,CY_U3P_UIB_SOCKET_CONS_2,0,0,
			&glDMADataOut,CY_U3P_DMA_TYPE_AUTO);

    glDMA_mode = DMA_LP;
    CyU3PDebugPrint(4,"DMA_LoopBack_mode end=%d\r\n", glDMA_mode);
}


void DMA_SinkSource_Cb(
        CyU3PDmaChannel   *chHandle, /* Handle to the DMA channel. */
        CyU3PDmaCbType_t  type,      /* Callback type.             */
        CyU3PDmaCBInput_t *input)    /* Callback status.           */
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	CyU3PDmaBuffer_t buf_p;


	// tmp
	glDataOutInjected++;


    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
    	status = CyU3PDmaChannelDiscardBuffer (chHandle);
        if (status != CY_U3P_SUCCESS)
        {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
            CyU3PDebugPrint (4, "CyU3PDmaChannelDiscardBuffer failed, Error code = %d\n", status);
#endif
        }

    }
    if (type == CY_U3P_DMA_CB_CONS_EVENT)
    {
        status = CyU3PDmaChannelGetBuffer (chHandle, &buf_p, CYU3P_NO_WAIT);
        if (status == CY_U3P_SUCCESS)
        {
            /* Commit the full buffer with default status. */
            status = CyU3PDmaChannelCommitBuffer (chHandle, buf_p.size, 0);
            if (status != CY_U3P_SUCCESS)
            {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
                CyU3PDebugPrint (4, "DMA_SinkSource_Cb,CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", status);
#endif
            }
        }
        else
        {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
            CyU3PDebugPrint (4, "DMA_SinkSource_Cb,CyU3PDmaChannelGetBuffer failed, Error code = %d\n", status);
#endif
        }
    }
}



void
DMASrcSinkFillInBuffers (
        void)
{
    CyU3PReturnStatus_t stat;
    CyU3PDmaBuffer_t    buf_p;
    uint16_t            index = 0;

    /* Now preload all buffers in the MANUAL_OUT pipe with the required data. */
    for (index = 0; index < 8; index++)
    {
        stat = CyU3PDmaChannelGetBuffer (&glDMAControlIn, &buf_p, CYU3P_NO_WAIT);
        if (stat != CY_U3P_SUCCESS)
        {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
            CyU3PDebugPrint (4, "DMASrcSinkFillInBuffers,CyU3PDmaChannelGetBuffer failed, Error code = %d\n", stat);
#endif
            CyFxAppErrorHandler(stat);
        }

        CyU3PMemSet (buf_p.buffer, 0xA5, buf_p.size);
        stat = CyU3PDmaChannelCommitBuffer (&glDMAControlIn, buf_p.size, 0);
        if (stat != CY_U3P_SUCCESS)
        {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
            CyU3PDebugPrint (4, "DMASrcSinkFillInBuffers,CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", stat);
#endif
            CyFxAppErrorHandler(stat);
        }
    }

    /* Now preload all buffers in the MANUAL_OUT pipe with the required data. */
    for (index = 0; index < 4; index++)
    {
        stat = CyU3PDmaChannelGetBuffer (&glDMADataIn, &buf_p, CYU3P_NO_WAIT);
        if (stat != CY_U3P_SUCCESS)
        {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
            CyU3PDebugPrint (4, "DMASrcSinkFillInBuffers,CyU3PDmaChannelGetBuffer failed, Error code = %d\n", stat);
#endif
            CyFxAppErrorHandler(stat);
        }

        CyU3PMemSet (buf_p.buffer, 0xA5, buf_p.size);
        stat = CyU3PDmaChannelCommitBuffer (&glDMADataIn, buf_p.size, 0);
        if (stat != CY_U3P_SUCCESS)
        {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
            CyU3PDebugPrint (4, "DMASrcSinkFillInBuffers,CyU3PDmaChannelCommitBuffer failed, Error code = %d\n", stat);
#endif
            CyFxAppErrorHandler(stat);
        }
    }

}

// config dma sink/source mode
void DMA_SinkSource_mode(void)
{
	CyU3PDebugPrint(4,"DMA_SinkSource_mode start=%d\r\n", glDMA_mode);

	CyU3PDmaChannelConfig_t dmaCfg;
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
	uint16_t size = 0;

	channelReset();

	// reconfig DMAs

	size = 1024; // super speed <- assumed condition , temporary code

	/* Control OUT channel
	 * Create a DMA Auto Channel between two sockets of the U port and P port
	 * DMA size is set based on the USB speed. */
	createChannel("DmaSinkSource.ControlOut",
			&dmaCfg,size,8,CY_U3P_UIB_SOCKET_PROD_1,CY_U3P_CPU_SOCKET_CONS,CY_U3P_DMA_CB_PROD_EVENT,DMA_SinkSource_Cb,
			&glDMAControlOut, CY_U3P_DMA_TYPE_MANUAL_IN);

	/* Control IN channel
	 * Create a DMA Auto Channel between two sockets of the U port and P port
	 * DMA size is set based on the USB speed. */
	createChannel("DmaSinkSource.ControlIn",
			&dmaCfg,size,8,CY_U3P_CPU_SOCKET_PROD,CY_U3P_UIB_SOCKET_CONS_1,CY_U3P_DMA_CB_CONS_EVENT,DMA_SinkSource_Cb,
			&glDMAControlIn, CY_U3P_DMA_TYPE_MANUAL_OUT);

	/* Data OUT Channel
	 * Create a DMA Auto channel for U Port -> P Port transfer. */
	createChannel("DmaSinkSource.DataOut",
			&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_UIB_SOCKET_PROD_2,CY_U3P_CPU_SOCKET_CONS,CY_U3P_DMA_CB_PROD_EVENT,DMA_SinkSource_Cb,
			&glDMADataOut, CY_U3P_DMA_TYPE_MANUAL_IN);

    /* Data IN Channel
    	 * Create a DMA Auto channel for U Port -> P Port transfer. */
	createChannel("DmaSinkSource.DataIn",
			&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_CPU_SOCKET_PROD,CY_U3P_UIB_SOCKET_CONS_2,CY_U3P_DMA_CB_CONS_EVENT,DMA_SinkSource_Cb,
			&glDMADataIn, CY_U3P_DMA_TYPE_MANUAL_OUT);

	DMASrcSinkFillInBuffers();

	glDMA_mode = DMA_SINKSOURCE;
	CyU3PDebugPrint(4,"DMA_SinkSource_mode end=%d\r\n", glDMA_mode);
}



void USBEP0RxThread(uint32_t Value)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	uint32_t	evStat;

	char        *str_tk;
	uint32_t    arg[10];

	CyU3PDebugPrint (4, "\n[EP0] USBEP0RxThread start\n");
	while(1) {
		CyU3PDebugPrint (4, "\n [EP0] Event waiting...\n");
		status = CyU3PEventGet (&glEp0Event, EVT_EP0, CYU3P_EVENT_OR_CLEAR, &evStat, CYU3P_WAIT_FOREVER);
		CyU3PDebugPrint (4, "\n [EP0] EventGet return=%d\n",status);

		if (status == CY_U3P_SUCCESS) {
			if (evStat & EVT_EP0) {
#if DBG_LEVEL >= DBG_TYPE_USB_EP0
				CyU3PDebugPrint (4, "[Host Request/Num] %d \r\n",glHostReqNum);
#endif
				switch(glHostReqNum) {
				case 3:
					glHostRxData[glHostRxData_idx] = 0;
#if DBG_LEVEL >= DBG_TYPE_USB_EP0
					CyU3PDebugPrint (4, "[Host Request/Data] %s \r\n",glHostRxData);
					CyU3PDebugPrint (4, "[Host CMD] %s \r\n", glHostRxData);
#endif
					if(strcmp((const char *)glHostRxData, "DMA MODE LP") == 0) {
						DMA_LoopBack_mode();
					}
					else if(strcmp((const char *)glHostRxData, "DMA MODE SINKSOURCE") == 0) {
						DMA_SinkSource_mode();
					}
					else if(strcmp((const char *)glHostRxData, "DMA MODE NORMAL") == 0) {
						DMA_Normal_mode();
					}
					else if(strcmp((const char *)glHostRxData, "DMA MODE SYNC") == 0) {
						DMA_Sync_mode();
					}
					else if(strcmp((const char *)glHostRxData, "ZING MODE PPC") == 0) {
						Zing_SetHRCP(1);
					}
					else if(strcmp((const char *)glHostRxData, "ZING MODE DEV") == 0) {
						Zing_SetHRCP(0);
					}
					else if(strcmp((const char *)glHostRxData, "ZING MODE RF_PATH") == 0) {
						Zing_SetPath(1); // not tested
					}
					else if(strcmp((const char *)glHostRxData, "ZING MODE SERDES_PATH") == 0) {
						Zing_SetPath(0); // not tested
					}
					else if(strcmp((const char *)glHostRxData, "ZING TEST SENDMSG") == 0) {
						{
							char* msg = "Hey Hi~!";
							Zing_SendMsg((uint8_t*)msg, strlen(msg));
						}
					}
					else if(strcmp((const char *)glHostRxData, "ZING TEST RECVMSG") == 0) {
						{
							char msg[100] = {0,};
							uint32_t len;
							Zing_RecvMsg((uint8_t*)msg, &len);
						}
					}
					else if(strcmp((const char *)glHostRxData, "ZING RST") == 0) {
						CyU3PDebugPrint (4, "\n [EP0] Before Zing_Reset\n");
						Zing_Reset(0);
						CyU3PDebugPrint (4, "\n [EP0] After Zing_Reset\n");
					}
					else if(strcmp((const char *)glHostRxData, "FX3 RST") == 0) {
						CyU3PDeviceReset(CyFalse);
					}
					else if(strcmp((const char *)glHostRxData, "123") == 0) {
						{
							char* str_tmp = "Hello";
							CyU3PUsbSendEP0Data(5,(uint8_t *)str_tmp);
						}
					}
					else {
						str_tk = strtok((char *)glHostRxData, " ");
						if(strcmp(str_tk, "ZING")==0) {
							str_tk = strtok(NULL, " ");
							if(strcmp(str_tk, "REGW")==0) { // cmd : ZING REGW 8001 12345678
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 16);   /* first argument */
								str_tk = strtok(NULL, " ");
								arg[1] = (uint32_t)strtoul(str_tk, NULL, 16);   /* second argument */
					            if(arg[0] >= REGISTER_START_ADDR && arg[0] <= REGISTER_END_ADDR) /* check range */
					            {
					            	Zing_RegWrite((uint16_t)arg[0],(uint8_t*)&arg[1],4);
					            }
					            else
					            {
#if DBG_LEVEL >= DBG_TYPE_USB_EP0
					                CyU3PDebugPrint (4, "Invalid address=0x%x\r\n", arg[0]);
#endif
					            }
							}
							else if(strcmp(str_tk, "REGR")==0) { // cmd : ZING REGR 8001
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 16);   /* first argument */
					            if(arg[0] >= REGISTER_START_ADDR && arg[0] <= REGISTER_END_ADDR) /* check range */
					            {
					            	Zing_RegRead((uint16_t)arg[0],(uint8_t*)glHostTxData,4);
					            	glHostTxData_idx = 4;
					            }
					            else
					            {
#if DBG_LEVEL >= DBG_TYPE_USB_EP0
					                CyU3PDebugPrint (4, "Invalid address=0x%x\r\n", arg[0]);
#endif
					            }
							}
							else if(strcmp(str_tk, "DATATX")==0) { // cmd : ZING DATATX 5 8192 A5
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 10);   /* first argument */
								str_tk = strtok(NULL, " ");
								arg[1] = (uint32_t)strtoul(str_tk, NULL, 10);   /* second argument */
								str_tk = strtok(NULL, " ");
								arg[2] = (uint32_t)strtoul(str_tk, NULL, 16);   /* second argument */
								if(arg[1] <= 8192) /* check range */
								{
									Zing_Test_DataTx2(arg[0],arg[1],arg[2]);
								}
								else {
#if DBG_LEVEL >= DBG_TYPE_USB_EP0
					                CyU3PDebugPrint (4, "Invalid length=%d\r\n", arg[0]);
#endif
								}
							}
							else if(strcmp(str_tk, "DATASINK")==0) { // cmd : ZING DATASINK 10 2
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 10);   /* first argument */
								str_tk = strtok(NULL, " ");
								arg[1] = (uint32_t)strtoul(str_tk, NULL, 10);   /* second argument */

							    Zing_Test_DataSink2(arg[0],arg[1]);
							}
							else if(strcmp(str_tk, "MNGT_TX4B")==0) { // cmd : ZING MNGT_TX4B 12345678
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 16);   /* first argument */
								Zing_Management_Send((uint8_t*)&arg[0],4);
							}
							else if(strcmp(str_tk, "MNGT_RX4B")==0) { // cmd : ZING MNGT_RX4B
								memcpy(glHostTxData,(uint8_t*)&glMngtData,4);
				            	glHostTxData_idx = 4;
							}
							else if(strcmp(str_tk, "AFC")==0) {  // cmd : ZING AFC 1250000000
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 10);   /* first argument */
								Zing_AFC2((float)arg[0]);
							}

						}

					}
					break;
				case 4:
					{
						uint32_t tmp_cnt = 5;
						char *tmp_data = "Hello User";
#if DBG_LEVEL >= DBG_TYPE_USB_EP0
						CyU3PDebugPrint (4, "[Host Response] %s \r\n",tmp_data);
#endif
						CyU3PUsbSendEP0Data(tmp_cnt,(uint8_t *)tmp_data);
					}
//					CyU3PDebugPrint (4, "[Host Request/Tx] %d, %s \r\n",glHostRxData_idx,glHostRxData);
//					CyU3PUsbSendEP0Data(glHostRxData_idx,(uint8_t *)glHostRxData);
					break;
				default:
					break;
				}
			}
			glHostRxData_idx = 0;
		}
	}
	CyU3PDebugPrint (4, "\n[EP0] USBEP0RxThread end\n");
}

void ControlChThread(uint32_t Value)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	REG_Resp_t* resp_pt;
	uint32_t rt_len, i;

	uint8_t * buf = (uint8_t *)CyU3PDmaBufferAlloc (512);

	while(1) {
		if(glDMA_mode == DMA_SYNC) {
			status = Zing_Transfer_Recv3(&glDMAControlIn,buf,&rt_len); // wait forever...

			if(status == CY_U3P_SUCCESS) {
				resp_pt = (REG_Resp_t*)buf;
				if(resp_pt->hdr.dir == 1 && resp_pt->hdr.interrupt == 1) { // Zing Interrupt Event -----------------
					// no act
#if DBG_LEVEL >= DBG_TYPE_ZING
						CyU3PDebugPrint (4, "[Zing/ControlCh] discarded interrupt event pk\r\n");
						CyU3PDebugPrint (4, "[Zing/ControlCh] pk header :(MSB-LSB) 0x%x\r\n",*((uint64_t*)resp_pt));
#endif
				}
				else if(resp_pt->hdr.target == 1) { // Reg ---------------------------------------------------------
					memcpy(glControlChData,buf,rt_len);
					glControlChData_idx = rt_len;
					// evt
					CyU3PEventSet (&glControlChEvent, EVT_CTLCH0, CYU3P_EVENT_OR);
				}
				else if(resp_pt->hdr.dir == 1 && resp_pt->hdr.fr_type == 1) { // Management Frame ------------------
#if DBG_LEVEL >= DBG_TYPE_ZING
					CyU3PDebugPrint (4, "[Zing/ControlCh] rx management frame\r\n");
					CyU3PDebugPrint (4, "[Zing/ControlCh] frame header :(MSB-LSB) 0x%x\r\n",*((uint64_t*)resp_pt));
					CyU3PDebugPrint (4, "[Zing/ControlCh] frame data : ");
					for(i=0;i<rt_len-ZING_HDR_SIZE;i++) {
						CyU3PDebugPrint (4, "0x%X, ",buf[i+ZING_HDR_SIZE]);
					}
					CyU3PDebugPrint (4, "\r\n");

					if(0) {

					}
					else {
						if(rt_len-ZING_HDR_SIZE == 4) { // EP0 : ZING MNGT_TX4B 12345678 --> ZING MNGT_RX4B
							glMngtData = *(uint32_t*)(buf+ZING_HDR_SIZE);
						}
					}
#endif
				}
			}
		}
		else {
			CyU3PThreadSleep(10);
		}

//		CyU3PThreadSleep(1000);
//		CyU3PDebugPrint(4,"Control Ch Thread...\r\n");

	}
	CyU3PDmaBufferFree(buf);
}

CyU3PReturnStatus_t ControlChThread_Create(void)
{
    void *StackPtr = NULL;
    CyU3PReturnStatus_t Status;

    CyU3PEventCreate (&glControlChEvent);

    StackPtr = CyU3PMemAlloc(APPLICATION_THREAD_STACK);
    Status = CyU3PThreadCreate(&ControlChThreadHandle,	// Handle to my Application Thread
            "22:tmp2",                		// Thread ID and name
            ControlChThread,     					// Thread entry function
            0,                             		// Parameter passed to Thread
            StackPtr,                       		// Pointer to the allocated thread stack
            APPLICATION_THREAD_STACK,               // Allocated thread stack size
            APPLICATION_THREAD_PRIORITY,            // Thread priority
            APPLICATION_THREAD_PRIORITY,            // = Thread priority so no preemption
            CYU3P_NO_TIME_SLICE,            		// Time slice no supported
            CYU3P_AUTO_START                		// Start the thread immediately
            );

    return Status;
}


/* This function starts the application. This is called
 * when a SET_CONF event is received from the USB host. The endpoints
 * are configured and the DMA pipe is setup in this function. */
void
AppStart (
        void)
{
    uint16_t size = 0;
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

    /* First identify the usb speed. Once that is identified,
     * create a DMA channel and start the transfer on this. */

    /* Based on the Bus Speed configure the endpoint packet size */
    switch (usbSpeed)
    {
        case CY_U3P_FULL_SPEED:
            size = 64;
            break;

        case CY_U3P_HIGH_SPEED:
            size = 512;
            break;

        case  CY_U3P_SUPER_SPEED:
            size = 1024;
            break;

        default:
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
            CyU3PDebugPrint (4, "Error! Invalid USB speed.\n");
#endif
            CyFxAppErrorHandler (CY_U3P_ERROR_FAILURE);
            break;
    }

    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyTrue;
    epCfg.epType = CY_U3P_USB_EP_BULK;
    epCfg.burstLen = 1;
    epCfg.streams = 0;
    epCfg.pcktSize = size;

    /* Producer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
#endif
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
#endif
        CyFxAppErrorHandler (apiRetStatus);
    }

    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyTrue;
    epCfg.epType = CY_U3P_USB_EP_BULK;
    epCfg.burstLen = (usbSpeed == CY_U3P_SUPER_SPEED) ? (CY_FX_DATA_BURST_LENGTH) : 1;
    epCfg.streams = 0;
    epCfg.pcktSize = size;

    /* Producer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER_2, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
#endif
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Consumer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER_2, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
#endif
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Update the status flag. */
    glIsApplnActive = CyTrue;
}



/* This function stops the application. This shall be called whenever
 * a RESET or DISCONNECT event is received from the USB host. The endpoints are
 * disabled and the DMA pipe is destroyed by this function. */
void
AppStop (
        void)
{
    CyU3PEpConfig_t epCfg;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Update the flag. */
    glIsApplnActive = CyFalse;

    /* Flush the endpoint memory */
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);
    CyU3PUsbFlushEp(CY_FX_EP_PRODUCER_2);
    CyU3PUsbFlushEp(CY_FX_EP_CONSUMER_2);

    /* Disable endpoints. */
    CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
    epCfg.enable = CyFalse;

    /* Control OUT: Producer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
#endif
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Control IN: Consumer endpoint configuration. */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
#endif
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Data OUT: Producer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER_2, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
#endif
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Data IN: Consumer endpoint configuration */
    apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER_2, &epCfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
        CyU3PDebugPrint (4, "CyU3PSetEpConfig failed, Error code = %d\n", apiRetStatus);
#endif
        CyFxAppErrorHandler (apiRetStatus);
    }

}


/* Application Thread */
void ApplicationThread(uint32_t Value)
{
    CyU3PReturnStatus_t Status;

    Status = InitializeDebugConsole(6);
#if DBG_LEVEL >= DBG_TYPE_INIT
    CyU3PDebugPrint(4,"----------------------------------------------------------------\r\n");
    CyU3PDebugPrint(4,"[init] Debug Console\r\n");
#endif

    USBEP0RxThread_Create();
#if DBG_LEVEL >= DBG_TYPE_INIT
    CyU3PDebugPrint(4,"[init] EP0 Vendor Ch\r\n");
#endif
    Status = SetupGPIO();
    Status = I2C_Init();
    Status = USB_Init();
    Status = PIB_Init();

#if DBG_LEVEL >= DBG_TYPE_INIT
    CyU3PDebugPrint(4,"[init] GPIO, I2C, USB, PIB\r\n");
#endif

    DMA_Sync_mode();
    Status = ControlChThread_Create();
    Status = Zing_Init();
    //Zing_AutoHRCP();
    //Zing_SetHRCP(PPC);
    Zing_SetHRCP(DEV);
#if DBG_LEVEL >= DBG_TYPE_INIT
    CyU3PDebugPrint(4,"[init] Zing\r\n");
#endif

    USB_Connect();
    while(glIsApplnActive == 0) {
    	CyU3PThreadSleep(100);
    }

    DMA_Normal_mode();
#if DBG_LEVEL >= DBG_TYPE_INIT
    CyU3PDebugPrint(4,"[init] DMA\r\n");
#endif

#if DBG_LEVEL >= DBG_TYPE_INIT
    CyU3PDebugPrint(4,"[init] Completed\r\n");
    CyU3PDebugPrint(4,"----------------------------------------------------------------\r\n");
#endif


    //tmp
    Status = CY_U3P_SUCCESS;

    if (Status == CY_U3P_SUCCESS)
    {
//        // test GPIO
//        CyU3PGpioSetValue(GPIO57, 1);
//        CyU3PThreadSleep(10);
//        CyU3PGpioSetValue(GPIO57, 0);

        // Now run forever
    	uint32_t loop = 0;
    	while (1)
    	{
#if DBG_LEVEL >= DBG_TYPE_TR_CNT
    		CyU3PDebugPrint(4,"[%d] ConIn:%d ConOut:%d DataIn:%d DataOut:%d\r",
    				loop++,glControlInInjected,glControlOutInjected,glDataInInjected,glDataOutInjected);
#endif
    		CyU3PThreadSleep(1000);


    	}
    }
#if DBG_LEVEL >= DBG_TYPE_BASIC_ERR
    CyU3PDebugPrint(4, "\r\nApplication failed to initialize. Error code: %d.\r\n", Status);
#endif

    while (1);		// Hang here
}






