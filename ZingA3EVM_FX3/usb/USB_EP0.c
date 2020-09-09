#include "USB_EP0.h"
#include "cyu3error.h"
#include "cyu3system.h"
#include "..\dma\dma.h"
#include "..\zing\Zing.h"
#include "cyu3usb.h"
#include "..\zing\ZingHw.h"
#include "..\zing\ControlCh.h"
#include "..\AppThread.h"

CyU3PThread EP0ThreadHandle;

void USBEP0RxThread(uint32_t Value)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	uint32_t evStat;
	char *str_tk;
	uint32_t arg[10];

	CyU3PDebugPrint (4, "[EP0] USBEP0RxThread start\n");
	while(1) {
		CyU3PDebugPrint (4, "[EP0] Event waiting...\n");
		status = CyU3PEventGet (&UsbEp0Ctx.Event_, EVT_EP0, CYU3P_EVENT_OR_CLEAR, &evStat, CYU3P_WAIT_FOREVER);
		CyU3PDebugPrint (4, "[EP0] EventGet return:%d\n",status);

		if (status == CY_U3P_SUCCESS) {
			if (evStat & EVT_EP0) {
				CyU3PDebugPrint (4, "[EP0] HostRequestNum:%d\n",UsbEp0Ctx.HostReqNum_);
				switch(UsbEp0Ctx.HostReqNum_) {
				case 3:
					UsbEp0Ctx.HostRxData_[UsbEp0Ctx.HostRxData_idx_] = 0;
					CyU3PDebugPrint (4, "[EP0] HostRxData:%s\n",UsbEp0Ctx.HostRxData_);

					if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "DMA MODE LP") == 0) {
						DMA_LoopBack_mode();
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "DMA MODE SINKSOURCE") == 0) {
						DMA_SinkSource_mode();
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "DMA MODE NORMAL") == 0) {
						DMA_Normal_mode();
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "DMA MODE SYNC") == 0) {
						DMA_Sync_mode();
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "ZING MODE PPC") == 0) {
						Zing_SetHRCP(1);
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "ZING MODE DEV") == 0) {
						Zing_SetHRCP(0);
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "ZING MODE RF_PATH") == 0) {
						Zing_SetPath(1); // not tested
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "ZING MODE SERDES_PATH") == 0) {
						Zing_SetPath(0); // not tested
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "ZING TEST SENDMSG") == 0) {
						{
							char* msg = "Hey Hi~!";
							Zing_SendMsg((uint8_t*)msg, strlen(msg));
						}
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "ZING TEST RECVMSG") == 0) {
						{
							char msg[100] = {0,};
							uint32_t len;
							Zing_RecvMsg((uint8_t*)msg, &len);
						}
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "ZING RST") == 0) {
						CyU3PDebugPrint (4, "[EP0] Before Zing_Reset\n");
						Zing_Reset(0);
						CyU3PDebugPrint (4, "[EP0] After Zing_Reset\n");
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "FX3 RST") == 0) {
						CyU3PDeviceReset(CyFalse);
					}
					else if(strcmp((const char *)UsbEp0Ctx.HostRxData_, "123") == 0) {
						{
							char* str_tmp = "Hello";
							CyU3PUsbSendEP0Data(5,(uint8_t *)str_tmp);
						}
					}
					else {
						str_tk = strtok((char *)UsbEp0Ctx.HostRxData_, " ");
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
									Zing_RegRead((uint16_t)arg[0],(uint8_t*)UsbEp0Ctx.HostTxData_,4);
									UsbEp0Ctx.HostTxData_idx_ = 4;
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
								memcpy(UsbEp0Ctx.HostTxData_,(uint8_t*)&CcCtx.MngtData_,4);
								UsbEp0Ctx.HostTxData_idx_ = 4;
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
			UsbEp0Ctx.HostRxData_idx_ = 0;
		}
	}
	CyU3PDebugPrint (4, "[EP0] USBEP0RxThread end\n");
}

CyU3PReturnStatus_t USBEP0RxThread_Create(void)
{
    void *StackPtr = NULL;
    CyU3PReturnStatus_t Status;

    Status = CyU3PEventCreate(&UsbEp0Ctx.Event_);
    if(Status!=CY_U3P_SUCCESS) return Status;

    StackPtr = CyU3PMemAlloc(APPLICATION_THREAD_STACK);
    Status = CyU3PThreadCreate(&EP0ThreadHandle,	// Handle to my Application Thread
            "12:tmp",                		// Thread ID and name
            USBEP0RxThread,     					// Thread entry function
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
