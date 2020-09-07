#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "cyu3types.h"
#include "cyu3dma.h"

// ======================================================================================
// ZING
// ======================================================================================
#define ZING_RF_SERDES_PATH (1) // 0 : serdes path, 1: rf path

#define PACKET_SUSPEND (0) // packet suspend / resume, 0 : disable, 1 : enable (decrease the speed)

/*
# FX3 보드  / FX3S 보드  설정...
io_cfg.isDQ32Bit = CyTrue;    						/    io_cfg.isDQ32Bit = CyFalse;
io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_DEFAULT;    /    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
#define ZING_GPIF_BUSWIDTH (32) // 8 / 16 / 32      /    #define ZING_GPIF_BUSWIDTH (8) // 8 / 16 / 32
*/


// ======================================================================================
// Threads, Events
// ======================================================================================
// Thread - App
#define APPLICATION_THREAD_STACK	(0x1000)
#define APPLICATION_THREAD_PRIORITY	(8)

//
#define EVT_EP0                 (1 << 1)

//
#define EVT_CTLCH0                 (1 << 1)

// ======================================================================================
// DMA
// ======================================================================================




// ======================================================================================
// USB
// ======================================================================================
// Ep
#define CY_FX_EP_PRODUCER               0x01    /* EP 1 OUT, ZING Control channel */
#define CY_FX_EP_CONSUMER               0x81    /* EP 1 IN */
#define CY_FX_EP_PRODUCER_2             0x02    /* EP 2 OUT, ZING Data channel */
#define CY_FX_EP_CONSUMER_2             0x82    /* EP 2 IN */

#define CY_FX_DATA_BURST_LENGTH         (8)     /* Number of Burst for the Data. USB 3.0 only, fix 8 in ZING */



// ======================================================================================
// PIB
// ======================================================================================



// ======================================================================================
// etc, DBG
// ======================================================================================
#define Elements(X)		(sizeof(X)/sizeof(X[0]))
#define DBG_LEVEL		(2)
// Print a value less than or equal to DBG_LEVEL
#define DBG_TYPE_UART			(2)
#define DBG_TYPE_ZING_DBG		(3)
#define DBG_TYPE_ZING_TR		(3)
#define DBG_TYPE_I2C			(2)
#define DBG_TYPE_TMP1			(500)

typedef enum DMA_MODE_T
{
    DMA_NORMAL = 0,
    DMA_SYNC,
    DMA_LP,
    DMA_SINKSOURCE,
} dma_mode_t;


// ======================================================================================
// extern func
// ======================================================================================
void USBEP0RxThread(uint32_t Value);
void AppStart(void);
void AppStop(void);
void ApplicationThread(uint32_t Value);
void ControlChThread(uint32_t Value);
CyU3PReturnStatus_t ControlChThread_Create(void);

// ======================================================================================
// extern val
// ======================================================================================

extern CyBool_t glIsApplnActive;

// extern dma
extern CyU3PDmaChannel glDMAControlOut;   /* DMA channel : USB to GPIF, Control OUT */
extern CyU3PDmaChannel glDMAControlIn;    /* DMA channel : USB to GPIF, Control IN */
extern CyU3PDmaChannel glDMADataOut;      /* DMA channel : USB to GPIF, Data OUT */
extern CyU3PDmaChannel glDMADataIn;       /* DMA channel : USB to GPIF, Data IN */

// extern ep0
extern CyU3PEvent      glEp0Event;                 /* Event group used to signal the thread. */
extern uint32_t glHostReqNum;
extern uint8_t glHostRxData[128];
extern uint32_t glHostRxData_idx;
extern uint8_t glHostTxData[128];
extern uint32_t glHostTxData_idx;

// zing control ch thread
extern CyU3PEvent      glControlChEvent;
extern uint8_t glControlChData[512];
extern uint32_t glControlChData_idx;


#include "cyu3externcend.h"
#endif /* _APPLICATION_H_ */

