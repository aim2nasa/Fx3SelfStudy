#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "cyu3types.h"
#include "cyu3dma.h"
#include "AppThread.h"
#include "cyu3externcstart.h"

#define PACKET_SUSPEND				(0) // packet suspend / resume, 0 : disable, 1 : enable (decrease the speed)

//Endpoints
#define CY_FX_EP_PRODUCER			0x01	//EP 1 OUT, ZING Control channel
#define CY_FX_EP_CONSUMER			0x81    //EP 1 IN
#define CY_FX_EP_PRODUCER_2			0x02    //EP 2 OUT, ZING Data channel
#define CY_FX_EP_CONSUMER_2			0x82    //EP 2 IN
#define CY_FX_DATA_BURST_LENGTH		(8)     //Number of Burst for the Data. USB 3.0 only, fix 8 in ZING

//Debug
#define DBG_LEVEL					(2)		// Print a value less than or equal to DBG_LEVEL
#define DBG_TYPE_ZING_DBG			(3)
#define DBG_TYPE_ZING_TR			(3)

//Extern func
void AppStart(void);
void AppStop(void);
void ApplicationThread(uint32_t Value);

CyBool_t IsApplnActive;

#include "cyu3externcend.h"
#endif
