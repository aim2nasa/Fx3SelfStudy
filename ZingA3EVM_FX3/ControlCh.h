#ifndef __CONTROL_CH_H__
#define __CONTROL_CH_H__

#include "cyu3types.h"
#include "cyu3os.h"

#define EVT_CTLCH0		(1 << 1)

CyU3PEvent ControlChEvent;
uint8_t ControlChData[512];
uint32_t ControlChData_idx;
uint32_t MngtData;
CyU3PThread ControlChThreadHandle;

void ControlChThread(uint32_t Value);
CyU3PReturnStatus_t ControlChThread_Create(void);

#endif
