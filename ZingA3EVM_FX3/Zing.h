
#ifndef _ZING_
#define _ZING_

extern CyU3PReturnStatus_t Zing_Init(void);
extern CyU3PReturnStatus_t Zing_PLLConfig(void);
extern void Zing_AllocBuffer(void);
extern CyU3PReturnStatus_t Zing_RegWrite(uint16_t addr, uint8_t* buf, uint16_t len);
extern CyU3PReturnStatus_t Zing_Header(
        uint8_t *pt,            /* pt : buffer pointer */
        uint16_t payload_size,  /* payload_size : memory size in bytes */
        uint16_t addr,          /* addr : zing internal address , address unit = 32bit */
        uint16_t type           /* type : Read/write */);
extern CyU3PReturnStatus_t Zing_Transfer_Send (
	CyU3PDmaChannel* dma_ch,
    uint8_t     *data,		/* pointer to msg */
    uint32_t    length	 	/* msg size in bytes */
    );
extern CyU3PReturnStatus_t Zing_Transfer_Recv (
	CyU3PDmaChannel* dma_ch,
    uint8_t     *data,		/* pointer to msg */
    uint32_t    length	 	/* msg size in bytes */
    );
extern CyU3PReturnStatus_t Zing_RegRead(uint16_t addr, uint8_t* buf, uint16_t len);
extern void Zing_RegReadFlush(void);
extern void Zing_AFC(void);
extern void Zing_AFC2(float f_tg);
extern void Zing_SetHRCP(uint32_t val);
extern void Zing_SetPath(uint32_t val);
extern CyU3PReturnStatus_t Zing_SendMsg(uint8_t* buf, uint32_t len);
extern CyU3PReturnStatus_t Zing_RecvMsg(uint8_t* buf, uint32_t* len_pt);
extern void Zing_Reset(uint8_t type);

extern CyU3PReturnStatus_t Zing_DataWrite(uint8_t* buf, uint32_t len);
extern CyU3PReturnStatus_t Zing_DataWrite2(uint8_t* buf, uint32_t len);
extern CyU3PReturnStatus_t Zing_Transfer_Send2 (
		CyU3PDmaChannel* dma_ch,
	    uint8_t     *data,		/* pointer to msg */
	    uint32_t    length	 	/* msg size in bytes */
	    );

extern CyU3PReturnStatus_t Zing_DataRead(uint8_t* buf, uint32_t* len_pt);
extern CyU3PReturnStatus_t Zing_Transfer_Recv2 (
		CyU3PDmaChannel* dma_ch,
	    uint8_t     *data,		/* pointer to msg */
	    uint32_t*    length_pt	 	/* msg size in bytes */
	    );

extern void Zing_DataReadFlush(void);
extern CyU3PReturnStatus_t Zing_AutoHRCP(void);

extern uint32_t Zing_GetVersion(void);
extern void Zing_SetGPIFBusWidth(uint8_t width);

extern void Zing_Test_DataTx (uint32_t repeat, uint32_t length, uint32_t pattern);
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

extern CyU3PReturnStatus_t Zing_Header2(
        uint8_t *pt,            /* pt : buffer pointer */
        uint16_t dir,
        uint16_t interrupt,
        uint16_t target,
        uint16_t type,
        uint16_t req_resp,
        uint16_t fr_type,
        uint16_t intr_flags,
        uint16_t addr,
        uint16_t payload_size);

// DMA override mode buffers
extern uint8_t *glZingControlInBuffer;
extern uint8_t *glZingControlOutBuffer;
extern uint8_t *glZingDataInBuffer;
extern uint8_t *glZingDataOutBuffer;

#endif   /* _INCLUDED__ */
