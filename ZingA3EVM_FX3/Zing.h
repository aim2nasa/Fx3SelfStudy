
#ifndef _ZING_
#define _ZING_

extern CyU3PReturnStatus_t Zing_Init(void);
extern CyU3PReturnStatus_t Zing_RegWrite(uint16_t addr, uint8_t* buf, uint16_t len);
extern CyU3PReturnStatus_t Zing_RegRead(uint16_t addr, uint8_t* buf, uint16_t len);
extern void Zing_AFC2(float f_tg);
extern void Zing_SetHRCP(uint32_t val);
extern void Zing_SetPath(uint32_t val);
extern CyU3PReturnStatus_t Zing_SendMsg(uint8_t* buf, uint32_t len);
extern CyU3PReturnStatus_t Zing_RecvMsg(uint8_t* buf, uint32_t* len_pt);
extern void Zing_Reset(uint8_t type);
extern CyU3PReturnStatus_t Zing_AutoHRCP(void);
extern uint32_t Zing_GetVersion(void);

extern void Zing_Test_DataTx2 (uint32_t repeat, uint32_t length, uint32_t pattern);
extern void Zing_Test_DataSink2 (uint32_t cnt, uint32_t timeout);

extern CyU3PReturnStatus_t Zing_Transfer_Recv3 (
		CyU3PDmaChannel* dma_ch,
	    uint8_t     *data,		/* pointer to msg */
	    uint32_t*    length_pt	 	/* msg size in bytes */
	    );

extern CyU3PReturnStatus_t Zing_Management_Send (
	    uint8_t     *data,		/* pointer to msg */
	    uint32_t    length	 	/* msg size in bytes */
	    );
#endif   /* _INCLUDED__ */
