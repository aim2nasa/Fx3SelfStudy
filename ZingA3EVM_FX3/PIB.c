#include "Application.h"

#if ZING_BUG_WM3 == 0

#if ZING_GPIF_BUSWIDTH == 32
	#include "GpifZing32bit.h"
#elif ZING_GPIF_BUSWIDTH == 16
	#include "GpifZing16bit.h"
#elif ZING_GPIF_BUSWIDTH == 8
	#include "GpifZing8bit.h"
#endif

#else

#if ZING_GPIF_BUSWIDTH == 32
	#include "GpifZing32bit_NoZLP.h"
#elif ZING_GPIF_BUSWIDTH == 16
	#include "GpifZing16bit_NoZLP.h"
#elif ZING_GPIF_BUSWIDTH == 8
	#include "GpifZing8bit_NoZLP.h"
#endif

#endif

CyU3PReturnStatus_t PIB_Init(void)
{
	CyU3PPibClock_t pibClock;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

   /* Initialize the p-port block. */
   pibClock.clkDiv = 2;             /* in AN87216, value is 2 */
   pibClock.clkSrc = CY_U3P_SYS_CLK;
   pibClock.isHalfDiv = CyFalse;
   /* Disable DLL for sync GPIF */
   pibClock.isDllEnable = CyFalse;

   apiRetStatus = CyU3PPibInit(CyTrue, &pibClock);
   if (apiRetStatus != CY_U3P_SUCCESS)
   {
	   CyU3PDebugPrint (4, "P-port Initialization failed(0x%x)\n",apiRetStatus);
   }

   /* Load the GPIF configuration for Master mode. */
   apiRetStatus = CyU3PGpifLoad (&CyFxGpifConfig);
   if (apiRetStatus != CY_U3P_SUCCESS)
   {
	   CyU3PDebugPrint (4, "CyU3PGpifLoad failed(0x%x)\n",apiRetStatus);
   }

   CyU3PGpifSocketConfigure (0,CY_U3P_PIB_SOCKET_0,4,CyFalse,1);
   CyU3PGpifSocketConfigure (1,CY_U3P_PIB_SOCKET_1,4,CyFalse,1);
   CyU3PGpifSocketConfigure (2,CY_U3P_PIB_SOCKET_2,4,CyFalse,1);
   CyU3PGpifSocketConfigure (3,CY_U3P_PIB_SOCKET_3,4,CyFalse,1);

   //CyU3PGpifRegisterCallback(CyFxBulkLpApplnGPIFEventCB);	//	refer to AN87216

   /* Start the state machine. */
   apiRetStatus = CyU3PGpifSMStart (START, ALPHA_START);
   if (apiRetStatus != CY_U3P_SUCCESS)
   {
	   CyU3PDebugPrint (4, "CyU3PGpifSMStart failed(0x%x)\n",apiRetStatus);
   }

   return apiRetStatus;
}
