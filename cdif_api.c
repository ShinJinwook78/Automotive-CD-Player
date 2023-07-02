/****************************************************************************
 *   FileName    : CDIF_API.c
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

#include <linux/slab.h>
//#include "SingleBuffer.h"
//#include "cd_filesystem.h"
#include "cdif_api.h"


unsigned char CD_CmpBuf1[CD_CMP_BYTE];
unsigned char CD_CmpBuf2[CD_CMP_BYTE];


/* For force stop func. */
int CD_DevStatus;
unsigned char CD_DevForceStop;

SingleBufferState sCDDIBuffer;
tCDDIBUF tCDDIBuffer;
CD_STATUS CD_Status;


CD_STATUS *pGetCDStatus(void)
{
	return &CD_Status;
}

/****************************************************************************
	CD_byte2sector

	Description
	 	Conversion from byte to sector

 	Parameter
 		byte : byte size (reading data size)

	Return
		res : sector count
														2006/06/23	by sjw
****************************************************************************/
unsigned int CD_byte2sector(unsigned long byte)
{
	unsigned int res;
	res = byte / SECTOR_SIZE;

	if ((byte%SECTOR_SIZE) != 0)
	{
		res += 1;
	}

	return res;
}


/****************************************************************************
	CD_HEX2BCD

	Description
 		Change HEX to BCD value

 	Parameter
 		lHex : Hex value

	Return
		lBcd : BCD value
													2007/11/26	by sjw
****************************************************************************/
unsigned char CD_HEX2BCD(unsigned char lHex)
{
	register unsigned char lBcd = 0;

	while (lHex >= 10)
	{
		lBcd += 0x10;
		lHex -= 10;
	}

	lBcd += lHex;
	return lBcd;
}


/****************************************************************************
	CD_BCD2HEX

	Description
 		Change BCD to HEX value

 	Parameter
 		lBcd : BCD value

	Return
		lHex : HEX value
													2007/11/26	by sjw
****************************************************************************/
unsigned char CD_BCD2HEX(unsigned char lBcd)
{
	register unsigned char lHex = 0;

	while (lBcd >= 0x10)
	{
		lHex += 10;
		lBcd -= 0x10;
	}

	lHex += lBcd;
	return lHex;
}


/****************************************************************************
	CD_BCDMSF2FRAME

	Description
 		Change MSF to Frame value

 	Parameter
 		lMSF : Minute Second Frame

	Return
		FAIL : Parsing fail
		SUCCESS : Parsing Success
													2007/04/20	by sjw
****************************************************************************/
unsigned int CD_BCDMSF2FRAME(unsigned int lMSF)
{
	unsigned int lFrame;
	TIME_CD MSF;
	MSF.ulValue = lMSF;
	lFrame = (CD_BCD2HEX(MSF.t.ucMin)*60*75)+(CD_BCD2HEX(MSF.t.ucSec)*75)+CD_BCD2HEX(MSF.t.ucFrame);
	return lFrame;
}


/****************************************************************************
	CD_HEXMSF2FRAME

	Description
 		Change Hex value MSF to Frame value

 	Parameter
 		lMSF : Minute Second Frame

	Return
		FAIL : Parsing fail
		SUCCESS : Parsing Success
													2008/05/26	by sjw
****************************************************************************/
unsigned int CD_HEXMSF2FRAME(unsigned int lMSF)
{
	unsigned int lFrame;
	TIME_CD MSF;
	MSF.ulValue = lMSF;
	lFrame = (MSF.t.ucMin*60*75)+(MSF.t.ucSec*75)+MSF.t.ucFrame;
	return lFrame;
}


/****************************************************************************
	CD_FRAME2MSF

	Description
 		Change Frame to MSF value

 	Parameter
 		lMSF : Minute Second Frame

	Return
		FAIL : Parsing fail
		SUCCESS : Parsing Success
													2008/04/30	by sjw
****************************************************************************/
unsigned int CD_FRAME2MSF(unsigned int lFrame)
{
	unsigned char m,s;
	TIME_CD MSF;
	m = (unsigned char)(lFrame / 4500);
	lFrame -= (unsigned int)(m*4500);
	m = CD_HEX2BCD(m);
	MSF.t.ucMin = m;
	s = (unsigned char)(lFrame / 75);
	lFrame-= s*75;
	s = CD_HEX2BCD(s);
	MSF.t.ucSec = s;
	MSF.t.ucFrame = CD_HEX2BCD((unsigned char)lFrame);
	MSF.t.ucMode = 0;
	return MSF.ulValue;
}


/****************************************************************************
	CD_FRAME2HEX

	Description
 		Change Frame to HEX value

 	Parameter
 		lMSF : Minute Second Frame

	Return
		FAIL : Parsing fail
		SUCCESS : Parsing Success
													2009/03/09	by sjw
****************************************************************************/
unsigned int CD_FRAME2HEX(unsigned int lFrame)
{
	unsigned char m,s;
	TIME_CD MSF;
	m = (unsigned char)(lFrame / 4500);
	lFrame -= (unsigned int)(m*4500);
	MSF.t.ucMin = m;
	s = (unsigned char)(lFrame / 75);
	lFrame-= s*75;
	MSF.t.ucSec = s;
	MSF.t.ucFrame = (unsigned char)lFrame;
	MSF.t.ucMode = 0;
	return MSF.ulValue;
}


/****************************************************************************
 	CD_SetBufferStatus

	Description
	 	Set CD Buffering status

 	Parameter
		Format :  CD_BUF_START = 1
				CD_BUF_STOP = 2
				CD_BUF_REBUF = 3

	Return
		None
													2007/10/08	by sjw
****************************************************************************/
void  CD_SetBufferStatus(CD_BUF_STATUS Format)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	pCDStatus->gCDBufStatus = Format;
}


/****************************************************************************
	CD_GetBufferStatus

	Description
 		Get CD Buffering status

 	Parameter
		None

	Return
		1 = CD_BUF_START
		2 = CD_BUF_STOP
		3 = CD_BUF_REBUF
		4 = CD_SHOCK
													2007/10/08	by sjw
****************************************************************************/
CD_BUF_STATUS CD_GetBufferStatus(void)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	return pCDStatus->gCDBufStatus;
}


/****************************************************************************
 	CD_SetBufferStatus

	Description
	 	Set CD Error status

 	Parameter
		Format :  CD_BUF_FULL = 1,
				CD_MISS_SUBQ,

	Return
		None
													2007/10/18	by sjw
****************************************************************************/
void  CD_SetErrorStatus(CD_ERROR_STATUS Format)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	pCDStatus->gCDErrorStatus = Format;
}


/****************************************************************************
	CD_GetBufferStatus

	Description
 		Get CD Error status

 	Parameter
		None

	Return
		CD_BUF_FULL = 1,
		CD_MISS_SUBQ,
													2007/10/18	by sjw
****************************************************************************/
CD_ERROR_STATUS CD_GetErrorStatus(void)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	return pCDStatus->gCDErrorStatus;
}


/****************************************************************************
 	CD_SetLastSector

	Description
	 	Set read last sector

 	Parameter
		gLastSector : read last sector

	Return
		None
													2008/05/20	by sjw
****************************************************************************/
void  CD_SetLastSector(unsigned int lastSector)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	pCDStatus->gLastSector = lastSector;
}


/****************************************************************************
 	CD_GetLastSector

	Description
	 	Get read last sector

 	Parameter
		None

	Return
		gLastSector : read last sector
													2008/05/20	by sjw
****************************************************************************/
unsigned int  CD_GetLastSector(void)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	return pCDStatus->gLastSector;
}


/****************************************************************************
 	CD_SetRequestSector

	Description
	 	Set request sector

 	Parameter
		gLastSector : read request sector

	Return
		None
													2008/05/20	by sjw
****************************************************************************/
void  CD_SetRequestSector(unsigned int requestSector)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	pCDStatus->gRequestSector = requestSector;
}


/****************************************************************************
 	CD_GetLastSector

	Description
	 	Get read request sector

 	Parameter
		None

	Return
		gLastSector : read request sector
													2008/05/20	by sjw
****************************************************************************/
unsigned int  CD_GetRequestSector(void)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	return pCDStatus->gRequestSector;
}


/****************************************************************************
 	CD_SetCHOffet

	Description
	 	Set CD_Chunk buffer offset

 	Parameter
		offset : chunk buffer offset

	Return
		None
													2008/07/17	by sjw
****************************************************************************/
void  CD_SetCHOffet(unsigned int offset)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	pCDStatus->CHOffset = (offset << 1);
}


/****************************************************************************
 	CD_GetCHOffet

	Description
	 	Get CD_Chunk buffer offset

 	Parameter
		None

	Return
		Chunk buffer offset
													2008/07/17	by sjw
****************************************************************************/
unsigned int  CD_GetCHOffet(void)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	return pCDStatus->CHOffset;
}


/****************************************************************************
 	CD_SetLastSubQ

	Description
	 	Save Last read SubQ value

 	Parameter
		offset : chunk buffer offset

	Return
		None
													2008/07/18	by sjw
****************************************************************************/
void  CD_SetLastSubQ(unsigned int subQ)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	pCDStatus->LastSubQ = subQ;
}


/****************************************************************************
 	CD_GetLastSubQ

	Description
	 	Get Last read SubQ value

 	Parameter
		None

	Return
		Last read SubQ value
													2008/07/18	by sjw
****************************************************************************/
unsigned int  CD_GetLastSubQ(void)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	return pCDStatus->LastSubQ;
}


/****************************************************************************
 	CD_SetDiffValue

	Description
	 	Set difference value (request msf value between read msf value)

 	Parameter
		value : difference value

	Return
		None
													2008/05/15	by sjw
****************************************************************************/
void  CD_SetDiffValue(int value)
{
	int temp;
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();

	if(value > MECA_READ_MARGIN)
	{
		pCDStatus->DiffValue = 0;
	}	
	else if(value < -MECA_READ_MARGIN)
	{
		pCDStatus->DiffValue = 0;
	}	
	else
	{
                // Check ￇￊ﾿￤
		if(value > 1)			// Pickup locations in front of request position
		{
			temp = CD_GetDiffValue();
			pCDStatus->DiffValue = temp+value;
		}
		else if(value < 0) 	// Pickup locations back of request position
		{
			temp = CD_GetDiffValue();
			pCDStatus->DiffValue = temp+value;
		}
		else
		{
			/* No action */
		}
	}
}


/****************************************************************************
	CD_GetDiffValue

	Description
		Get difference value (request msf value between read msf value)

 	Parameter
		None

	Return
		difference value
													2008/05/15	by sjw
****************************************************************************/
int CD_GetDiffValue(void)
{
	CD_STATUS *pCDStatus;
	pCDStatus = pGetCDStatus();
	//return (pCDStatus->DiffValue - 5);
	return -CD_SEEK_POSITION;
}


/****************************************************************************
 	CD_KmpPatternSearch

 	Description:
		find pattern(compare buffer) using kmp algorithm

 	Parameter
 		pSrcAddr : source buffer
 		src_len : source length
		pPattern : pattern sample(compare buffer)
		pattern_len : pattern length

	Return
		SUCCESS : find point(length) in Address (>0)
		Null return

****************************************************************************/
int CD_KmpPatternSearch(const unsigned char *pSrcAddr, int src_len, const unsigned char *pPattern, unsigned int pattern_len)
{
	unsigned int i,matches;
	unsigned int *kmpNext;
	unsigned int lsize;
	int lRet = CD_FALSE;

	lsize = sizeof(unsigned int)*(pattern_len+1); 

	//printk ("CDIF_Driver : %s, size = %x\n", __FUNCTION__, lsize);
	
	kmpNext = kmalloc(1024, GFP_KERNEL); 
	if (kmpNext != 0)
	{
		//printk ("CDIF_Driver : allocated addres is %p\n", kmpNext);
		i = 1;
		matches = 0;
		kmpNext[0] = 0;

		while (i < pattern_len)
		{
			if (pPattern[i] == pPattern[matches])
			{
				matches++;
				kmpNext[i] = matches;
				i++;
			}
			else if ( matches > 0 )
			{
				matches = kmpNext[matches-1];
			}
			else
			{
				kmpNext[i] = 0;
				i++;
			}
		}

		i = 0;
		matches = 0;

		while ( i < (unsigned int)src_len )
		{
			if (pSrcAddr[i] == pPattern[matches])
			{
				matches++;

				if (matches == pattern_len)
				{
					lRet = (int)((i+1)-pattern_len);		/* QAC : 12.1 */
					break;
				}
				else
				{
					i++;
				}
			}
			else if ( matches > 0 )
			{
				matches = kmpNext[matches-1];
			}
			else
			{
				i++;
			}
		}

		(void) kfree(kmpNext);
	}
	else
	{
		printk ("%s, kmalloc failed\n", __FUNCTION__);
	}
	
	return lRet;
}


/****************************************************************************
	DAIGetCDDIBuffer

	Description
 		Get CDDIBuffer pointer

 	Parameter
		None

	Return
		None
													2007/10/08	by sjw
****************************************************************************/
SingleBufferState * DAIGetCDDIBuffer(void)
{
	// Return a pointer to the play buffer.
	return(&sCDDIBuffer);
}


/****************************************************************************
	CDDISetBuffer

	Description
 		sCDDIBuffer setting

 	Parameter
		None

	Return
		None
													2007/10/08	by sjw
****************************************************************************/
long CDDISetBuffer(SingleBufferState *psBuffer)
{
	tCDDIBUF *pCDDI;
	long res;
	pCDDI = &tCDDIBuffer;
	pCDDI->pOutput = psBuffer;
	res = SingleBufferSet(pCDDI->pOutput, pCDDI->psSingle, CDDI_BUFFER_SIZE);
	return res;
}

/****************************************************************************
	CD_ShockProcess

	Description
 		Process CD Shock status : minus 10 frame from write pointer in IO buffer.

 	Parameter
		pBuf : CD IO buffer

	Return
		NULL
													2008/07/20	by sjw
****************************************************************************/
void CD_ShockProcess(SingleBufferState *pBuf)
{
	int remainSize, ShockSampleSize, res;
	remainSize = SingleBufferDataAvailable(DAIGetCDDIBuffer());
	ShockSampleSize = CDDI_SHOCK_SIZE*CDDA_SAMPLE_SIZE;

	if(remainSize > ShockSampleSize)
	{
		if (pBuf->lWritePtr >= ShockSampleSize)
		{
			pBuf->lWritePtr -= ShockSampleSize;
		}
		else
		{
			res = ShockSampleSize - pBuf->lWritePtr;
			pBuf->lWritePtr = pBuf->lLength - res;
		}
	}
}


/****************************************************************************
	CD_GetDevStatus

	Description
 		return Force stop status

 	Parameter
		None

	Return
		CD_STATUS_NORMAL 0
		CD_STATUS_ERR -1
		CD_STATUS_FORCESTOP -2
													2008/09/19	by sjw
****************************************************************************/
int CD_GetDevStatus(void)
{
	return CD_DevStatus;
}


unsigned int datacd_enable = 0;	// is datacd?? 1 : data cd, 0 : audio cd
/****************************************************************************
	CD_SetDevForceStop

	Description
 		Set Force stop function. & Set Force stop status

 	Parameter
		1 : Force stop ON
		0 : Force stop OFF

	Return
		None
													2008/09/19	by sjw
****************************************************************************/
void CD_SetDevForceStop(unsigned int SetBit)
{
//	CD_DevForceStop |= SetBit;
	CD_DevForceStop = SetBit && 0xff;
	CD_DevStatus = CD_STATUS_FORCESTOP;
	if (SetBit) // In case of unmount
	{
		datacd_enable = 0;
	}
	else // In case of mount
	{
		datacd_enable = 1;
	}
    printk("[tcc-debug] cdif_api : FORCE STOP status activation\n");	
}


/****************************************************************************
	CD_ReleaseDevForceStop

	Description
 		Set Force stop function. & Set Force stop status

 	Parameter
		1 : Force stop ON
		0 : Force stop OFF

	Return
		None
													2011/07/30	by sjw
****************************************************************************/
void CD_ReleaseDevForceStop(unsigned int ClrBit)
{
	CD_DevForceStop &= ~ClrBit;
	
	if (CD_DevForceStop != 0)
	{
		CD_DevStatus = CD_STATUS_FORCESTOP;
	}
	else
	{	
		CD_DevStatus = CD_STATUS_NORMAL;
	}	
}


/****************************************************************************
	CD_IsDevForceStop

	Description
 		return Force stop Flag.

 	Parameter
		None

	Return
		1 : Force stop ON
		0 : Force stop OFF
													2008/09/19	by sjw
****************************************************************************/
unsigned char CD_IsDevForceStop(void)
{
	return CD_DevForceStop;
}


