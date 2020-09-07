#include "Application.h"
#include "gitcommit.h"
#include "DebugConsole.h"
#include "USB_Handler.h"
#include "PIB.h"
#include "LPP.h"
#include "ZingHw.h"
#include "cyu3error.h"
#include "Zing.h"
#include "dma.h"

//App thread, flag
CyU3PThread ApplicationThreadHandle;	//Handle to my Application Thread
CyBool_t glIsApplnActive = CyFalse;		//Whether the application is active or not

//Ep0 vendor ch
CyU3PEvent glEp0Event;					//Event group used to signal the thread
uint32_t glHostReqNum;
uint8_t glHostRxData[128];
uint32_t glHostRxData_idx;
uint8_t glHostTxData[128];
uint32_t glHostTxData_idx;

//Zing control ch thread
CyU3PThread ControlChThreadHandle;		//Handle to my Application Thread
CyU3PEvent glControlChEvent;
uint8_t glControlChData[512];
uint32_t glControlChData_idx;

uint32_t glMngtData;

void USBEP0RxThread(uint32_t Value)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	uint32_t evStat;
	char *str_tk;
	uint32_t arg[10];

	CyU3PDebugPrint (4, "[EP0] USBEP0RxThread start\n");
	while(1) {
		CyU3PDebugPrint (4, "[EP0] Event waiting...\n");
		status = CyU3PEventGet (&glEp0Event, EVT_EP0, CYU3P_EVENT_OR_CLEAR, &evStat, CYU3P_WAIT_FOREVER);
		CyU3PDebugPrint (4, "[EP0] EventGet return=%d\n",status);

		if (status == CY_U3P_SUCCESS) {
			if (evStat & EVT_EP0) {
				CyU3PDebugPrint (4, "[EP0] HostRequestNum=%d\n",glHostReqNum);
				switch(glHostReqNum) {
				case 3:
					glHostRxData[glHostRxData_idx] = 0;
					CyU3PDebugPrint (4, "[EP0] HostRxData %s\n",glHostRxData);

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
						CyU3PDebugPrint (4, "[EP0] Before Zing_Reset\n");
						Zing_Reset(0);
						CyU3PDebugPrint (4, "[EP0] After Zing_Reset\n");
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
							if(strcmp(str_tk, "REGW")==0) {	//cmd : ZING REGW 8001 12345678
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 16);	//first argument
								str_tk = strtok(NULL, " ");
								arg[1] = (uint32_t)strtoul(str_tk, NULL, 16);	//second argument
								if(arg[0] >= REGISTER_START_ADDR && arg[0] <= REGISTER_END_ADDR)	//check range
								{
									Zing_RegWrite((uint16_t)arg[0],(uint8_t*)&arg[1],4);
								}
								else
								{
									CyU3PDebugPrint (4, "[EP0] Invalid address=0x%x\r\n", arg[0]);
								}
							}
							else if(strcmp(str_tk, "REGR")==0) {	//cmd : ZING REGR 8001
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 16);	//first argument
								if(arg[0] >= REGISTER_START_ADDR && arg[0] <= REGISTER_END_ADDR)	//check range
								{
									Zing_RegRead((uint16_t)arg[0],(uint8_t*)glHostTxData,4);
									glHostTxData_idx = 4;
								}
								else
								{
									CyU3PDebugPrint (4, "[EP0] Invalid address=0x%x\r\n", arg[0]);
								}
							}
							else if(strcmp(str_tk, "DATATX")==0) {	//cmd : ZING DATATX 5 8192 A5
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 10);	//first argument
								str_tk = strtok(NULL, " ");
								arg[1] = (uint32_t)strtoul(str_tk, NULL, 10);	//second argument
								str_tk = strtok(NULL, " ");
								arg[2] = (uint32_t)strtoul(str_tk, NULL, 16);	//second argument
								if(arg[1] <= 8192)	//check range
								{
									Zing_Test_DataTx2(arg[0],arg[1],arg[2]);
								}
								else {
									CyU3PDebugPrint (4, "[EP0] Invalid length=%d\r\n", arg[0]);
								}
							}
							else if(strcmp(str_tk, "DATASINK")==0) {	//cmd : ZING DATASINK 10 2
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 10);	//first argument
								str_tk = strtok(NULL, " ");
								arg[1] = (uint32_t)strtoul(str_tk, NULL, 10);	//second argument
							    Zing_Test_DataSink2(arg[0],arg[1]);
							}
							else if(strcmp(str_tk, "MNGT_TX4B")==0) {	//cmd : ZING MNGT_TX4B 12345678
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 16);	//first argument
								Zing_Management_Send((uint8_t*)&arg[0],4);
							}
							else if(strcmp(str_tk, "MNGT_RX4B")==0) {	//cmd : ZING MNGT_RX4B
								memcpy(glHostTxData,(uint8_t*)&glMngtData,4);
								glHostTxData_idx = 4;
							}
							else if(strcmp(str_tk, "AFC")==0) {			//cmd : ZING AFC 1250000000
								str_tk = strtok(NULL, " ");
								arg[0] = (uint32_t)strtoul(str_tk, NULL, 10);	//first argument
								Zing_AFC2((float)arg[0]);
							}
						}
					}
					break;
				case 4:
					{
						uint32_t tmp_cnt = 5;
						char *tmp_data = "Hello User";
						CyU3PDebugPrint (4, "[EP0] Host Response=%s\n",tmp_data);
						CyU3PUsbSendEP0Data(tmp_cnt,(uint8_t *)tmp_data);
					}
					break;
				default:
					break;
				}
			}
			glHostRxData_idx = 0;
		}
	}
	CyU3PDebugPrint (4, "[EP0] USBEP0RxThread end\n");
}

void ControlChThread(uint32_t Value)
{
	REG_Resp_t *resp_pt;
	uint32_t rt_len,recv,intEvt,regRead,manFrame;
	uint8_t *buf = (uint8_t *)CyU3PDmaBufferAlloc (512);

	recv = intEvt = regRead = manFrame = 0;
	while(1) {
		if(dmaMode == DMA_SYNC) {
			if(Zing_Transfer_Recv3(&dmaControlIn,buf,&rt_len) == CY_U3P_SUCCESS) {
				recv++;
				resp_pt = (REG_Resp_t*)buf;
				if(resp_pt->hdr.dir == 1 && resp_pt->hdr.interrupt == 1) { //Zing Interrupt Event
					// no act
					intEvt++;
					CyU3PDebugPrint (4, "[ZCH] discarded interrupt event pk\r\n");
					CyU3PDebugPrint (4, "[ZCH] pk header :(MSB-LSB) 0x%x\r\n",*((uint64_t*)resp_pt));
				}
				else if(resp_pt->hdr.target == 1) { //Reg
					regRead++;
					memcpy(glControlChData,buf,rt_len);
					glControlChData_idx = rt_len;
					CyU3PEventSet (&glControlChEvent, EVT_CTLCH0, CYU3P_EVENT_OR);
				}
				else if(resp_pt->hdr.dir == 1 && resp_pt->hdr.fr_type == 1) { //Management Frame
					manFrame++;
					CyU3PDebugPrint (4, "[ZCH] rx management frame\r\n");
					CyU3PDebugPrint (4, "[ZCH] frame header :(MSB-LSB) 0x%x\r\n",*((uint64_t*)resp_pt));
					CyU3PDebugPrint (4, "[ZCH] frame data : ");
					for(uint32_t i=0;i<rt_len-ZING_HDR_SIZE;i++) CyU3PDebugPrint (4, "0x%X, ",buf[i+ZING_HDR_SIZE]);
					CyU3PDebugPrint (4, "\r\n");

					if(rt_len-ZING_HDR_SIZE == 4) { //EP0 : ZING MNGT_TX4B 12345678 --> ZING MNGT_RX4B
						glMngtData = *(uint32_t*)(buf+ZING_HDR_SIZE);
					}
				}
				CyU3PDebugPrint(4,"[ZCH] Recv:%d Interrupt:%d RegRead:%d ManFrame:%d\n",recv,intEvt,regRead,manFrame);
			}
		}
		else {
			CyU3PThreadSleep(10);
		}
	}
	CyU3PDmaBufferFree(buf);
}

CyU3PReturnStatus_t ControlChThread_Create(void)
{
	void *StackPtr = NULL;
	CyU3PReturnStatus_t Status;

	Status = CyU3PEventCreate(&glControlChEvent);
	if (Status != CY_U3P_SUCCESS) return Status;

	StackPtr = CyU3PMemAlloc(APPLICATION_THREAD_STACK);
	Status = CyU3PThreadCreate(&ControlChThreadHandle,	// Handle to my Application Thread
		"22:tmp2",								// Thread ID and name
		ControlChThread,						// Thread entry function
		0,										// Parameter passed to Thread
		StackPtr,								// Pointer to the allocated thread stack
		APPLICATION_THREAD_STACK,				// Allocated thread stack size
		APPLICATION_THREAD_PRIORITY,			// Thread priority
		APPLICATION_THREAD_PRIORITY,			// = Thread priority so no preemption
		CYU3P_NO_TIME_SLICE,					// Time slice no supported
		CYU3P_AUTO_START						// Start the thread immediately
	);
	return Status;
}


/* This function starts the application. This is called
 * when a SET_CONF event is received from the USB host. The endpoints
 * are configured and the DMA pipe is setup in this function. */
void AppStart(void)
{
	uint16_t size = 0;
	CyU3PEpConfig_t epCfg;
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
	CyU3PUSBSpeed_t usbSpeed = CyU3PUsbGetSpeed();

    /* First identify the usb speed. Once that is identified,
     * create a DMA channel and start the transfer on this. */

    /* Based on the Bus Speed configure the endpoint packet size */
	switch(usbSpeed)
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
		CyFxAppErrorHandler("Invalid USB speed",CY_U3P_ERROR_FAILURE);
		break;
	}

	CyU3PMemSet((uint8_t *)&epCfg, 0, sizeof (epCfg));
	epCfg.enable = CyTrue;
	epCfg.epType = CY_U3P_USB_EP_BULK;
	epCfg.burstLen = 1;
	epCfg.streams = 0;
	epCfg.pcktSize = size;

	apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyFxAppErrorHandler("CyU3PSetEpConfig(CY_FX_EP_PRODUCER)",apiRetStatus);
	}

	apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyFxAppErrorHandler("CyU3PSetEpConfig(CY_FX_EP_CONSUMER)",apiRetStatus);
    }

	CyU3PMemSet ((uint8_t *)&epCfg, 0, sizeof (epCfg));
	epCfg.enable = CyTrue;
	epCfg.epType = CY_U3P_USB_EP_BULK;
	epCfg.burstLen = (usbSpeed == CY_U3P_SUPER_SPEED) ? (CY_FX_DATA_BURST_LENGTH) : 1;
	epCfg.streams = 0;
	epCfg.pcktSize = size;

	apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER_2, &epCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyFxAppErrorHandler("CyU3PSetEpConfig(CY_FX_EP_PRODUCER_2)",apiRetStatus);
	}

	apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER_2, &epCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyFxAppErrorHandler("CyU3PSetEpConfig(CY_FX_EP_CONSUMER_2)",apiRetStatus);
	}

	/* Update the status flag. */
	glIsApplnActive = CyTrue;
}

/* This function stops the application. This shall be called whenever
 * a RESET or DISCONNECT event is received from the USB host. The endpoints are
 * disabled and the DMA pipe is destroyed by this function. */
void AppStop(void)
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
	CyU3PMemSet((uint8_t *)&epCfg, 0, sizeof (epCfg));
	epCfg.enable = CyFalse;

	/* Control OUT: Producer endpoint configuration. */
	apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER, &epCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyFxAppErrorHandler("CyU3PSetEpConfig(CY_FX_EP_PRODUCER)",apiRetStatus);
	}

    /* Control IN: Consumer endpoint configuration. */
	apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER, &epCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyFxAppErrorHandler("CyU3PSetEpConfig(CY_FX_EP_CONSUMER)",apiRetStatus);
    }

	/* Data OUT: Producer endpoint configuration */
	apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_PRODUCER_2, &epCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
    {
		CyFxAppErrorHandler("CyU3PSetEpConfig(CY_FX_EP_PRODUCER_2)",apiRetStatus);
    }

	/* Data IN: Consumer endpoint configuration */
	apiRetStatus = CyU3PSetEpConfig(CY_FX_EP_CONSUMER_2, &epCfg);
	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyFxAppErrorHandler("CyU3PSetEpConfig(CY_FX_EP_CONSUMER_2)",apiRetStatus);
	}
}

void ApplicationThread(uint32_t Value)
{
	CyU3PReturnStatus_t Status;

	Status = InitializeDebugConsole(6);
	CheckStatus("[App] InitializeDebugConsole", Status);

	CyU3PDebugPrint(4,"[App] Git:%s\n",GIT_INFO);

	Status = USBEP0RxThread_Create();
	CheckStatus("[App] USBEP0RxThread_Create", Status);

	Status = SetupGPIO();
	CheckStatus("[App] SetupGPIO", Status);

	Status = I2C_Init();
	CheckStatus("[App] I2C_Init", Status);

	Status = USB_Init();
	CheckStatus("[App] USB_Init", Status);

	Status = PIB_Init();
	CheckStatus("[App] PIB_Init", Status);

	DMA_Sync_mode();

	Status = ControlChThread_Create();
	CheckStatus("[App] ControlChThread_Create", Status);

	Status = Zing_Init();
	CheckStatus("[App] Zing_Init", Status);

#if 0
	Zing_AutoHRCP();
	Zing_SetHRCP(PPC);
#endif
	Zing_SetHRCP(DEV);

	Status = USB_Connect();
	CheckStatus("[App] USB_Connect", Status);

	while(glIsApplnActive == 0) {
		CyU3PThreadSleep(100);
	}

	DMA_Normal_mode();

	uint32_t loop = 0;
	while (1)
	{
		CyU3PDebugPrint(4,"[App] Loop:%d ConIn:%d ConOut:%d DataIn:%d DataOut:%d\r",
				loop++,controlInInjected,controlOutInjected,dataInInjected,dataOutInjected);

		CyU3PThreadSleep(1000);
	}

	while (1);	// Hang here
}
