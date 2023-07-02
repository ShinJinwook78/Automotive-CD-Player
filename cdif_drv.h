/****************************************************************************
 *   FileName    : CD_DSP_Drv.h
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
*	Date: 2006. 06. 12 by Kevin													*
****************************************************************************/

#ifndef __CDIF_DRV_H__
#define __CDIF_DRV_H__


/***********************************************************
	DEFINE
***********************************************************/


#define CDIF_DEV_NAME		"cdif"
#define CDIF_DEV_MAJOR		255

#define CDDIDMA_BUFFER_SIZE     8192

/** 
 * @author sjpark@cleinsoft
 * @date 2014/03/06
 * CDIF Register Address Set - CDIF Audio Channel changed from Audio0 to Audio1 on daudio 3rd board.
 **/
#if defined(CONFIG_DAUDIO_20140220)
	#define AUDIO1_CDIF_USE
#else
	#define AUDIO0_CDIF_USE
#endif

#if defined(AUDIO0_CDIF_USE)
///////////////////////// DAMR /////////////////////////
//#define	HwDAMR					*(volatile unsigned long *)0x76201040UL		// R/W, Digital Audio Mode Register
#define	HwDAMR					0x76201040UL
#define	HwDAMR_EN				Hw15					// Enable DAMR Register
#define	HwDAMR_CC_EN			Hw8					// Enable CDIF Clock master mode
#define	HwDAMR_CC_DIS			~Hw8				// Disable CDIF Clock master mode
#define	HwDAMR_CM_EN			Hw2					// CDIF Monitor Mode is Enable
#define	HwDAMR_CM_DIS			~Hw2				// CDIF Monitor Mode is Disable

///////////////////////// CICR /////////////////////////
//#define	HwCICR					*(volatile unsigned long *)0x762010A0UL	//R/W,  CD Interface Control Register
#define	HwCICR					0x762010A0UL
#define	HwCDIF_EN				Hw7
#define	HwCDIF_BCK_64			0			// 64fs
#define	HwCDIF_BCK_32			Hw2			// 32fs
#define 	HwCDIF_BCK_48		Hw3			// 48fs
#define	HwCDIF_MD				Hw1
#define	HwCDIF_BP				Hw0

#define	HwCDDI_0				0x76201080UL
#define	HwCDDI_1				0x76201084UL
#define	HwCDDI_2				0x76201088UL
#define	HwCDDI_3				0x7620108CUL
#define	HwCDDI_4				0x76201090UL
#define	HwCDDI_5				0x76201094UL
#define	HwCDDI_6				0x76201098UL
#define	HwCDDI_7				0x7620109CUL

///////////////////////// GPIO /////////////////////////
//#define	HwGPIOPD_FS0			*(volatile unsigned long *)0x74200034UL// R/W, GPIO_PD Function Select 0
#define	HwGPIOPG_FNC0			0x742001B0UL
#define	HwGPIOPG_FNC1			0x742001B4UL
#define	HwGPIOPG_FNC2			0x742001B8UL
#define	HwGPIOPG_FNC3			0x742001BCUL

///////////////////////// INTERRUPT  /////////////////////////
#define HwIEN1					0x74100004UL
#define HwSEL1					0x7410001CUL
#define HwMODE1					0x74100064UL
#define HwINTMSK1				0x74100103UL

#define HwIEN_ADMA0				Hw28
#define CDIF_BASE_ADDR_ADMA 	(BASE_ADDR_ADMA0)
#define INT_ADMA				(INT_ADMA0)

#elif defined(AUDIO1_CDIF_USE)

///////////////////////// DAMR /////////////////////////
//#define   HwDAMR                  *(volatile unsigned long *)0x76201040UL     // R/W, Digital Audio Mode Register
#define HwDAMR                  0x76101040UL
#define HwDAMR_EN               Hw15                    // Enable DAMR Register
#define HwDAMR_CC_EN            Hw8                 // Enable CDIF Clock master mode
#define HwDAMR_CC_DIS           ~Hw8                // Disable CDIF Clock master mode
#define HwDAMR_CM_EN            Hw2                 // CDIF Monitor Mode is Enable
#define HwDAMR_CM_DIS           ~Hw2                // CDIF Monitor Mode is Disable

///////////////////////// CICR /////////////////////////
//#define   HwCICR                  *(volatile unsigned long *)0x762010A0UL //R/W,  CD Interface Control Register
#define HwCICR                  0x761010A0UL
#define HwCDIF_EN               Hw7
#define HwCDIF_BCK_64           0           // 64fs
#define HwCDIF_BCK_32           Hw2         // 32fs
#define HwCDIF_BCK_48       	Hw3         // 48fs
#define HwCDIF_MD               Hw1
#define HwCDIF_BP               Hw0

#define HwCDDI_0                0x76101080UL
#define HwCDDI_1                0x76101084UL
#define HwCDDI_2                0x76101088UL
#define HwCDDI_3                0x7610108CUL
#define HwCDDI_4                0x76101090UL
#define HwCDDI_5                0x76101094UL
#define HwCDDI_6                0x76101098UL
#define HwCDDI_7                0x7610109CUL

///////////////////////// INTERRUPT  /////////////////////////
#define HwIEN1      			0x74100004UL
#define HwSEL1      			0x7410001CUL
#define HwMODE1     			0x74100064UL
#define HwINTMSK1   			0x74100103UL

#define HwIEN_ADMA1     		Hw26
#define CDIF_BASE_ADDR_ADMA 	(BASE_ADDR_ADMA1)
#define INT_ADMA				(INT_ADMA1)
#endif  //defined(AUDIO0_CDIF_USE)

/** 
 * @author back_yak@cleinsoft
 * @date 2014/01/29 , 2014/02/27
 * Daudio CDP GPIO
 **/
#if defined(CONFIG_DAUDIO_20131007)
#define	HwGPIOPE_FNC0		0x74200130UL
#define	HwGPIOPE_FNC1		0x74200134UL
#define	HwGPIOPE_FNC2		0x74200138UL
#define	HwGPIOPE_FNC3		0x7420013CUL
#endif

#if defined(CONFIG_DAUDIO_20140220)
#define HwGPIOPC_FNC0       0x742001B0UL
#define HwGPIOPC_FNC1       0x742001B4UL
#define HwGPIOPC_FNC2       0x742001B8UL
#define HwGPIOPC_FNC3       0x742001BCUL
#endif

/* Define Error Statue */
#define DECK_ERROR		-1
#define DECK_TIME_OUT	-2
#define SEEK_TIME_OUT	-3
#define KMP_ERROR		-4
#define FRAME_OVER		-5
#define FRAME_UNDER	-6
#define SHOCK_DETECT	-7
#define BUFFERING_TIME_OUT		-8
#define CD_FORCE_STOP	-9


/*
 * ----------------------------------------------------------------------------
 * I/O mapping
 * ----------------------------------------------------------------------------
 */
#define PCIO_BASE	0

#define IO_OFFSET	0x81000000	/* Virtual IO = 0xf1000000 */
#define IO_ADDRESS(pa)	((pa) + IO_OFFSET)
#define io_p2v(pa)	((pa) + IO_OFFSET)
#define io_v2p(va)	((va) - IO_OFFSET)

/* Physical value to Virtual Address */
#define tcc_p2v(pa)         io_p2v(pa)


enum
{
	CDIF_IOCTL_ADMA_ENABLE,
	CDIF_IOCTL_REQUEST_IRQ,
	CDIF_IOCTL_BYPASS_ENABLE,
	CDIF_IOCTL_BYPASS_DISABLE,
	CDIF_IOCTL_VALUE_CHECK
};


/***********************************************************
	VARIABLE GLOBAL DEFINE - Interface Functions
***********************************************************/
extern int CD_DevStatus;
extern unsigned char CD_DevForceStop;


/***********************************************************
	FUNCTION GLOBAL DEFINE - Interface Functions
***********************************************************/
extern void set_cdif_enable(unsigned long ulFlag);

extern unsigned short *pGetCDMemory(void);
extern int CD_ReadData(unsigned long startAddr, unsigned long sectorSize, unsigned short *pBuff);
extern int CD_DescrambleSector(unsigned long startAddr, unsigned long sectorSize, void *pBuff);
extern int CD_ReadSector(unsigned long startAddr, unsigned long sectorSize, void *pBuff);
extern void CD_SeekResultMonitor(char *res);
extern int CD_Send_msf(unsigned long msf);
extern int CD_read_TOC(unsigned long *buffer, unsigned int size);
extern void CD_Set_StartBuffing(int wantSectorGap, unsigned long requestSampleSize);
extern int CD_Do_StartBuffering(unsigned long requestSampleSize);
extern int CD_Do_StopBuffering(unsigned long startAddr, unsigned long requestSampleSize);
extern void CD_CopyReadData(unsigned short *pBuff, unsigned int copySize, unsigned long requestSampleSize);

extern int cdif_init(void);
extern int cdif_release(void);
extern int cdif_ioctl(unsigned int cmd, unsigned long arg);


#endif /* #ifndef __CD_DSP_DRV_H__ */

/* End Of File */
