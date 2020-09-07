
/* This file contains the externals used by the Keyboard application. */

#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "cyu3types.h"
#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3gpio.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3uart.h"
#include "cyu3i2c.h"
#include "cyu3externcstart.h"
#include "cyu3utils.h"
#include "cyu3usbconst.h"
#include "cyu3usb.h"
#include "cyu3pib.h"

#include "Zing.h"
#include "ZingHw.h"
#include "LPP.h"
#include "PIB.h"
#include "USB_Handler.h"
#include "Support.h"

#include <stdio.h>

// ======================================================================================
// ZING
// ======================================================================================
#define ZING_GPIF_BUSWIDTH (32) // 8 / 16 / 32
#define ZING_RF_SERDES_PATH (1) // 0 : serdes path, 1: rf path

#define ZING_BUG_WM3 (1) // 0 : zlp, 1 : no zlp
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
// I2C, GPIO
// ======================================================================================
// I2C
#define CY_FX_USBI2C_I2C_BITRATE	(100000)

// GPIO
#define GPIO57	(57)
#define GPIF_BUSWIDTH_CTL0	(50)    // 50, 38
#define GPIF_BUSWIDTH_CTL1	(51)    // 51, 37


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
extern void DMA_Sync_mode(void);
extern void
DMA_Normal_CtrlOut_Cb (
        CyU3PDmaChannel     *handle,
        CyU3PDmaCbType_t     evtype,
        CyU3PDmaCBInput_t   *input);
extern void
DMA_Normal_CtrlIn_Cb (
        CyU3PDmaChannel     *handle,
        CyU3PDmaCbType_t     evtype,
        CyU3PDmaCBInput_t   *input);
extern void
DMA_Normal_DataOut_Cb (
        CyU3PDmaChannel     *handle,
        CyU3PDmaCbType_t     evtype,
        CyU3PDmaCBInput_t   *input);
extern void
DMA_Normal_DataIn_Cb (
        CyU3PDmaChannel     *handle,
        CyU3PDmaCbType_t     evtype,
        CyU3PDmaCBInput_t   *input);
extern void DMA_Normal_mode(void);
extern void DMA_LoopBack_mode(void);
extern void DMA_SinkSource_Cb(
        CyU3PDmaChannel   *chHandle, /* Handle to the DMA channel. */
        CyU3PDmaCbType_t  type,      /* Callback type.             */
        CyU3PDmaCBInput_t *input);    /* Callback status.           */
extern void
DMASrcSinkFillInBuffers (
        void);
extern void DMA_SinkSource_mode(void);
extern void USBEP0RxThread(uint32_t Value);
extern void
AppStart (
        void);
extern void
AppStop (
        void);
extern void ApplicationThread(uint32_t Value);

extern void ControlChThread(uint32_t Value);
extern CyU3PReturnStatus_t ControlChThread_Create(void);


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

