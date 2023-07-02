/****************************************************************************
 *   FileName    : CDIF_Drv.c
 *   Description : 
 ****************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips Inc.
 *   All rights reserved 
 
This source code contains confidential information of Telechips.
Any unauthorized use without a written permission of Telechips including not limited to re-
distribution in source or binary form is strictly prohibited.
This source code is provided "AS IS" and nothing contained in this source code shall 
constitute any express or implied warranty of any kind, including without limitation, any warranty 
of merchantability, fitness for a particular purpose or non-infringement of any patent, copyright 
or other third party intellectual property right. No warranty is made, express or implied, 
regarding the information's accuracy, completeness, or performance. 
In no event shall Telechips be liable for any claim, damages or other liability arising from, out of 
or in connection with this source code or the use in the source code. 
This source code is provided subject to the terms of a Mutual Non-Disclosure Agreement 
between Telechips and Company.
*
****************************************************************************/
/****************************************************************************
* Note.																		*
*	Date: 2007. 06. 25 by Kevin													*
****************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <asm/system.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <mach/irqs.h>
#include <linux/irqreturn.h>
#include <mach/tca_i2s.h>
#include <linux/delay.h>

#include "cdif_drv.h"
#include "cdif_proc.h"
#include "knetlink.h"
#include "SingleBuffer.h"
#include "cdif_api.h"
//#include "cd_filesystem.h"

#define     SEEK_COMPLETION 1
//#define CD_DUMP
//#define	TRAP_DATA

#if SEEK_COMPLETION
#include <linux/completion.h>
char seek_response[2];
DECLARE_COMPLETION(tcc_completion);
#endif

unsigned short CD_ChunkBuf[CDDA_BUFFER_SIZE];			//CD Buffer
dma_addr_t dma_buf_phyaddr = 0; 	//Physical address
dma_addr_t dma_buf_viraddr[2];		//Virtual address

spinlock_t cdif_lock;
extern unsigned int datacd_enable;

static void set_cdif_adma(void);
static void cdif_adma_handler(void);
static irqreturn_t tcc_adma_handler(int irq, void *dev_id);
static int set_cdif_interrupt(void);
static void set_cdbypass_enable(void);
static void set_cdbypass_disable(void);
static void cd_drv_initialize(void);
static void CD_Research_Frame(unsigned long startAddr);
static void Notify_Error(int error);

unsigned short *pGetCDMemory(void)
{
	return CD_ChunkBuf;
}

void set_cdif_enable(unsigned long ulFlag)
{
	volatile unsigned int *pCICR;

	pCICR = (volatile unsigned int *)tcc_p2v(HwCICR);
	
	if (ulFlag == 1UL)
	{
		*pCICR |= (unsigned long)HwCDIF_EN;
	}
	else
	{
	printk("[tcc-debug] %s, [CICR flag:%d]\n",__func__, *pCICR);		
		*pCICR &= ~(unsigned long)HwCDIF_EN;
	}
}

static void set_cdif_adma(void)
{
	volatile PADMA pADMA;
	unsigned bitSet = 0;

	/** 
	 * @author sjpark@cleinsoft
	 * @date 2014/03/06
	 * CDIF Register Address Set - CDIF Audio Channel changed from Audio0 to Audio1 on daudio 3rd board.
	 **/
	pADMA = (volatile PADMA)tcc_p2v(CDIF_BASE_ADDR_ADMA);
	
	BITCLR(pADMA->ChCtrl, Hw31);		//DMA Channel Disable of CDIF Rx 

	printk ("KERNEL : DMA buffer allocate\n");

	dma_buf_viraddr[0] = (dma_addr_t)dma_alloc_writecombine(0, CDDIDMA_BUFFER_SIZE*2, &dma_buf_phyaddr, GFP_KERNEL); 

	if (dma_buf_viraddr[0] == 0)
	{
		printk ("KERNEL : can not allocate DMA memory\n");
	}
	else
	{
		printk ("KERNEL : set_cdif_adma ADDR cdvir = %x, cdphy = %x\n", dma_buf_viraddr[0], dma_buf_phyaddr);

		dma_buf_viraddr[1] = dma_buf_viraddr[0] + CDDIDMA_BUFFER_SIZE;

		#if (CDDIDMA_BUFFER_SIZE == 1024*8)
		// Total buffer size = 1024*16 Bytes
		pADMA->RxCdParam	= (4UL|0xFFFC0000UL);
		#elif (CDDIDMA_BUFFER_SIZE == 1024*4)
		// Total buffer size = 1024*8 Bytes
		pADMA->RxCdParam	= (4UL|0xFFFE0000UL);
		#elif (CDDIDMA_BUFFER_SIZE == 1024*2)
		// Total buffer size = 1024*4 Bytes
		pADMA->RxCdParam	= (4UL|0xFFFF0000UL);
		#elif (CDDIDMA_BUFFER_SIZE == 1024)
		// Total buffer size = 1024*2 Bytes
		pADMA->RxCdParam	= (4UL|0xFFFF8000UL);
		#elif (CDDIDMA_BUFFER_SIZE == 512)
		// Total buffer size = 1024 Bytes
		pADMA->RxCdParam	= (4UL|0xFFFFC000UL);
		#else
		printk ("KERNEL : Buffer setting Error!!\n");
		#endif
		
		// generate interrupt when half of buffer was filled
		pADMA->RxCdTCnt = (((CDDIDMA_BUFFER_SIZE)>>0x01)>>0x03)-1;
		pADMA->RxCdDar = dma_buf_phyaddr;

		bitSet = ( HwZERO
			|Hw29	// HwTransCtrl_RCN_EN
			|Hw27	// CDIF Rx DMA transfer is done with lock transfer
			|Hw19	// HwTransCtrl_CRRPT_EN
			|Hw15	// HwTransCtrl_CRBSIZE_4
			|(Hw6|Hw7)	// HwTransCtrl_CRWSIZE_32
		);
		pADMA->TransCtrl |= bitSet;	

		//pADMA->RptCtrl |= (Hw31|Hw1|Hw0);	// DMA Interrupt occur is occurred when the last DMA Repeated DMA operation
		BITCLR(pADMA->RptCtrl, Hw31);

		bitSet = ( HwZERO
			|Hw31 	// HwChCtrl_CREN_EN
			//|Hw19	// HwChctrl_CDIF LRMODE
			|Hw15	// HwChCtrl_CRDW_16 //???
			|Hw3	// HwChCtrl_CRIEN
		);
		pADMA->ChCtrl |= bitSet;
	}
}

static void cdif_adma_handler(void)
{
	volatile PADMA pADMA;
	unsigned short *sourceAddr;
	unsigned int curReadAddr;
	unsigned short *pSingle;
	SingleBufferState *pBuf;
	unsigned int dataSize;
	
	//printk ("MODULE : %s \n", __FUNCTION__);

	pBuf = &sCDDIBuffer;		//BasicMode == BPLAY or BIDLE

	if (pBuf->lLength == 0)
	{
		memset((unsigned char*)&dma_buf_viraddr[0], 0x00, 2*CDDIDMA_BUFFER_SIZE);
		pBuf->lWritePtr= 0;
		return;
	}

	dataSize = (unsigned int)(pBuf->lLength - pBuf->lWritePtr);

	/** 
	 * @author sjpark@cleinsoft
	 * @date 2014/03/06
	 * CDIF Register Address Set - CDIF Audio Channel changed from Audio0 to Audio1 on daudio 3rd board.
	 **/
	pADMA = (volatile PADMA)tcc_p2v(CDIF_BASE_ADDR_ADMA);
	
	curReadAddr = pADMA->RxCdCdar;	// CDIF Rx Data Current Destination Address
	sourceAddr = (unsigned short *)(dma_buf_phyaddr+CDDIDMA_BUFFER_SIZE);

	if (curReadAddr < (unsigned int)sourceAddr)
	{
		sourceAddr = (unsigned short *)dma_buf_viraddr[1];
	}
	else
	{
		sourceAddr = (unsigned short *)dma_buf_viraddr[0];
	}

	pSingle = (unsigned short *)(pBuf->pSingleBuf + (pBuf->lWritePtr << 1));

	// Need to modify for optimization.

	memcpy(pSingle, sourceAddr, CDDIDMA_BUFFER_SIZE);

	dataSize -= CDDIDMA_BUFFER_SIZE >> 2;
	pBuf->lWritePtr += CDDIDMA_BUFFER_SIZE >> 2;

	if (dataSize == 0)
	{
		pBuf->lWritePtr = 0;
	}

	if ((SingleBufferSpaceAvailable(DAIGetCDDIBuffer()) <= (CDDIDMA_BUFFER_SIZE >> 2)) && datacd_enable)// if cddi buffer full,
	{
		printk("[tcc-debug] %s, CICR disable, buffer full \n", __func__);
		set_cdif_enable(OFF);
		CD_SetErrorStatus(CD_BUF_FULL);
#ifdef CD_DUMP
		printk ("CDIF Memory Full, buffer addr = (0x%p)\n", sCDDIBuffer.pSingleBuf);
#endif	
}
}

static irqreturn_t tcc_adma_handler(int irq, void *dev_id)
{
	/** 
	 * @author sjpark@cleinsoft
	 * @date 2014/03/06
	 * CDIF Register Address Set - CDIF Audio Channel changed from Audio0 to Audio1 on daudio 3rd board.
	 **/
	volatile PADMA pADMA = (volatile PADMA)tcc_p2v(CDIF_BASE_ADDR_ADMA);//BASE_ADDR_ADMA);
	unsigned long reg_temp = 0;                 

	/** 
	 * @author sjpark@cleinsoft
	 * @date 2014/03/09
	 * CDIF dma clearing issue - clear cdif dma only 
	 **/

	// Check Only CDIF Interrupt
	if (pADMA->IntStatus & (Hw7 | Hw3)) {
		//dmaInterruptSource |= DMA_CH_CDIF;
		cdif_adma_handler();
		reg_temp |= (Hw7 | Hw3);
	}	

	if (reg_temp) { 
		pADMA->IntStatus |= reg_temp;		// DMA Interrupt Status Register
	}

	return IRQ_HANDLED;
}


unsigned int cdif_data;

static int set_cdif_interrupt(void)
{
	int ret = -1;

	/** 
	 * @author sjpark@cleinsoft
	 * @date 2014/03/06
	 * CDIF Register Address Set - CDIF Audio Channel changed from Audio0 to Audio1 on daudio 3rd board.
	 **/
	ret = request_irq(INT_ADMA, tcc_adma_handler, IRQF_SHARED, CDIF_DEV_NAME, &cdif_data);

	if (ret < 0)
	{
		printk("KERNEL : CDIF_IOCTL_REQUEST_IRQ Fail!! ret = %d\n", ret);
	}
	else
	{	
		printk("KERNEL : CDIF_IOCTL_REQUEST_IRQ Sucess!! ret = %d\n", ret);
	}	

	return ret;
}

static void set_cdbypass_enable(void)
{
	volatile unsigned int *DAMRR;

	DAMRR = (volatile unsigned int *)tcc_p2v(HwDAMR);
	*DAMRR = (HwDAMR_EN | HwDAMR_CC_EN | HwDAMR_CM_EN);
}

static void set_cdbypass_disable(void)
{
	volatile unsigned int *DAMRR;

	DAMRR = (volatile unsigned int *)tcc_p2v(HwDAMR);
	*DAMRR &= (HwDAMR_CC_DIS & HwDAMR_CM_DIS);
}

static void cd_drv_initialize(void)
{
	int ret;
	
	set_cdif_enable(OFF);
	CD_SetErrorStatus(CD_NO_PROBLEM);
	CD_SetBufferStatus(CD_BUF_STOP);
	(void)CDDISetBuffer(DAIGetCDDIBuffer());

	ret = set_cdif_interrupt();
	if (!ret)
	{	
		set_cdif_adma();
	}	
	else
	{
		printk("%s, Err : interrupt_irq\n", __FUNCTION__);
	}
}

#if SEEK_COMPLETION
void CD_SeekResultMonitor(char *res)
{
	if(res[0] == 'S')
	{
		strncpy(seek_response, res, sizeof(res));
//      printk("[dw_debug] wake up!! \n");
		complete(&tcc_completion);
	}
}
#endif

int CD_Send_msf(unsigned long msf)
{
	char response[2];
	int ret;
	unsigned long timeout;
	
	ret = nl_send_msf(msf);

#if SEEK_COMPLETION
	timeout = (10 * HZ);                    // timeout is 10second
	ret = wait_for_completion_interruptible_timeout(&tcc_completion, timeout);
	if(ret == 0)
	{
		printk("seek wait time out \n");
		return SEEK_TIME_OUT;
	}

	if(seek_response[1] == 1)               // seek success
	{
//      printk("[dw_debug] CD_Send_msf : seek reponse is success \n");
		ret = 1;
	}
/*	
	else if(seek_response[1] == 2)               // seek wait time out
	{
		printk("seek error\n");
		ret = DECK_TIME_OUT;
	}
*/	
	else                                    //seek fail
	{
		ret = DECK_ERROR;
	}
	memset(seek_response, 0, sizeof(seek_response));
	return ret;

#else
	timeout = get_jiffies_64() + (10 * HZ);     // timeout is 10second
	do
	{
		ret = nl_read_msf_response(response);

//      if(time_after_eq(jiffies, timeout))
		if(time_after_eq(jiffies, timeout) == true)
		{
			printk("seek wait time out \n");
			return SEEK_TIME_OUT;
		}
	}while( ret < 0 );

	if(response[1] == 1)                    // seek success
	{
//      printk("  response : %c %d\n", response[0], response[1]);
		return 1;
	}
	else if(response[1] == 2)               // seek wait time out
	{
//      printk("  response : %c %d\n", response[0], response[1]);
		return DECK_TIME_OUT;
	}
	else                                    //seek fail
	{
//      printk("  response : %c %d\n", response[0], response[1]);
		return DECK_ERROR;
	}
#endif
		
}

int pickupseek=0;
int research_cnt=0;
#ifdef CD_DUMP
int seek_n=0;
#endif

void CD_Set_StartBuffing(int wantSectorGap, unsigned long requestSampleSize)
{
	unsigned long haveSampleSize;
	SingleBufferState *pCDDIBuf = DAIGetCDDIBuffer();
	CD_ERROR_STATUS lErrStatus; /* QAC : 12.4 */
	
	haveSampleSize = (unsigned long)SingleBufferDataAvailable(pCDDIBuf);
	if ((wantSectorGap > 20) || (wantSectorGap < 0))	//20 sector
	{
		CD_SetBufferStatus(CD_BUF_STOP);
		set_cdif_enable(OFF);
	}
	else
	{
		lErrStatus = CD_GetErrorStatus();	/* QAC : 12.4 */

		if ((lErrStatus == CD_BUF_FULL) && (haveSampleSize < requestSampleSize))
		{
			CD_SetBufferStatus(CD_BUF_STOP);
			CD_SetErrorStatus(CD_NO_PROBLEM);
		}
	}
}

int CD_Do_StartBuffering(unsigned long requestSampleSize)
{
	long iWaitTimer;
	int lreturnVlaue =1;
	unsigned long haveSampleSize;
	SingleBufferState *pCDDIBuf = DAIGetCDDIBuffer();

	pickupseek = 0;
	haveSampleSize = (unsigned long)SingleBufferDataAvailable(pCDDIBuf);

	iWaitTimer = 500;				
	while (haveSampleSize < requestSampleSize) // wait until datalength bigger than requestSize.	// Empty
	{
		if(CD_GetErrorStatus() == CD_SHOCK)
		{
			return SHOCK_DETECT;
		}

		if(CD_IsDevForceStop() != 0)
		{
			return CD_FORCE_STOP;
		}
		
		if(--iWaitTimer<0)	
		{
			if (CD_GetErrorStatus() == CD_BUF_FULL)
			{
				return haveSampleSize;
			}
			pCDDIBuf->lWritePtr = 0;
			pCDDIBuf->lReadPtr = 0;
			CD_SetErrorStatus(CD_MISS_SUBQ);
			CD_SetBufferStatus(CD_BUF_STOP);
			set_cdif_enable(OFF);
			printk ("%s, err : TIME OVER, requset sample : %lu, have sample : %lu \n", __FUNCTION__, requestSampleSize, haveSampleSize);
			return BUFFERING_TIME_OUT;
		}
		msleep(5); // han
		haveSampleSize = (unsigned long)SingleBufferDataAvailable(pCDDIBuf);
	}

	return lreturnVlaue;
}

int CD_Do_StopBuffering(unsigned long startAddr, unsigned long requestSampleSize)
{
	long iWaitTimer;
	int lreturnValue=1;
	unsigned long haveSampleSize;
	SingleBufferState *pCDDIBuf = DAIGetCDDIBuffer();
	int ret = -1;
	
	pCDDIBuf->lWritePtr = 0;
	pCDDIBuf->lReadPtr = 0;

	pickupseek = 1;
	research_cnt = 0;
#ifdef CD_DUMP	
	seek_n++;
#endif

	ret = CD_Send_msf(startAddr);
	//printk("pick up time check\n");

	if (ret == -1)
	{
		return ret;
	}

	CD_SetBufferStatus(CD_BUF_START);   // han : using buffering
	set_cdif_enable(ON);
	
#ifdef CD_DUMP
	if (seek_n == 5)
	{
		seek_n = 0;
		while(1);
	}
#endif
	haveSampleSize = (unsigned long)SingleBufferDataAvailable(pCDDIBuf);

	//iWaitTimer = requestSampleSize * 2;
	iWaitTimer = 500;
	while (haveSampleSize < requestSampleSize)		// wait until datalength bigger than requestSize.	// Empty
	{
		if(CD_GetErrorStatus() == CD_SHOCK)
		{
			return SHOCK_DETECT;
		}
		if (CD_IsDevForceStop() != 0)
		{
			return CD_FORCE_STOP;
		}

		if (--iWaitTimer<0)
		{
			if (CD_GetErrorStatus() == CD_BUF_FULL)
			{
				return haveSampleSize;
			}
			CD_SetErrorStatus(CD_MISS_SUBQ);
			CD_SetBufferStatus(CD_BUF_STOP);
			set_cdif_enable(OFF);
			printk ("%s, err : TIME OVER, requset sample : %lu, have sample : %lu \n", __FUNCTION__, requestSampleSize, haveSampleSize);
			return BUFFERING_TIME_OUT;
		}

		msleep(5);
		haveSampleSize = (unsigned long)SingleBufferDataAvailable(pCDDIBuf);
	}

	//printk ("%s, length = %lx, readptr = %lx, writeptr = %lx \n", __FUNCTION__, pCDDIBuf->lLength, pCDDIBuf->lReadPtr, pCDDIBuf->lWritePtr);
	CD_SetErrorStatus(CD_NO_PROBLEM);

	return lreturnValue;
}


void CD_CopyReadData(unsigned short *pBuff, unsigned int copySize, unsigned long requestSampleSize)
{
	short *pSingle;
	unsigned int i, remainSize;
	SingleBufferState *pCDDIBuf = DAIGetCDDIBuffer();
	short *pChunkBuff = (short*)pBuff;
	
	pSingle  = (pCDDIBuf->pSingleBuf + (pCDDIBuf->lReadPtr << 1));
	/* Copy from CDDI buffer to Chunk buffer */

	/* in case of straight buffering */
	if (pCDDIBuf->lWritePtr > pCDDIBuf->lReadPtr)
	{
		for (i=0; i<copySize; i++)
		{
			*(pChunkBuff+i) = *pSingle;			// save data
			pSingle++;
		}
	}	/* in case of reverse buffering */
	else if (pCDDIBuf->lWritePtr < pCDDIBuf->lReadPtr)
	{
		remainSize = (unsigned int)((pCDDIBuf->lLength - pCDDIBuf->lReadPtr) << 1);

		if (copySize > remainSize)
		{
			for (i=0; i<remainSize; i++)
			{
				*(pChunkBuff+i) = *pSingle;		// save data
				pSingle++;
			}

			/* Initialize L/R start address */
			pSingle  = pCDDIBuf->pSingleBuf;

			for (i=remainSize; i<copySize; i++)
			{
				*(pChunkBuff+i) = *pSingle;		// save data
				pSingle++;
			}
		}
		else
		{
			for (i=0; i<copySize; i++)
			{
				*(pChunkBuff+i) = *pSingle;		// save data
				pSingle++;
			}
		}
	}
	else
	{
		/* no statement */
	}

	if (requestSampleSize != 0)
	{
		CD_SetCHOffet(copySize);	//SingleBuffer's offset
	}
}


/****************************************************************************
 FUNCTION NAME:
	CD_ReadData

 DESCRIPTION:
	Read indicated sector from CD.

 PARAMETERS:
	startAddr : logical block address
 	sectorSize : request sector number
 	pBuff : Data Buffer

 RETURNS:
 	int : return read size

 REMARK:
 	2007/10/08	by sjw
****************************************************************************/
int CD_ReadData(unsigned long startAddr, unsigned long sectorSize, unsigned short *pBuff)
{
	unsigned int copySize;
	unsigned long wantSampleSize, requestSampleSize;
	unsigned long maxSector, marginSector=0, marginSample;
	int lres=0, lreturnValue=0, wantSectorGap;

	//printk ("%s \n", __FUNCTION__);

/********************************Calculate read data********************************/

	wantSectorGap = (int)(CD_GetRequestSector() - CD_GetLastSector());
	maxSector = (CDDA_BUFFER_SIZE / CDDA_SECTOR_SIZE);

	if (maxSector >= sectorSize)
	{
		if (maxSector > (sectorSize + CD_SEEK_TOLERANCE))
		{
			marginSector = CD_SEEK_TOLERANCE;
		}
		else
		{
			marginSector = (sectorSize + CD_SEEK_TOLERANCE) - maxSector;
		}
	}
	else 	//if(sectorSize > maxSector)
	{
		sectorSize = maxSector-CD_SEEK_TOLERANCE; 
		marginSector = 0;
	}

	wantSampleSize = (sectorSize+marginSector)*CDDA_SAMPLE_SIZE;
	requestSampleSize = wantSampleSize;

/********************************Check buffering status********************************/

	if (CD_GetBufferStatus()==CD_BUF_START)
	{
		CD_Set_StartBuffing(wantSectorGap, requestSampleSize);
	}
	else if (CD_GetBufferStatus()==CD_BUF_STOP)
	{
		set_cdif_enable(OFF);
	}
	else		// CD_BUF_REBUF
	{
		marginSample = (maxSector -sectorSize) * CDDA_SAMPLE_SIZE;
		if(marginSample > 0x3000)
		{
			requestSampleSize = wantSampleSize + 0x3000;
		}
		else
		{
			requestSampleSize = wantSampleSize + marginSample;
		}
		set_cdif_enable(OFF);
	}

/********************************Data buffering********************************/

TRY_READ:

	switch (CD_GetBufferStatus())
	{
		case CD_BUF_START:
			lres = CD_Do_StartBuffering(requestSampleSize);
			break;
		case CD_BUF_STOP:
			lres = CD_Do_StopBuffering(startAddr, requestSampleSize);
			break;
		case CD_BUF_REBUF:
			//lres = CD_Do_ReBuffering(cdType,startAddr, requestSampleSize);
			break;
		default:
			break;
	}

	if (lres >= 0)
	{	
		copySize = wantSampleSize << 1;

		CD_CopyReadData(pBuff, copySize, requestSampleSize);

		lreturnValue = (int)(wantSampleSize<<2);
	}
	else if (lres == BUFFERING_TIME_OUT)
	{
		goto TRY_READ;
	}
	else
	{
		lreturnValue = lres;
	}
	
	return lreturnValue;

}


static void CD_Research_Frame(unsigned long startAddr)
{
	int secdiff = 0;
	unsigned long datalength, realSector;
	SingleBufferState *pCDDIBuf = DAIGetCDDIBuffer();
	long ltmp;

	CD_SetRequestSector(startAddr);

	if (CD_GetBufferStatus()==CD_BUF_START)
	{
		secdiff = (int)(startAddr - (unsigned long)CD_GetLastSector()-1);

		if (secdiff>=0)
		{
			datalength = (unsigned long)SingleBufferDataAvailable(pCDDIBuf);	//current data size in IO buffer
			realSector = datalength / CDDA_SAMPLE_SIZE;

			//if((realSector>secdiff) && ((realSector-secdiff) > sectorSize))
			if (realSector>(unsigned long)secdiff)
			{
				ltmp = SingleBufferUpdateReadPointer(pCDDIBuf, (long)((secdiff * CDDA_SECTOR_SIZE)>>2));	//Single
			}
			else
			{
				CD_SetBufferStatus(CD_BUF_STOP);
			}
		}
	}
}


/****************************************************************************
 	Get requested sector data. (Need C3 decoder)

 	Parameter
 		startAddr : logical block address
 		sectorSize : request sector number
 		pBuff : Data Buffer

	Return
		res : read size
													2006/06/23	by sjw
****************************************************************************/

int CD_DescrambleSector(unsigned long startAddr, unsigned long sectorSize, void *pBuff)
{
	unsigned int usedSectNum=0, copyPointer, lcnt, lmode, msf, frame, ldec, request_frame;
	unsigned long datalength, realSector, goMSF, reqSize;
	unsigned char *psCDBuff = (unsigned char*)pBuff;
	unsigned char *pdCDBuff = (unsigned char*)pBuff;
	int res, readSize, frame_retry=0, c3_retry=0, kmp_retry=0;
	int secdiff = 0, kmpcnt;
	unsigned char Pattern[CD_CMP_BYTE];
	long ltmp;
	SingleBufferState *pCDDIBuf = DAIGetCDDIBuffer();

	//printk ("%s, addr = (%d), size = (%d)\n", __FUNCTION__, startAddr, sectorSize);

	if (CD_IsDevForceStop() != 0)
	{
		return CD_FORCE_STOP;
	}

	frame_retry = CD_RETRY_CNT;		// Sector Search retry count
	c3_retry = CD_C3_CNT;
	kmp_retry = CD_KMP_RETRY_CNT;

	CD_Research_Frame(startAddr);
	psCDBuff = pBuff;

/********************************Data Read********************************/

TRY_READ:

	kmpcnt = CD_KMP_RETRY_CNT;	// CD_KmpPatternSearch retry count
	psCDBuff = pdCDBuff;
	goMSF = (unsigned long)((startAddr+usedSectNum)+CD_GetDiffValue());
	reqSize = sectorSize-(unsigned long)usedSectNum;

	/*Read raw data from Deck*/
	readSize = CD_ReadData(goMSF, reqSize, (unsigned short *)psCDBuff);

	if (readSize <= 0)
	{
		return readSize;
	}

	Pattern[0] = 0x00;
	Pattern[1] = 0xFF;
	Pattern[2] = 0xFF;
	Pattern[3] = 0xFF;
	Pattern[4] = 0xFF;
	Pattern[5] = 0xFF;
	Pattern[6] = 0xFF;
	Pattern[7] = 0xFF;
	Pattern[8] = 0xFF;
	Pattern[9] = 0xFF;
	Pattern[10] = 0xFF;
	Pattern[11] = 0x00;

/********************************Header Search********************************/
	
SEARCH_LBN:

	res = CD_KmpPatternSearch(psCDBuff, CDDA_SECTOR_SIZE, Pattern, CD_HEAD_SIZE);

	if ((res < 0)&&(kmpcnt > 0))
	{
		//printk ("%s, KMP RETRY, frame = (%d), Cnt = %d\n", __FUNCTION__, goMSF, kmpcnt);
		psCDBuff += SECTOR_SIZE;
		ltmp = SingleBufferUpdateReadPointer(pCDDIBuf, SECTOR_SIZE>>2);
		kmpcnt --;
		goto SEARCH_LBN;
	}
	else if (res <0)
	{
		CD_SetBufferStatus(CD_BUF_STOP);
		kmp_retry--; 

		if(kmp_retry > 0)
		{
			goto TRY_READ;
		}
		else
		{
			return KMP_ERROR;
		}	
	}
	else
	{
		//printk ("%s, KMP SUCCESS \n", __FUNCTION__);
		/* no statement */
	}

	psCDBuff += res;
	ltmp = SingleBufferUpdateReadPointer(pCDDIBuf, (long)((unsigned int)res>>2));
	
	if(CD_IsDevForceStop() != 0)	/* QAC : 13.2 */
	{
		return CD_FORCE_STOP;
	}				

/****************************************** Descramble ******************************************/

	Descramble_Sector(psCDBuff);
	lmode = *(psCDBuff+MODE_LOCA); 		// Calculate mode value
	msf = 0;
	for (lcnt=0; lcnt<3; lcnt++)
	{
		ldec = *(psCDBuff+MINUTE_LOCA+lcnt);
		msf <<= 8;
		msf |= ldec;
	}

	frame = CD_BCDMSF2FRAME(msf);			// Convert MSF to FRAME value

	research_cnt ++;
	
/************************************ Search frame & C3 decoder ***********************************/			

	request_frame = (unsigned int)startAddr+usedSectNum;
	
	if (frame == request_frame)				// Compare request location with read location
	{
		//printk ("Compare OK, frame = (%d)\n", frame);

		res = C3_decoder(psCDBuff, CD_C3_CNT);
		
		if(res < CD_TRUE)		// C3 Ddecoder Fail
		{
			CD_SetBufferStatus(CD_BUF_STOP);
			c3_retry--;
			if(c3_retry > 0)
			{	
				printk ("occur : C3 Error, frame = (%d), cnt = (%d)\n", frame, research_cnt);
				goto TRY_READ;
			}
			else
			{
				printk ("ignore : C3 Error, frame = (%d), cnt = (%d)\n", frame, research_cnt);
			}
		}
		
		for (copyPointer=0; copyPointer<SECTOR_SIZE; copyPointer++)
		{
			if (lmode == MODE1)
			{
				*(pdCDBuff + copyPointer) = *(psCDBuff + (copyPointer + MODE1_DATA_LOCATION));		// Copy data after c3 decoder by mode
			}
			else
			{
				*(pdCDBuff + copyPointer) = *(psCDBuff + (copyPointer + MODE2_DATA_LOCATION));		// Copy data after c3 decoder by mode
			}
		}

		psCDBuff += CDDA_SECTOR_SIZE;
		pdCDBuff += SECTOR_SIZE;
		CD_SetLastSector(startAddr+usedSectNum);
		ltmp = SingleBufferUpdateReadPointer(pCDDIBuf, CDDA_SECTOR_SIZE>>2);	//Single
		frame_retry = CD_RETRY_CNT;
	}
	else if (frame > request_frame)		// If read location is bigger than request location, retry read requested sector from CD.
	{
		if (frame_retry <= 0)
		{
			return FRAME_OVER;
		}

		secdiff = (int)((startAddr+usedSectNum) -frame);
		
		if (research_cnt > PREVIOUS_DATA_SKIP_COUNT)	// filtering previous data from CD Deck
		{
			//printk ("Frame BIG : Retry, read = (%d), request = (%d), cnt = (%d)\n", frame, request_frame, research_cnt);
			if (pickupseek)
			{
				frame_retry--;
				CD_SetDiffValue(secdiff);
			}
			CD_SetBufferStatus(CD_BUF_STOP);
			goto TRY_READ;
		}
		else
		{
			//printk ("Deck BIG : Search, read = (%d), request = (%d), cnt = (%d)\n", frame, request_frame, research_cnt);
			psCDBuff += SECTOR_SIZE;
			ltmp = SingleBufferUpdateReadPointer(pCDDIBuf, SECTOR_SIZE>>2);
			goto SEARCH_LBN;
		}
	}
	else			// If read location is smaller than request location, retry read requested sector from buffer.
	{
		if (frame_retry <= 0)
		{
			return FRAME_UNDER;
		}

		secdiff = (int)((startAddr+usedSectNum) -frame);
		datalength = (unsigned long)SingleBufferDataAvailable(pCDDIBuf);	//current data size in IO buffer
		realSector = datalength / CDDA_SAMPLE_SIZE;

		if (frame < goMSF)
		{
			//printk ("Deck SMALL : Search1, read = (%d), request = (%d), cnt = (%d)\n", frame, request_frame, research_cnt);
			psCDBuff += SECTOR_SIZE;
			ltmp = SingleBufferUpdateReadPointer(pCDDIBuf, SECTOR_SIZE>>2);
			goto SEARCH_LBN;
		}
		else
		{
			if (realSector >= (unsigned long)secdiff)
			{
				//printk ("Frame SMALL : Search, read = (%d), request = (%d), cnt = (%d)\n", frame, request_frame, research_cnt);
				psCDBuff += (secdiff * CDDA_SECTOR_SIZE) - CD_MOVE_SIZE;
				ltmp = SingleBufferUpdateReadPointer(pCDDIBuf, (long)((unsigned int)((secdiff * CDDA_SECTOR_SIZE) - CD_MOVE_SIZE)>>2));
				goto SEARCH_LBN;
			}
			else
			{
				if (research_cnt > PREVIOUS_DATA_SKIP_COUNT)	// filtering previous data from CD Deck
				{
					//printk ("Frame SMALL : Retry, read = (%d), request = (%d), cnt = (%d)\n", frame, request_frame, research_cnt);
					if (pickupseek)
					{
						frame_retry--;
						CD_SetDiffValue(secdiff);
					}
					CD_SetBufferStatus(CD_BUF_STOP);
					goto TRY_READ;
				}
				else
				{
						//printk ("Deck SMALL : Search2, read = (%d), request = (%d), cnt = (%d)\n", frame, request_frame, research_cnt);
					psCDBuff += SECTOR_SIZE;
					ltmp = SingleBufferUpdateReadPointer(pCDDIBuf, SECTOR_SIZE>>2);
					goto SEARCH_LBN;
				}
			}
		}
	}

	usedSectNum++;

	if (usedSectNum<(unsigned int)sectorSize)
	{
		goto SEARCH_LBN;
	}

	return SECTOR_SIZE;
}

static void Notify_Error(int error)
{
	switch(error)
	{
		case DECK_ERROR:
			printk("*** %s *** : (DECK_PHYSICAL_ERROR)\n", __FUNCTION__);
			break;
		case DECK_TIME_OUT:
			printk("*** %s *** : (DECK_TIME_OUT)\n", __FUNCTION__);
			break;
		case SEEK_TIME_OUT:
			printk("*** %s *** : (SEEK_WAIT_TIME_OUT)\n", __FUNCTION__);
			break;
		case KMP_ERROR:
			printk("*** %s *** : (KMP_ERROR)\n", __FUNCTION__);
			break;
		case FRAME_OVER:
			printk("*** %s *** : (FRAME_OVER)\n", __FUNCTION__);
			break;
		case FRAME_UNDER:
			printk("*** %s *** : (FRAME_UNDER)\n", __FUNCTION__);
			break;
		case SHOCK_DETECT:
			printk("*** %s *** : (SHOCK_DETECT)\n", __FUNCTION__);
			break;
		case BUFFERING_TIME_OUT:
			printk("*** %s *** : (BUFFERING_TIME_OUT)\n", __FUNCTION__);
			break;
		case CD_FORCE_STOP:
			printk("*** %s *** : (CD_FORCE_STOP)\n", __FUNCTION__);
			break;
		default:
			printk("*** %s *** : (UNKNOWN ERROR)\n", __FUNCTION__);
			break;
	}
}

/****************************************************************************
	memory copy from CD to Kernel
	
	Parameter
		startAddr : logical block address
		sectorSize : request sector number
		pBuff : Data Buffer 	
		
	Return 
		returnByteSize : read size (byte)
													2014/02/11	by sjw
****************************************************************************/
int CD_ReadSector(unsigned long startAddr, unsigned long sectorSize, void *pBuff)
{
	int readByteSize=0;
	unsigned char *pCDBuf;
printk("[tcc-debug] %s start[msf:%d]\n", __func__, startAddr);
	pCDBuf = (unsigned char *)pGetCDMemory();
	readByteSize = CD_DescrambleSector(startAddr, sectorSize, pCDBuf);

	if (readByteSize < 0)	// Fail
	{
		Notify_Error(readByteSize);
#ifdef TRAP_DATA
		memcpy(pBuff, pCDBuf, 2048);
		readByteSize = 2048;
#endif		
	}
	else		// Success
	{
		memcpy(pBuff, pCDBuf, readByteSize);
	}	

	if (pickupseek)
	{
		//printk("data upload time check\n");
	}

printk("[tcc-debug] %s end[msf:%d]\n", __func__, startAddr);
	return readByteSize;
}

int CD_read_TOC(unsigned long *buffer, unsigned int size)
{
	return nl_read_toc_buffer(buffer, size);
}

int cdif_init()
{
	volatile unsigned int *pCICR, *pGnFNC1, *pGnFNC2;

	printk ("MODULE : %s \n", __FUNCTION__);
	
	pCICR = (volatile unsigned int *)tcc_p2v(HwCICR);
	
	*pCICR = 0x00;

	*pCICR = (
                //HwCDIF_EN |
                HwCDIF_BCK_64 //|         //Bit Clock 64fs
                //HwCDIF_MD             //LSB justified format
	         );
	
	printk ("KERNEL : CICR ADDR = [0x%p]\n", pCICR);
	printk ("KERNEL : CICR VALUE = [0x%08X]\n", *pCICR);		 

/** 
 * @author back_yak@cleinsoft
 * @date 2014/01/29 , 2014/02/27
 * Daudio CDP GPIO
 **/

	////////////////////GPIO PORT MULITLEXING ////////////////////
#if defined(CONFIG_DAUDIO_20131007)	
	// 2nd board setting
	pGnFNC1 = (volatile unsigned int *)tcc_p2v(HwGPIOPE_FNC3);	//GPIOE 29(BCLK)
	*pGnFNC1 |= (Hw20 | Hw21);	// FUNC3	--> reference PIN DESCRIPTION
	
	pGnFNC2 = (volatile unsigned int *)tcc_p2v(HwGPIOPE_FNC3);	//GPIOE 30, 31 (LRCK, CDDI)
	*pGnFNC2 |= (Hw25 | Hw24 | Hw29 | Hw28);	// FUNC3
#endif
#if defined(CONFIG_DAUDIO_20140220)  
	// 3rd board setting
	pGnFNC1 = (volatile unsigned int *)tcc_p2v(HwGPIOPC_FNC3);  //GPIOC 27, 28(BCLK, LRCK)
	*pGnFNC1 |= (Hw12 | Hw16);  // FUNC1    --> reference PIN DESCRIPTION

	pGnFNC2 = (volatile unsigned int *)tcc_p2v(HwGPIOPC_FNC1);  //GPIOC 9 (CDDI)
	*pGnFNC2 |= (Hw4);    // FUNC1
#endif

	cd_drv_initialize();
	return 0;
}

int cdif_release()
{
	/** 
	 * @author sjpark@cleinsoft
	 * @date 2014/03/06
	 * CDIF Register Address Set - CDIF Audio Channel changed from Audio0 to Audio1 on daudio 3rd board.
	 **/
	free_irq(INT_ADMA, &cdif_data);
	
	return 0;
}

int cdif_ioctl (unsigned int cmd, unsigned long arg)
{
	int ret = 1;
	
	//spin_lock(&cdif_lock);
	switch (cmd)
	{
		case CDIF_IOCTL_BYPASS_ENABLE:
			set_cdbypass_enable();
			break;
		case CDIF_IOCTL_BYPASS_DISABLE:
			set_cdbypass_disable();
			break;
		case CDIF_IOCTL_REQUEST_IRQ:
			ret = set_cdif_interrupt();
			break;
		case CDIF_IOCTL_ADMA_ENABLE:
			set_cdif_adma();
			break;
		case CDIF_IOCTL_VALUE_CHECK:
			//check_value_gpio();
			//CD_ReadSector(1, 1, test_buf);
			break;
		default:
			printk("KERNEL : CDIF_IOCTL_DEFAULT\n");
			ret = -ENOSYS;
			break;
	}
	//spin_unlock(&cdif_lock);

	return ret;
}

/* End Of File */
