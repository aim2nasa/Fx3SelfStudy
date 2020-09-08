#ifndef __ZING_INTERNAL_H__
#define __ZING_INTERNAL_H__

CyU3PReturnStatus_t Zing_PLLConfig(void);
void Zing_AllocBuffer(void);
void Zing_Header(
		uint8_t *pt,            /* pt : buffer pointer */
        uint16_t payload_size,  /* payload_size : memory size in bytes */
        uint16_t addr,          /* addr : zing internal address , address unit = 32bit */
        uint16_t type           /* type : Read/write */);

CyU3PReturnStatus_t Zing_DataWrite(uint8_t* buf, uint32_t len);
CyU3PReturnStatus_t Zing_DataWrite2(uint8_t* buf, uint32_t len);
CyU3PReturnStatus_t Zing_Transfer_Send2 (
		CyU3PDmaChannel* dma_ch,
	    uint8_t     *data,		/* pointer to msg */
	    uint32_t    length	 	/* msg size in bytes */
	    );

CyU3PReturnStatus_t Zing_DataRead(uint8_t* buf, uint32_t* len_pt);
CyU3PReturnStatus_t Zing_Transfer_Recv2 (
		CyU3PDmaChannel* dma_ch,
	    uint8_t     *data,		/* pointer to msg */
	    uint32_t*    length_pt	/* msg size in bytes */
	    );

void Zing_DataReadFlush(void);
void Zing_SetGPIFBusWidth(uint8_t width);

void Zing_Header2(
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

#endif
