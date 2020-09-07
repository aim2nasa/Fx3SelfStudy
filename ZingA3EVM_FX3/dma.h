#ifndef __DMA_H__
#define __DMA_H__

#include "cyu3types.h"
#include "cyu3dma.h"

typedef enum DMA_MODE_T
{
	DMA_NORMAL = 0,
	DMA_SYNC,
	DMA_LP,
	DMA_SINKSOURCE,
} dma_mode_t;

dma_mode_t glDMA_mode;
CyU3PDmaChannel glDMAControlOut;
CyU3PDmaChannel glDMAControlIn;
CyU3PDmaChannel glDMADataOut;
CyU3PDmaChannel glDMADataIn;
uint32_t glDataOutInjected;
uint32_t glDataInInjected;
uint32_t glControlOutInjected;
uint32_t glControlInInjected;

void setDmaChannelCfg(CyU3PDmaChannelConfig_t *pDmaCfg, uint16_t size, uint16_t count, CyU3PDmaSocketId_t prodSckId,
		CyU3PDmaSocketId_t consSckId, uint32_t notification, CyU3PDmaCallback_t cb);
void createChannel(const char* name,
		CyU3PDmaChannelConfig_t *pDmaCfg,uint16_t size,uint16_t count,CyU3PDmaSocketId_t prodSckId,
		CyU3PDmaSocketId_t consSckId,uint32_t notification,CyU3PDmaCallback_t cb,
		CyU3PDmaChannel *handle,CyU3PDmaType_t type);
void channelReset();
void DMA_Sync_mode(void);
void DMA_Normal_CtrlOut_Cb(CyU3PDmaChannel *handle,CyU3PDmaCbType_t evtype,CyU3PDmaCBInput_t *input);
void DMA_Normal_CtrlIn_Cb(CyU3PDmaChannel *handle,CyU3PDmaCbType_t evtype,CyU3PDmaCBInput_t *input);
void DMA_Normal_DataOut_Cb(CyU3PDmaChannel *handle,CyU3PDmaCbType_t evtype,CyU3PDmaCBInput_t *input);
void DMA_Normal_DataIn_Cb(CyU3PDmaChannel *handle,CyU3PDmaCbType_t evtype,CyU3PDmaCBInput_t *input);
void DMA_Normal_mode(void);
void DMA_LoopBack_mode(void);
void DMA_SinkSource_Cb(CyU3PDmaChannel *chHandle,CyU3PDmaCbType_t type,CyU3PDmaCBInput_t *input);
void DMASrcSinkFillInBuffers(void);
void DMA_SinkSource_mode(void);

#endif
