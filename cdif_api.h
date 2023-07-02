/****************************************************************************
 *   FileName    : CD_API.h
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
#ifndef	__CD_API_H__
#define	__CD_API_H__

#include "cdif_drv.h"

#include "SingleBuffer.h"
#include "cd_filesystem.h"

#ifndef ON
#define ON	0x01
#endif
#ifndef OFF
#define OFF	0x00
#endif

#ifndef CD_TRUE
#define CD_TRUE		1
#endif
#ifndef CD_FALSE
#define CD_FALSE	-1
#endif

#define CD_CMP_NAME             ";"
#define ONE_SESSION             1

#define FIRST_SESSION           1
#define SECOND_SESSION          2

#define CD_EXT_LONG             3
#define CD_EXT_SHORT            2

#define CD_NAME_LONG            4
#define CD_NAME_SHORT           2

#define BRWS_EXT_LONG           7
#define BRWS_EXT_SHORT          4

#define	CDDI_BUFFER_SPACE       4		//Sample
#define	CDDI_BUFFER_FILLSIZE    5880	//10Sector sample size
#define CDDI_SHOCK_SIZE         10		//10sector
#define	MECA_READ_MARGIN        30		//10sector
#define	CDDI_BUFFER_SIZE        1024*1024*2	//sample

/* Returns for status of CD device [CD_GetDevStatus()] */
#define CD_STATUS_NORMAL 0
#define CD_STATUS_ERR -1
#define CD_STATUS_FORCESTOP -2

enum
{
	CDDA_CMD_PLAY = 1,
	CDDA_CMD_FF,
	CDDA_CMD_REW
};

enum
{
	CD_PLAY_NORMAL = 1,
	CD_PLAY_RESUME,
	CD_PLAY_PAUSE,
	CD_FF_NORMAL,
	CD_FF_TURBO,
	CD_FF_NEXT_TRACK,
	CD_REW_NORMAL,
	CD_REW_TURBO,
	CD_REW_PREV_TRACK,
	CD_FFREW_END
};

typedef enum
{
	CD_BUF_START = 1,
	CD_BUF_STOP,
	CD_BUF_REBUF

} CD_BUF_STATUS;

typedef enum
{
	CD_NO_PROBLEM,
	CD_BUF_FULL,
	CD_MISS_SUBQ,
	CD_SHOCK

} CD_ERROR_STATUS;


typedef struct _CD_STATUS
{
	CD_BUF_STATUS	gCDBufStatus;
	CD_ERROR_STATUS	gCDErrorStatus;
	unsigned int 	gLastSector;
	unsigned int	gRequestSector;
	int	BefValue;	//Before MSF value
	int	DiffValue; 	//Different value
	unsigned int	CHOffset;	// CD_ChunkBuffer Offset
	unsigned int	LastSubQ;	// Last read SubQ value
	unsigned int	gCDSaveCurrNum;	// Current file number when CD Play

} CD_STATUS;


typedef struct
{
	SingleBufferState *pOutput;
	unsigned long ulLength;

	short psSingle[CDDI_BUFFER_SIZE*2];

} tCDDIBUF;


typedef struct _CD_COPY
{
	unsigned int gCopyFileIndex;	// Copy file Index

} tCD_COPY;

extern tCDDIBUF tCDDIBuffer; /* QAC : 8.8 */
extern CD_STATUS CD_Status;


extern CD_STATUS *pGetCDStatus(void); /* QAC : 8.8 */
extern void  CD_SetBufferStatus(CD_BUF_STATUS Format);
extern CD_BUF_STATUS  CD_GetBufferStatus(void);
extern void  CD_SetErrorStatus(CD_ERROR_STATUS Format);
extern CD_ERROR_STATUS CD_GetErrorStatus(void);
extern void  CD_SetLastSector(unsigned int lastSector);
extern unsigned int  CD_GetLastSector(void);
extern void  CD_SetRequestSector(unsigned int requestSector);
extern unsigned int  CD_GetRequestSector(void);
extern void  CD_SetCHOffet(unsigned int offset);
extern unsigned int  CD_GetCHOffet(void);
extern void  CD_SetLastSubQ(unsigned int subQ);
extern unsigned int  CD_GetLastSubQ(void);
extern void  CD_SetDiffValue(int value);
extern int CD_GetDiffValue(void);
extern int CD_KmpPatternSearch(const unsigned char *pSrcAddr, int src_len, const unsigned char *pPattern, unsigned int pattern_len);
extern SingleBufferState * DAIGetCDDIBuffer(void);
extern long CDDISetBuffer(SingleBufferState *psBuffer);
extern void CD_ShockProcess(SingleBufferState *pBuf);
extern unsigned char CD_HEX2BCD(unsigned char lHex);
extern unsigned char CD_BCD2HEX(unsigned char lBcd);
extern unsigned int CD_FRAME2MSF(unsigned int lFrame);
extern unsigned int CD_FRAME2HEX(unsigned int lFrame);
extern void CD_SetDevForceStop(unsigned int SetBit);
extern unsigned char CD_IsDevForceStop(void);
extern int CD_GetDevStatus(void);

#endif
