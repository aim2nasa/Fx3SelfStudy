#include "dma.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "..\uart\DebugConsole.h"
#include "..\Application.h"

void initDmaCount()
{
	Dma.Mode_= DMA_NORMAL;
	Dma.DataOutCount_ = 0;
	Dma.DataInCount_ = 0;
	Dma.ControlOutCount_ = 0;
	Dma.ControlInCount_ = 0;
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
	char msg[256]={0,};
	memcpy(msg,name,strlen(name));

	if(strcmp(name,"DmaNormal.DataIn")==0) {
#if PACKET_SUSPEND == 1
		notification |= CY_U3P_DMA_CB_PROD_SUSP | CY_U3P_DMA_CB_CONS_SUSP;
#endif
	}

	setDmaChannelCfg(pDmaCfg, size, count, prodSckId, consSckId, notification, cb);
	apiRetStatus = CyU3PDmaChannelCreate(handle, type, pDmaCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		strcat(msg," CyU3PDmaChannelCreate");
		CyFxAppErrorHandler(msg,apiRetStatus);
	}

	if(strcmp(name,"DmaNormal.DataIn")==0) {
#if PACKET_SUSPEND == 1
		CyU3PDmaChannelSetSuspend(&handle,CY_U3P_DMA_SCK_SUSP_EOP,CY_U3P_DMA_SCK_SUSP_EOP);
#endif
	}

	apiRetStatus = CyU3PDmaChannelSetXfer(handle, 0);	//Set DMA Channel transfer size to INFINITE
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		strcat(msg," CyU3PDmaChannelSetXfer");
		CyFxAppErrorHandler(msg,apiRetStatus);
	}
}

void channelReset()
{
	//Abort & destroy DMAs
	CyU3PDmaChannelAbort(&Dma.ControlOut_);
	CyU3PDmaChannelAbort(&Dma.ControlIn_);
	CyU3PDmaChannelAbort(&Dma.DataOut_);
	CyU3PDmaChannelAbort(&Dma.DataIn_);

	CyU3PDmaChannelDestroy(&Dma.ControlOut_);
	CyU3PDmaChannelDestroy(&Dma.ControlIn_);
	CyU3PDmaChannelDestroy(&Dma.DataOut_);
	CyU3PDmaChannelDestroy(&Dma.DataIn_);

	//Flush the Endpoint memory
	CyU3PUsbFlushEp(CY_FX_EP_PRODUCER);
	CyU3PUsbFlushEp(CY_FX_EP_CONSUMER);
	CyU3PUsbFlushEp(CY_FX_EP_PRODUCER_2);
	CyU3PUsbFlushEp(CY_FX_EP_CONSUMER_2);
}

void DMA_Sync_mode(void)
{
	CyU3PDmaChannelConfig_t dmaCfg;
	uint16_t size = 1024; // super speed <- assumed condition , temporary code

	channelReset();

	createChannel("DmaSync.ControlOut",
		&dmaCfg,size,8,CY_U3P_CPU_SOCKET_PROD,CY_U3P_PIB_SOCKET_0,CY_U3P_DMA_CB_PROD_EVENT,0,
		&Dma.ControlOut_,CY_U3P_DMA_TYPE_MANUAL_OUT);

	createChannel("DmaSync.ControlIn",
		&dmaCfg,size,8,CY_U3P_PIB_SOCKET_1,CY_U3P_CPU_SOCKET_CONS,CY_U3P_DMA_CB_PROD_EVENT,0,
		&Dma.ControlIn_,CY_U3P_DMA_TYPE_MANUAL_IN);

	createChannel("DmaSync.DataOut",
		&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_CPU_SOCKET_PROD,CY_U3P_PIB_SOCKET_2,CY_U3P_DMA_CB_PROD_EVENT,0,
		&Dma.DataOut_,CY_U3P_DMA_TYPE_MANUAL_OUT);

	createChannel("DmaSync.DataIn",
		&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_PIB_SOCKET_3,CY_U3P_CPU_SOCKET_CONS,CY_U3P_DMA_CB_PROD_EVENT,0,
		&Dma.DataIn_,CY_U3P_DMA_TYPE_MANUAL_IN);

	Dma.Mode_ = DMA_SYNC;
	CyU3PDebugPrint(4,"DMA_Sync_mode(%d) done\n", Dma.Mode_);
}

void DMA_Normal_CtrlOut_Cb(CyU3PDmaChannel *handle,CyU3PDmaCbType_t evtype,CyU3PDmaCBInput_t *input)
{
	switch (evtype)
	{
	case CY_U3P_DMA_CB_PROD_EVENT:
		Dma.ControlOutCount_++;
		break;
	case CY_U3P_DMA_CB_SEND_CPLT:	//override mode
		break;
	case CY_U3P_DMA_CB_CONS_EVENT:	//normal mode
		break;
	default:
		break;
	}
}

void DMA_Normal_CtrlIn_Cb(CyU3PDmaChannel *handle,CyU3PDmaCbType_t evtype,CyU3PDmaCBInput_t *input)
{
	switch (evtype)
	{
	case CY_U3P_DMA_CB_PROD_EVENT:
		Dma.ControlInCount_++;
		break;
	case CY_U3P_DMA_CB_SEND_CPLT:	//override mode
		break;
	case CY_U3P_DMA_CB_CONS_EVENT:	//normal mode
		break;
	default:
		break;
	}
}

void DMA_Normal_DataOut_Cb(CyU3PDmaChannel *handle,CyU3PDmaCbType_t evtype,CyU3PDmaCBInput_t *input)
{
	switch (evtype)
	{
	case CY_U3P_DMA_CB_PROD_EVENT:
		Dma.DataOutCount_++;
		break;
	case CY_U3P_DMA_CB_SEND_CPLT:	//override mode
		break;
	case CY_U3P_DMA_CB_CONS_EVENT:	//normal mode
		break;
	default:
		break;
	}
}

void DMA_Normal_DataIn_Cb(CyU3PDmaChannel *handle,CyU3PDmaCbType_t evtype,CyU3PDmaCBInput_t *input)
{
	switch (evtype)
	{
	case CY_U3P_DMA_CB_PROD_EVENT:
		Dma.DataInCount_++;
		break;
	case CY_U3P_DMA_CB_SEND_CPLT:	//override mode
		break;
	case CY_U3P_DMA_CB_CONS_EVENT:	//normal mode
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
	CyU3PDmaChannelConfig_t dmaCfg;
	uint16_t size = 1024; // super speed <- assumed condition , temporary code

	channelReset();

	createChannel("DmaNormal.ControlOut",
		&dmaCfg,size,8,CY_U3P_UIB_SOCKET_PROD_1,CY_U3P_PIB_SOCKET_0,CY_U3P_DMA_CB_PROD_EVENT,DMA_Normal_CtrlOut_Cb,
		&Dma.ControlOut_,CY_U3P_DMA_TYPE_AUTO_SIGNAL);

	createChannel("DmaNormal.ControlIn",
		&dmaCfg,size,8,CY_U3P_PIB_SOCKET_1,CY_U3P_UIB_SOCKET_CONS_1,CY_U3P_DMA_CB_PROD_EVENT,DMA_Normal_CtrlIn_Cb,
		&Dma.ControlIn_,CY_U3P_DMA_TYPE_AUTO_SIGNAL);

	createChannel("DmaNormal.DataOut",
		&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_UIB_SOCKET_PROD_2,CY_U3P_PIB_SOCKET_2,CY_U3P_DMA_CB_PROD_EVENT,DMA_Normal_DataOut_Cb,
		&Dma.DataOut_,CY_U3P_DMA_TYPE_AUTO_SIGNAL);

	createChannel("DmaNormal.DataIn",
		&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_PIB_SOCKET_3,CY_U3P_UIB_SOCKET_CONS_2,CY_U3P_DMA_CB_PROD_EVENT,DMA_Normal_DataIn_Cb,
		&Dma.DataIn_,CY_U3P_DMA_TYPE_AUTO_SIGNAL);

	Dma.Mode_ = DMA_NORMAL;
	CyU3PDebugPrint(4,"DMA_Normal_mode(%d) done\n", Dma.Mode_);
}

void DMA_LoopBack_mode(void)
{
	CyU3PDmaChannelConfig_t dmaCfg;
	uint16_t size = 1024; // super speed <- assumed condition , temporary code

	channelReset();

	createChannel("DmaLoopback.ControlOut",
			&dmaCfg,size,8,CY_U3P_UIB_SOCKET_PROD_1,CY_U3P_UIB_SOCKET_CONS_1,0,0,
			&Dma.ControlOut_,CY_U3P_DMA_TYPE_AUTO);

	createChannel("DmaLoopback.DataOut",
			&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_UIB_SOCKET_PROD_2,CY_U3P_UIB_SOCKET_CONS_2,0,0,
			&Dma.DataOut_,CY_U3P_DMA_TYPE_AUTO);

	Dma.Mode_ = DMA_LP;
	CyU3PDebugPrint(4,"DMA_LoopBack_mode(%d) done\n", Dma.Mode_);
}

void DMA_SinkSource_Cb(CyU3PDmaChannel *chHandle,CyU3PDmaCbType_t type,CyU3PDmaCBInput_t *input)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	CyU3PDmaBuffer_t buf_p;

	Dma.DataOutCount_++;

	if (type == CY_U3P_DMA_CB_PROD_EVENT)
	{
		status = CyU3PDmaChannelDiscardBuffer(chHandle);
		if (status != CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint(4, "DMA_SinkSource_Cb,CyU3PDmaChannelDiscardBuffer failed(0x%x)\n", status);
		}
	}
	if (type == CY_U3P_DMA_CB_CONS_EVENT)
	{
		status = CyU3PDmaChannelGetBuffer(chHandle, &buf_p, CYU3P_NO_WAIT);
		if (status == CY_U3P_SUCCESS)
		{
			status = CyU3PDmaChannelCommitBuffer(chHandle, buf_p.size, 0);	//Commit the full buffer with default status
			if (status != CY_U3P_SUCCESS)
			{
				CyU3PDebugPrint(4, "DMA_SinkSource_Cb,CyU3PDmaChannelCommitBuffer failed(0x%x)\n", status);
			}
		}
		else
		{
			CyU3PDebugPrint(4, "DMA_SinkSource_Cb,CyU3PDmaChannelGetBuffer failed(0x%x)\n", status);
		}
	}
}

void DMASrcSinkFillInBuffers(void)
{
	CyU3PReturnStatus_t stat;
	CyU3PDmaBuffer_t    buf_p;
	uint16_t            index = 0;

	/* Now preload all buffers in the MANUAL_OUT pipe with the required data. */
	for (index = 0; index < 8; index++)
	{
		stat = CyU3PDmaChannelGetBuffer(&Dma.ControlIn_, &buf_p, CYU3P_NO_WAIT);
		if (stat != CY_U3P_SUCCESS)
		{
			CyFxAppErrorHandler("DMASrcSinkFillInBuffers,CyU3PDmaChannelGetBuffer",stat);
		}

		CyU3PMemSet(buf_p.buffer, 0xA5, buf_p.size);
		stat = CyU3PDmaChannelCommitBuffer(&Dma.ControlIn_, buf_p.size, 0);
		if (stat != CY_U3P_SUCCESS)
		{
			CyFxAppErrorHandler("DMASrcSinkFillInBuffers,CyU3PDmaChannelCommitBuffer",stat);
		}
	}

	/* Now preload all buffers in the MANUAL_OUT pipe with the required data. */
	for (index = 0; index < 4; index++)
	{
		stat = CyU3PDmaChannelGetBuffer(&Dma.DataIn_, &buf_p, CYU3P_NO_WAIT);
		if (stat != CY_U3P_SUCCESS)
		{
			CyFxAppErrorHandler("DMASrcSinkFillInBuffers,CyU3PDmaChannelGetBuffer",stat);
		}

		CyU3PMemSet(buf_p.buffer, 0xA5, buf_p.size);
		stat = CyU3PDmaChannelCommitBuffer(&Dma.DataIn_, buf_p.size, 0);
		if (stat != CY_U3P_SUCCESS)
		{
			CyFxAppErrorHandler("DMASrcSinkFillInBuffers,CyU3PDmaChannelCommitBuffer",stat);
		}
	}
}

void DMA_SinkSource_mode(void)
{
	CyU3PDmaChannelConfig_t dmaCfg;
	uint16_t size = 1024; // super speed <- assumed condition , temporary code

	channelReset();

	createChannel("DmaSinkSource.ControlOut",
			&dmaCfg,size,8,CY_U3P_UIB_SOCKET_PROD_1,CY_U3P_CPU_SOCKET_CONS,CY_U3P_DMA_CB_PROD_EVENT,DMA_SinkSource_Cb,
			&Dma.ControlOut_, CY_U3P_DMA_TYPE_MANUAL_IN);

	createChannel("DmaSinkSource.ControlIn",
			&dmaCfg,size,8,CY_U3P_CPU_SOCKET_PROD,CY_U3P_UIB_SOCKET_CONS_1,CY_U3P_DMA_CB_CONS_EVENT,DMA_SinkSource_Cb,
			&Dma.ControlIn_, CY_U3P_DMA_TYPE_MANUAL_OUT);

	createChannel("DmaSinkSource.DataOut",
			&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_UIB_SOCKET_PROD_2,CY_U3P_CPU_SOCKET_CONS,CY_U3P_DMA_CB_PROD_EVENT,DMA_SinkSource_Cb,
			&Dma.DataOut_, CY_U3P_DMA_TYPE_MANUAL_IN);

	createChannel("DmaSinkSource.DataIn",
			&dmaCfg,size*CY_FX_DATA_BURST_LENGTH,4,CY_U3P_CPU_SOCKET_PROD,CY_U3P_UIB_SOCKET_CONS_2,CY_U3P_DMA_CB_CONS_EVENT,DMA_SinkSource_Cb,
			&Dma.DataIn_, CY_U3P_DMA_TYPE_MANUAL_OUT);

	DMASrcSinkFillInBuffers();

	Dma.Mode_ = DMA_SINKSOURCE;
	CyU3PDebugPrint(4,"DMA_SinkSource_mode(%d) done\n", Dma.Mode_);
}
