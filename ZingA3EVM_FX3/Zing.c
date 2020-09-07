#include "Application.h"
#include "Zing_internal.h"
#include "Support.h"
#include "PIB.h"
#include <math.h>

// DMA override mode buffers
uint8_t *glZingControlInBuffer;
uint8_t *glZingControlOutBuffer;
uint8_t *glZingDataInBuffer;
uint8_t *glZingDataOutBuffer;

// ZING mode
uint32_t zing_hrcp = PPC;

CyU3PReturnStatus_t Zing_Init(void)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
	uint32_t reg_val;
	uint32_t rt_reg_val;

	// init pll through i2c
	CyU3PDebugPrint (4, "[Init/Zing/PLL]...\r\n");
	apiRetStatus = Zing_PLLConfig();
	if(apiRetStatus!=CY_U3P_SUCCESS) return apiRetStatus;
	CyU3PDebugPrint (4, "[Init/Zing/PLL] done\r\n");

	// allocate buffer
	Zing_AllocBuffer();

	// GPIF Bus width
	Zing_SetGPIFBusWidth(ZING_GPIF_BUSWIDTH);

	// init rf/serdes
	CyU3PDebugPrint (4, "[init/Zing/RF,Serdes]...\r\n");
	reg_val = 0x00000000;
	Zing_RegWrite(0x8009,(uint8_t*)&reg_val,4);
	Zing_RegWrite(0x8024,(uint8_t*)&reg_val,4);
	Zing_RegWrite(0x8025,(uint8_t*)&reg_val,4);
	Zing_RegWrite(0x8026,(uint8_t*)&reg_val,4);
	Zing_RegWrite(0x802C,(uint8_t*)&reg_val,4);
	Zing_RegWrite(0x802D,(uint8_t*)&reg_val,4);

	reg_val = 0x0001DFFF;
	Zing_RegWrite(0x8028,(uint8_t*)&reg_val,4);
	reg_val = 0x00FE011F;
	Zing_RegWrite(0x8027,(uint8_t*)&reg_val,4);
	reg_val = 0x000AA666;
	Zing_RegWrite(0x8024,(uint8_t*)&reg_val,4);
	reg_val = 0x0003FF59;
	Zing_RegWrite(0x8025,(uint8_t*)&reg_val,4);
	reg_val = 0x000000EF;
	Zing_RegWrite(0x8026,(uint8_t*)&reg_val,4);

	reg_val = 0x00000000;
	Zing_RegWrite(0x8009,(uint8_t*)&reg_val,4); Zing_RegRead(0x8009,(uint8_t*)&rt_reg_val,4);
	reg_val = 0x00000008;
	Zing_RegWrite(0x802C,(uint8_t*)&reg_val,4); Zing_RegRead(0x802C,(uint8_t*)&rt_reg_val,4);
	reg_val = 0x00000001;
	Zing_RegWrite(0x800E,(uint8_t*)&reg_val,4); Zing_RegRead(0x800E,(uint8_t*)&rt_reg_val,4);
	reg_val = 0x88C8A33D;
	Zing_RegWrite(0x802C,(uint8_t*)&reg_val,4); Zing_RegRead(0x802C,(uint8_t*)&rt_reg_val,4);
	reg_val = 0xFF888888;
	Zing_RegWrite(0x802F,(uint8_t*)&reg_val,4);
	reg_val = 0x9224F0F5;
	Zing_RegWrite(0x802D,(uint8_t*)&reg_val,4);
	reg_val = 0x488F73;
	Zing_RegWrite(0x802E,(uint8_t*)&reg_val,4);
	reg_val = 0x788F73;
	Zing_RegWrite(0x802E,(uint8_t*)&reg_val,4);

	// AFC
	CyU3PDebugPrint (4, "[init/Zing/AFC]...\r\n");
    //Zing_AFC(); // AFC : Automatic Frequency Controller
	Zing_AFC2(1.25*1000000000); // AFC : Automatic Frequency Controller
    CyU3PDebugPrint (4, "[init/Zing/AFC] done\r\n");

#if ZING_RF_SERDES_PATH == 0
	reg_val = 0x88C8A3BF; // serdes path
	Zing_RegWrite(0x802C,(uint8_t*)&reg_val,4);
#elif ZING_RF_SERDES_PATH == 1
	reg_val = 0x88C8A3DF; // rf path
	Zing_RegWrite(0x802C,(uint8_t*)&reg_val,4);
#endif
	CyU3PDebugPrint (4, "[init/Zing/RF,Serdes] done\r\n");

    // init Modem
    CyU3PDebugPrint (4, "[init/Zing/Modem]...\r\n");
	reg_val = REG_HW_CFG_INIT_STAGE0;
	Zing_RegWrite(REG_HW_CFG,(uint8_t*)&reg_val,4);
	reg_val = REG_IFS_PPC_INIT;
	Zing_RegWrite(REG_IFS,(uint8_t*)&reg_val,4);
	reg_val = REG_SUPERFRAME_INIT;
	Zing_RegWrite(REG_SUPERFRAME_CFG,(uint8_t*)&reg_val,4);
	reg_val = REG_PPID_INIT;
	Zing_RegWrite(REG_PPID,(uint8_t*)&reg_val,4);
	reg_val = REG_PHY_CONTROL_INIT;
	Zing_RegWrite(REG_PHY_CTRL,(uint8_t*)&reg_val,4);
	reg_val = REG_PLL_SERDES_INIT2;
	Zing_RegWrite(REG_PLL_CTRL_SERDES,(uint8_t*)&reg_val,4);
	reg_val = REG_DEVID_INIT;
	Zing_RegWrite(REG_DEVICE_ID,(uint8_t*)&reg_val,4);
	reg_val = REG_MAC_TIMEOUT_INIT;
	Zing_RegWrite(REG_MAC_TIMEOUT_CFG,(uint8_t*)&reg_val,4);
	reg_val = REG_PHY_TIMEOUT_INIT;
	Zing_RegWrite(REG_PHY_TIMEOUT_CFG,(uint8_t*)&reg_val,4);
	reg_val = REG_RETRANSMIT_LIMIT_INIT;
	Zing_RegWrite(REG_MAC_RETX_LIMIT,(uint8_t*)&reg_val,4);
	reg_val = REG_HW_CFG_INIT_STAGE1;
	Zing_RegWrite(REG_HW_CFG,(uint8_t*)&reg_val,4);
	reg_val = REG_HW_CFG_INIT_STAGE2;
	Zing_RegWrite(REG_HW_CFG,(uint8_t*)&reg_val,4);
    CyU3PDebugPrint (4, "[init/Zing/Modem] done\r\n");

	Zing_RegRead(REG_RTL_VERSION,(uint8_t*)&rt_reg_val,4);
	CyU3PDebugPrint (4, "[Zing] RTL version : 0x%x\r\n",rt_reg_val);

	Zing_RegRead(REG_HW_CFG,(uint8_t*)&rt_reg_val,4);
	CyU3PDebugPrint (4, "[Zing] HRCP : %s\r\n",rt_reg_val&0x00000010 ? "PPC" : "DEV");
	CyU3PDebugPrint (4, "[Zing] Data mode : %s\r\n",rt_reg_val&0x01000000 ? "MSDU only mode" : "Header mode");

	return apiRetStatus;
}

CyU3PReturnStatus_t Zing_PLLConfig(void)
{
	CyU3PReturnStatus_t apiRetStatus;
	uint8_t buffer[] = {0x6F,0x00,0x00,0x00,0x00,0x90,0x01,0x00,0x00,0x00,0x00};
	uint8_t rxbuffer[11] = {0,};
	uint8_t devAddr = I2C_DeviceAddress;
	uint8_t regAddr = 0x00;

	apiRetStatus = I2C_Write(devAddr,regAddr,buffer,sizeof(buffer));
	if(apiRetStatus!=CY_U3P_SUCCESS) return apiRetStatus;

	apiRetStatus = I2C_Read(devAddr,regAddr,rxbuffer,sizeof(rxbuffer));

#if DBG_LEVEL >= DBG_TYPE_I2C
	CyU3PDebugPrint(4, "[I2C/RD] ");
	for(uint8_t i=0;i<sizeof(rxbuffer);i++) CyU3PDebugPrint(4, "0x%X ",rxbuffer[i]);
	CyU3PDebugPrint(4, "\r\n");
#endif

	return apiRetStatus;
}

void Zing_AllocBuffer(void)
{
    /* DMA override mode buffers */
    glZingControlInBuffer = (uint8_t *)CyU3PDmaBufferAlloc (512);
    glZingControlOutBuffer = (uint8_t *)CyU3PDmaBufferAlloc (512);
    glZingDataInBuffer = (uint8_t *)CyU3PDmaBufferAlloc (8192);
    glZingDataOutBuffer = (uint8_t *)CyU3PDmaBufferAlloc (8192);
}



CyU3PReturnStatus_t Zing_RegWrite(uint16_t addr, uint8_t* buf, uint16_t len)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    if(len < 4)
    {
#if DBG_LEVEL >= DBG_TYPE_ZING_DBG
        CyU3PDebugPrint(4, "must be greater than or equal to 4 bytes\n\r");
#endif
        return CY_U3P_ERROR_BAD_SIZE;
    }

    if(len > 128)
    {
#if DBG_LEVEL >= DBG_TYPE_ZING_DBG
        CyU3PDebugPrint(4, "Can't read too big size of memory.(must be less than 512 bytes)\n\r");
#endif
        return CY_U3P_ERROR_BAD_SIZE;
    }

	Zing_Header(glZingControlOutBuffer,len,addr,ZING_HDR_ACTION_WRITE);
	memcpy(glZingControlOutBuffer+ZING_HDR_SIZE, buf, len);
	apiRetStatus = Zing_Transfer_Send2(&glDMAControlOut,glZingControlOutBuffer,len+ZING_HDR_SIZE);
#if DBG_LEVEL >= DBG_TYPE_ZING_TR
	{
		int i;
		CyU3PDebugPrint(4,"[Zing/RegWrite] (0x%X) ",addr);
		for(i=0;i<len;i++) {
			CyU3PDebugPrint(4,"0x%X, ",buf[i]);
		}
		CyU3PDebugPrint(4,"\r\n");
	}
#endif

	return apiRetStatus;
}

void Zing_Header(
        uint8_t *pt,            /* pt : buffer pointer */
        uint16_t payload_size,  /* payload_size : memory size in bytes */
        uint16_t addr,          /* addr : zing internal address , address unit = 32bit */
        uint16_t type           /* type : Read/write */)
{
	ZingHdr_t *p_hdr;
	memset(pt,0,ZING_HDR_SIZE);

	p_hdr = (ZingHdr_t *)pt;
	p_hdr->dir = ZING_HDR_DIR_EGRESS;
	p_hdr->target = ZING_HDR_TARGET_REG;
	p_hdr->type = type;
	p_hdr->addr = addr;
	p_hdr->length = payload_size;
}

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
        uint16_t payload_size)
{
	ZingHdr_t *p_hdr;
	memset(pt,0,ZING_HDR_SIZE);
	p_hdr = (ZingHdr_t *)pt;

	p_hdr->dir = dir;
	p_hdr->interrupt = interrupt;
	p_hdr->target = target;
	p_hdr->type = type;
	p_hdr->req_resp = req_resp;
	p_hdr->fr_type = fr_type;
	p_hdr->intr_flags = intr_flags;
	p_hdr->addr = addr;
	p_hdr->length = payload_size;
}

CyU3PReturnStatus_t Zing_RegRead(uint16_t addr, uint8_t* buf, uint16_t len)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	uint32_t	evStat;

    if(len < 4)
    {
#if DBG_LEVEL >= DBG_TYPE_ZING_DBG
        CyU3PDebugPrint(4, "must be greater than or equal to 4 bytes\n\r");
#endif
        return CY_U3P_ERROR_BAD_SIZE;
    }

    if(len > 128)
    {
#if DBG_LEVEL >= DBG_TYPE_ZING_DBG
        CyU3PDebugPrint(4, "Can't read too big size of memory.(must be less than 512 bytes)\n\r");
#endif
        return CY_U3P_ERROR_BAD_SIZE;
    }

    Zing_Header(glZingControlOutBuffer,len,addr,ZING_HDR_ACTION_READ);
    status = Zing_Transfer_Send2(&glDMAControlOut,glZingControlOutBuffer,ZING_HDR_SIZE);

    status = CyU3PEventGet (&glControlChEvent, EVT_CTLCH0, CYU3P_EVENT_OR_CLEAR, &evStat, CYU3P_WAIT_FOREVER);
	// copy data only (except zing header(8Byte))
	memcpy(buf,glControlChData+8,glControlChData_idx-8);

#if DBG_LEVEL >= DBG_TYPE_ZING_TR
	{
		int i;
		CyU3PDebugPrint(4,"[Zing/RegRead] (0x%X) ",addr);
		for(i=0;i<len;i++) {
			CyU3PDebugPrint(4,"0x%X, ",buf[i]);
		}
		CyU3PDebugPrint(4,"\r\n");
	}
#endif

	return status;
}

// f_tg : Hz
void Zing_AFC2(float f_tg)
{
	 uint32_t	reg_value, reg_val;
	 uint32_t CntArr[AFC_N] = {0,};
	 int i;
	 float f_ref = 25000000;
	 uint32_t ref_cnt = 32768;
	 uint8_t selected_idx = 0;
	 int min_dif_cnt=0;
	 int DifCntArr[AFC_N] = {0,};
	 uint32_t reg_value2[AFC_N] ={
			 0x12218091,
			 0x12228091,
			 0x12238091,
			 0x12248091,
			 0x12258091,
			 0x12268091,
			 0x12278091,
			 0x12288091
	 };
	 uint32_t data;
	 float f_set;

	 uint32_t t1,t2;
	 t1 = CyU3PGetTime();


	 // calc ref_cnt
	 ref_cnt = (uint32_t)(f_ref/f_tg*50*pow(2,15));

	 // set reg_value2
	 for(i=0;i<AFC_N;i++) {
		 reg_value2[i] = (0x12008091) | (i<<16);
	 }


	// init
	reg_value=0x0000001F;
	Zing_RegWrite(0x802C,(uint8_t*)&reg_value,4);

	reg_value=0x00000088;
	Zing_RegWrite(0x802F,(uint8_t*)&reg_value,4);

	reg_value=0x1221809B;
	Zing_RegWrite(0x802D,(uint8_t*)&reg_value,4);

	// measure full mode AFC
	for(i=0; i<AFC_N; i++) {
		reg_value = reg_value2[i];
		Zing_RegWrite(0x802D,(uint8_t*)&reg_value,4);

		reg_value=0x00000000;
		Zing_RegWrite(0x802B,(uint8_t*)&reg_value,4);

		reg_value=0x00010000;
		Zing_RegWrite(0x802B,(uint8_t*)&reg_value,4);

		CyU3PThreadSleep (10);

		Zing_RegRead(0x802B,(uint8_t*)&data,4);
		CntArr[i] = data & 0x0000ffff;
	}

	// select vco tb
	for(i=0; i<AFC_N; i++) {
		DifCntArr[i] = abs((int)CntArr[i]-(int)ref_cnt);
	}
	min_dif_cnt = DifCntArr[0];
	selected_idx = 0;
	for(i=1; i<AFC_N; i++) {
		if(DifCntArr[i] < min_dif_cnt) {
			min_dif_cnt = DifCntArr[i];
			selected_idx = i;
		}
	}

	// write reg
	reg_val = 0x00000000;
	Zing_RegWrite(0x8009,(uint8_t*)&reg_val,4);
	reg_val = 0x00000008;
	Zing_RegWrite(0x802C,(uint8_t*)&reg_val,4);
	reg_val = 0x00000001;
	Zing_RegWrite(0x800E,(uint8_t*)&reg_val,4);
	reg_val = 0x88C8A33D;
	Zing_RegWrite(0x802C,(uint8_t*)&reg_val,4);
	reg_val = 0xFF888888;
	Zing_RegWrite(0x802F,(uint8_t*)&reg_val,4);

	reg_value=(0x9224F0F5 & 0xFF00FFFF) | (reg_value2[selected_idx] & 0x00FF0000);
	Zing_RegWrite(0x802D,(uint8_t*)&reg_value,4);
	Zing_RegRead(0x802D,(uint8_t*)&data,4);

	reg_val = 0x488F73;
	Zing_RegWrite(0x802E,(uint8_t*)&reg_val,4);
	reg_val = 0x788F73;
	Zing_RegWrite(0x802E,(uint8_t*)&reg_val,4);

	t2 = CyU3PGetTime();
	// print dbg
	CyU3PDebugPrint (4, "elapsed time :%d ms\r\n", t2-t1);

#if DBG_LEVEL >= DBG_TYPE_ZING
	for(i=0;i<AFC_N;i++) {
		CyU3PDebugPrint (4, "cnt[%d] :%d\r\n", i, CntArr[i]);
	}
	CyU3PDebugPrint (4, "set reg 802D :0x%x\r\n", reg_value);
	f_set = (f_ref/((float)CntArr[selected_idx])*50*pow(2,15));
	CyU3PDebugPrint (4, "vco freq :%d Hz\r\n", (int)f_set);

#endif

}




// val = 1 (PPC), val = 0 (DEV)
void Zing_SetHRCP(uint32_t val)
{
	uint32_t rt_reg_val, reg_val;
	HW_CFG *TempReg_pt;

	//DMA_Sync_mode();

	// PPC/DEV
	Zing_RegRead(REG_HW_CFG,(uint8_t*)&rt_reg_val,4);
	TempReg_pt = (HW_CFG*)&rt_reg_val;
	TempReg_pt->ppc_mode = val;
	Zing_RegWrite(REG_HW_CFG,(uint8_t*)&rt_reg_val,4);

	// IFS
	if(val == 1) {//PPC
		reg_val = REG_IFS_PPC_INIT;
		Zing_RegWrite(REG_IFS,(uint8_t*)&reg_val,4);
	} else { //DEV
		reg_val = REG_IFS_DEV_INIT;
		Zing_RegWrite(REG_IFS,(uint8_t*)&reg_val,4);
	}

#if DBG_LEVEL >= DBG_TYPE_ZING
	CyU3PDebugPrint (4, "[Zing] HRCP : %s\r\n",val ? "PPC" : "DEV");
#endif

	//DMA_Normal_mode();
}

// val = 1 (RF), val = 0 (SERDES)
void Zing_SetPath(uint32_t val)
{
	uint32_t rt_reg_val;

	//DMA_Sync_mode();

	// PPC/DEV
	Zing_RegRead(0x802c,(uint8_t*)&rt_reg_val,4);
	rt_reg_val &= 0xFFFFFF9F;
	if(val==1) { //rf path
		rt_reg_val |= 0x00000040;
	}
	else if(val ==0) { //serdes path
		rt_reg_val |= 0x00000020;
	}
	Zing_RegWrite(0x802c,(uint8_t*)&rt_reg_val,4);

	//DMA_Normal_mode();
}




// in DMA_Sync_mode
CyU3PReturnStatus_t Zing_SendMsg(uint8_t* buf, uint32_t len)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

	apiRetStatus = Zing_DataWrite(buf,len);
#if DBG_LEVEL >= DBG_TYPE_ZING_TR
	if (apiRetStatus == CY_U3P_SUCCESS) {
		CyU3PDebugPrint(4,"[Zing] (send) %s \r\n",buf);
	} else {
		CyU3PDebugPrint(4,"[Zing] (send : failed) \r\n",buf);
	}
#endif
	return apiRetStatus;
}

// in DMA_Sync_mode
CyU3PReturnStatus_t Zing_RecvMsg(uint8_t* buf, uint32_t* len_pt)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

	apiRetStatus = Zing_DataRead(buf,len_pt);
#if DBG_LEVEL >= DBG_TYPE_ZING_TR
	if (apiRetStatus == CY_U3P_SUCCESS) {
		CyU3PDebugPrint(4,"[Zing] (recv) %s \r\n",buf);
	} else {
		CyU3PDebugPrint(4,"[Zing] (recv : failed) \r\n",buf);
	}
#endif
	return apiRetStatus;
}

// type = 0 (S/W reset), type = 1 (H/W reset)
void Zing_Reset(uint8_t type)
{
	uint32_t rt_reg_val;
	HW_CFG *TempReg_pt;

	if(type == 0) {

		//DMA_Sync_mode();

		//
		Zing_RegRead(REG_HW_CFG,(uint8_t*)&rt_reg_val,4);
		TempReg_pt = (HW_CFG*)&rt_reg_val;
		TempReg_pt->init_n = 0;
		Zing_RegWrite(REG_HW_CFG,(uint8_t*)&rt_reg_val,4);

		CyU3PThreadSleep (100);

		TempReg_pt->init_n = 1;
		Zing_RegWrite(REG_HW_CFG,(uint8_t*)&rt_reg_val,4);

		//DMA_Normal_mode();
	}
	else if(type == 1) {

	}
}

CyU3PReturnStatus_t Zing_DataWrite(uint8_t* buf, uint32_t len)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

	memcpy(glZingDataOutBuffer, buf, len);
	apiRetStatus = Zing_Transfer_Send2(&glDMADataOut,glZingDataOutBuffer,len);
#if DBG_LEVEL >= DBG_TYPE_ZING_TR
	if(apiRetStatus==CY_U3P_SUCCESS) {
		{
			int i;
			CyU3PDebugPrint(4,"[Zing/DataWrite] ");
			for(i=0;i<len;i++) {
				CyU3PDebugPrint(4,"0x%X, ",buf[i]);
			}
			CyU3PDebugPrint(4,"\r\n");
		}
	}
#endif

		return apiRetStatus;
}

CyU3PReturnStatus_t Zing_DataWrite2(uint8_t* buf, uint32_t len)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

	apiRetStatus = Zing_Transfer_Send2(&glDMADataOut,buf,len);

	return apiRetStatus;
}

CyU3PReturnStatus_t Zing_Transfer_Send2 (
	CyU3PDmaChannel* dma_ch,
    uint8_t     *data,		/* pointer to msg */
    uint32_t    length	 	/* msg size in bytes */
    )
{
    CyU3PReturnStatus_t status;
    CyU3PDmaBuffer_t Buf;


    /* Wait for a free buffer to transmit the received data. The failure cases are same as above. */
    status = CyU3PDmaChannelGetBuffer (dma_ch, &Buf, ZING_HW_TIMEOUT);
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Zing_Transfer_Send2,CyU3PDmaChannelGetBuffer failed(0x%x)\n", status);
    }
    else {
		CyU3PMemCopy (Buf.buffer, data, length);
		Buf.count = length;

		status = CyU3PDmaChannelCommitBuffer (dma_ch, Buf.count, 0);
		if (status != CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint (4, "Zing_Transfer_Send2,CyU3PDmaChannelCommitBuffer failed(0x%x)\n", status);
		}
    }

    return status;
}

CyU3PReturnStatus_t Zing_DataRead(uint8_t* buf, uint32_t* len_pt)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

	apiRetStatus = Zing_Transfer_Recv2(&glDMADataIn,glZingDataInBuffer,len_pt);

	if(apiRetStatus==CY_U3P_SUCCESS)
		memcpy(buf,glZingDataInBuffer,*len_pt);
	else
		*len_pt = 0;

#if DBG_LEVEL >= DBG_TYPE_ZING_TR
	if(apiRetStatus==CY_U3P_SUCCESS) {
		{
			int i;
			CyU3PDebugPrint(4,"[Zing/DataRead] ");
			for(i=0;i<*len_pt;i++) {
				CyU3PDebugPrint(4,"0x%X, ",buf[i]);
			}
			CyU3PDebugPrint(4,"\r\n");
		}
	}
#endif

	return apiRetStatus;
}

CyU3PReturnStatus_t Zing_Transfer_Recv2 (
	CyU3PDmaChannel* dma_ch,
    uint8_t     *data,		/* pointer to msg */
    uint32_t*    length_pt	 	/* msg size in bytes */
    )
{
	CyU3PReturnStatus_t status;
	CyU3PDmaBuffer_t Buf;


	/* Wait for a free buffer to transmit the received data. The failure cases are same as above. */
	status = CyU3PDmaChannelGetBuffer (dma_ch, &Buf, ZING_HW_TIMEOUT);
	if (status != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "Zing_Transfer_Recv2,CyU3PDmaChannelGetBuffer failed(0x%x)\n", status);
	}
	else {
		CyU3PMemCopy (data, Buf.buffer, Buf.count);
		*length_pt = Buf.count;

		status = CyU3PDmaChannelDiscardBuffer (dma_ch);
		if (status != CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint (4, "Zing_Transfer_Recv2,CyU3PDmaChannelCommitBuffer failed(0x%x)\n", status);
		}
	}

	return status;
}

void Zing_DataReadFlush(void)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
    uint8_t  *in_buffer;
    uint32_t len;

    in_buffer = (uint8_t *)CyU3PDmaBufferAlloc (8192);

	while(1) {
		status = Zing_DataRead(in_buffer,&len);
		if(status != CY_U3P_SUCCESS) {
			break;
		} else {
			CyU3PDebugPrint (4, "data flushed \r\n");
		}
		CyU3PThreadSleep(500);
	}
    CyU3PDmaBufferFree(in_buffer);
}

CyU3PReturnStatus_t Zing_AutoHRCP(void)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	uint32_t i;
	char* tx_msg = "Hi~!";
	char rx_msg[100] = {0,};
	uint32_t rx_len;
	uint32_t fail_cnt1, fail_cnt2;
	uint32_t rt_reg_val;

#if DBG_LEVEL >= DBG_TYPE_ZING
	CyU3PDebugPrint (4, "[Zing/AutoPpcDev] Start\r\n");
#endif
	fail_cnt1=0;
	fail_cnt2=0;
	goto STATE1;

STATE1 :
#if DBG_LEVEL >= DBG_TYPE_ZING_DBG
CyU3PDebugPrint (4, "STATE1\r\n");
#endif
	for(i=0;i<4;i++) {
		status = Zing_SendMsg((uint8_t*)tx_msg, strlen(tx_msg));
		status = Zing_RecvMsg((uint8_t*)rx_msg, &rx_len);
		if(status == CY_U3P_SUCCESS) {
			goto STATE3;
		}
	}
	goto STATE2;

STATE2:
#if DBG_LEVEL >= DBG_TYPE_ZING_DBG
CyU3PDebugPrint (4, "STATE2\r\n");
#endif
	fail_cnt1++;
	if(fail_cnt1 <= 30) {
		Zing_Reset(0);
		CyU3PThreadSleep(RandomGen_GetNumber());
		if(zing_hrcp == PPC) {
			zing_hrcp = DEV;
			Zing_SetHRCP(DEV);
		}
		else {
			zing_hrcp = PPC;
			Zing_SetHRCP(PPC);
		}
		goto STATE1;
	}
	else {
		goto STATE6;
	}


STATE3:
#if DBG_LEVEL >= DBG_TYPE_ZING_DBG
CyU3PDebugPrint (4, "STATE3\r\n");
#endif
	Zing_Reset(0);
	CyU3PThreadSleep(500);
	for(i=0;i<4;i++) {
		status = Zing_SendMsg((uint8_t*)tx_msg, strlen(tx_msg));
		status = Zing_RecvMsg((uint8_t*)rx_msg, &rx_len);
		if(status != CY_U3P_SUCCESS) {
			goto STATE4;
		}
	}
	goto STATE5;

STATE4:
#if DBG_LEVEL >= DBG_TYPE_ZING_DBG
CyU3PDebugPrint (4, "STATE4\r\n");
#endif
	fail_cnt2++;
	if(fail_cnt2 <= 10) {
		goto STATE1;
	}
	goto STATE6;

STATE5:
#if DBG_LEVEL >= DBG_TYPE_ZING_DBG
CyU3PDebugPrint (4, "STATE5\r\n");
	CyU3PDebugPrint (4, "Success\r\n");
#endif
	goto STATE_END;

STATE6:
#if DBG_LEVEL >= DBG_TYPE_ZING_DBG
CyU3PDebugPrint (4, "STATE6\r\n");
	CyU3PDebugPrint (4, "Failed\r\n");
#endif
	goto STATE_END;

STATE_END:
#if DBG_LEVEL >= DBG_TYPE_ZING_DBG
CyU3PDebugPrint (4, "STATE_END\r\n");
#endif
	Zing_Reset(0);
	Zing_DataReadFlush();

Zing_RegRead(REG_HW_CFG,(uint8_t*)&rt_reg_val,4);
#if DBG_LEVEL >= DBG_TYPE_ZING
CyU3PDebugPrint (4, "[Zing] HRCP : %s\r\n",rt_reg_val&0x00000010 ? "PPC" : "DEV");
#endif

#if DBG_LEVEL >= DBG_TYPE_ZING
	CyU3PDebugPrint (4, "[Zing/AutoPpcDev] End\r\n");
#endif

	return status;
}

// in DMA Sync mode
uint32_t Zing_GetVersion(void)
{
	uint32_t rt_reg_val;
	Zing_RegRead(REG_RTL_VERSION,(uint8_t*)&rt_reg_val,4);

	return rt_reg_val;
}

void Zing_SetGPIFBusWidth(uint8_t width)
{
	switch(width) {
	case 8 :
	    CyU3PGpioSetValue(GPIF_BUSWIDTH_CTL0, CyFalse);
	    CyU3PGpioSetValue(GPIF_BUSWIDTH_CTL1, CyTrue);
	    break;
	case 16 :
	    CyU3PGpioSetValue(GPIF_BUSWIDTH_CTL0, CyTrue);
	    CyU3PGpioSetValue(GPIF_BUSWIDTH_CTL1, CyFalse);
		break;
	case 32 :
	    CyU3PGpioSetValue(GPIF_BUSWIDTH_CTL0, CyFalse);
	    CyU3PGpioSetValue(GPIF_BUSWIDTH_CTL1, CyFalse);
		break;
	}

#if DBG_LEVEL >= DBG_TYPE_ZING
	CyU3PDebugPrint (4, "[Zing/BusWidth] %d\r\n", width);
#endif
}

void Zing_Test_DataTx2 (uint32_t repeat, uint32_t length, uint32_t pattern)
{
	uint8_t* buf = (uint8_t *)CyU3PDmaBufferAlloc (length);
	CyU3PMemSet(buf,(uint8_t)pattern,length);
	uint32_t i;
	uint32_t idx32=0;

    CyU3PDmaBuffer_t Buf;
    CyU3PDmaChannel* dma_ch = &glDMADataOut;

    const uint32_t buf_cnt = 4;
    uint32_t cnt1, cnt2;

    if(repeat <= 4) {
    	cnt1 = repeat;
    	cnt2 = 0;
    }
    else {
    	cnt1 = buf_cnt;
    	cnt2 = repeat-buf_cnt;
    }

	for(i=0; i<cnt1; i++) { // 4: dma buf cnt
	    CyU3PDmaChannelGetBuffer (dma_ch, &Buf, 10000);
	    CyU3PMemCopy (Buf.buffer, buf, length);
	    idx32++;
	    memcpy(Buf.buffer,&idx32,4);
	    Buf.count = length;

		CyU3PDmaChannelCommitBuffer (dma_ch,length, 0);
	}
	if(cnt2 != 0) {
		for(i=0; i<cnt2; i++) {
			CyU3PDmaChannelGetBuffer (dma_ch, &Buf, 10000);
			idx32++;
			memcpy(Buf.buffer,&idx32,4);
			CyU3PDmaChannelCommitBuffer (dma_ch,length, 0);
		}
	}
	CyU3PDmaBufferFree(buf);
}


void Zing_Test_DataSink2 (uint32_t cnt, uint32_t timeout)
{
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	CyU3PDmaChannel* dma_ch = &glDMADataIn;
	CyU3PDmaBuffer_t Buf;

	uint32_t idx32;
	uint8_t rx_pattern;
	uint32_t rx_cnt = 0;
	uint32_t rt_len = 0;
	uint32_t length = 8192;
	uint8_t* buf = (uint8_t *)CyU3PDmaBufferAlloc (length);
	CyU3PMemSet(buf,(uint8_t)0,length);

	uint32_t t1,t2;

	t1 = CyU3PGetTime();

	idx32 = 0;
	while(1) {
		/* Wait for a free buffer to transmit the received data. The failure cases are same as above. */
		status = CyU3PDmaChannelGetBuffer (dma_ch, &Buf, timeout*1000);
		if (status == CY_U3P_SUCCESS)
		{
			//CyU3PMemCopy (buf, Buf.buffer, Buf.count);
			rt_len = Buf.count;
			if(rt_len != 0) {
				memcpy(&idx32,Buf.buffer,4);
				rx_pattern = Buf.buffer[4];
				if(idx32 == rx_cnt+1) {
					rx_cnt++;
				}
				//CyU3PDebugPrint (4, "%d\r\n",idx32);
			}
			status = CyU3PDmaChannelDiscardBuffer (dma_ch);
		}
		else {
			break;
		}
	}

	t2 = CyU3PGetTime()-(timeout*1000);

#if DBG_LEVEL >= DBG_TYPE_ZING
	if (cnt == rx_cnt) {
		CyU3PDebugPrint (4, "Success| expected:%d, received:%d, pattern:%X, elapsed:%d ms\r\n", \
				cnt,rx_cnt, rx_pattern, t2-t1);
	}
	else {
		CyU3PDebugPrint (4, "Failed| expected:%d received:%d, pattern:%X, elapsed:%d ms\r\n", \
				cnt,rx_cnt, rx_pattern, t2-t1);
	}
#endif
}

CyU3PReturnStatus_t Zing_Transfer_Recv3 (
	CyU3PDmaChannel* dma_ch,
    uint8_t     *data,		/* pointer to msg */
    uint32_t*    length_pt	 	/* msg size in bytes */
    )
{
	CyU3PReturnStatus_t status;
	CyU3PDmaBuffer_t Buf;


	/* Wait for a free buffer to transmit the received data. The failure cases are same as above. */
	status = CyU3PDmaChannelGetBuffer (dma_ch, &Buf, CYU3P_WAIT_FOREVER);
	if (status != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "Zing_Transfer_Recv3,CyU3PDmaChannelGetBuffer failed(0x%x)\n", status);
	}
	else {
		CyU3PMemCopy (data, Buf.buffer, Buf.count);
		*length_pt = Buf.count;

		status = CyU3PDmaChannelDiscardBuffer (dma_ch);
		if (status != CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint (4, "Zing_Transfer_Recv3,CyU3PDmaChannelCommitBuffer failed(0x%x)\n", status);
		}
	}

	return status;
}

CyU3PReturnStatus_t Zing_Management_Send (
    uint8_t     *data,		/* pointer to msg */
    uint32_t    length	 	/* msg size in bytes */
    )
{
	CyU3PReturnStatus_t status=CY_U3P_SUCCESS;

	Zing_Header2(glZingControlOutBuffer,0,0,0,1,0,1,0,0,length);
	memcpy(glZingControlOutBuffer+ZING_HDR_SIZE, data, length);
	status = Zing_Transfer_Send2(&glDMAControlOut,glZingControlOutBuffer,length+ZING_HDR_SIZE);

	return status;
}
