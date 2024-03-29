#ifndef __ZING_H__
#define __ZING_H__

#include "cyu3types.h"
#include "cyu3dma.h"

#define ZING_RF_SERDES_PATH		(1)		//0 : serdes path, 1: rf path

CyU3PReturnStatus_t Zing_Init(void);
CyU3PReturnStatus_t Zing_RegWrite(uint16_t addr, uint8_t* buf, uint16_t len);
CyU3PReturnStatus_t Zing_RegRead(uint16_t addr, uint8_t* buf, uint16_t len);
void Zing_AFC2(float f_tg);
void Zing_SetHRCP(uint32_t val);
void Zing_SetPath(uint32_t val);
CyU3PReturnStatus_t Zing_SendMsg(uint8_t* buf, uint32_t len);
CyU3PReturnStatus_t Zing_RecvMsg(uint8_t* buf, uint32_t* len_pt);
void Zing_Reset(uint8_t type);
CyU3PReturnStatus_t Zing_AutoHRCP(void);
uint32_t Zing_GetVersion(void);
void Zing_Test_DataTx2 (uint32_t repeat, uint32_t length, uint32_t pattern);
void Zing_Test_DataSink2 (uint32_t cnt, uint32_t timeout);
CyU3PReturnStatus_t Zing_Transfer_Recv3(CyU3PDmaChannel *dma_ch,uint8_t *data,uint32_t *length_pt);
CyU3PReturnStatus_t Zing_Management_Send(uint8_t *data,uint32_t length);

#endif
