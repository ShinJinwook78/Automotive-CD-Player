/****************************************************************************
 *   FileName    : CD_Filesystem.h
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
*	Date: 2007. 06. 04 by S J W													*
****************************************************************************/
#ifndef __CD_FILESYSTEM_H__
#define __CD_FILESYSTEM_H__

#include "SingleBuffer.h"

extern unsigned char	TOCBUFF[1024*3]; /* test code */

/*Default*/
#ifndef CD_TRUE
#define CD_TRUE     1
#endif
#ifndef CD_FALSE
#define CD_FALSE    -1
#endif

#ifndef CD_SET
#define CD_SET      1
#endif
#ifndef CD_CLR
#define CD_CLR      0
#endif

#ifndef CD_SUCCESS
#define CD_SUCCESS  1
#endif
#ifndef CD_FAIL
#define CD_FAIL     0
#endif


/*Common Defines*/
#define DEFAULT_SECTOR              150         // 2 Seconds Sector size (75frame+75frame)	
#define SECTOR_SIZE                 2048        // 1sector = 2048byte
#define SECTOR_SAMPLE_SIZE          512         // 1sector = 2048byte / 4 (Sample size) 
#define CDDA_SAMPLE_SIZE            588         // SECTOR_SIZE / 4 (Sample size)
#define CDDA_SHORT_SIZE             1176        // Short type CDDA Sector Size
#define CDDA_SECTOR_SIZE            2352        // CDROM XA sector = 2352byte
#define CD_ISO_ID                   "CD001"
#define CD_UDF_ID                   "NSR"
#define CD_BEA_ID                   "BEA01"
#define CD_VolSeqNum                0x01000001  // Volume Sequence Number
#define THE_END                     100
#define PRIMARY_VOL                 16          // Primary Volume Descriptor Pointer
#define SUPPLE_VOL                  17          // Supplementary Volume Descriptor Pointer
#define CD_MAX_FD                   1
#define BP_ISO_IDENTIFIER           1           // Standard Identifier "CD001"
#define BP_UDF_IDENTIFIER           5           // Standard Identifier "BEA01"
#define ANCHOR_LOCATION             256         // First anchor pointer location is 256 sector
#define FID_BASIC_SIZE              38          // Basic length of File Identifier format
#define MODE1_DATA_LOCATION         16          // data location of Mode1
#define MODE2_DATA_LOCATION         24          // data location of mode value other than Mode1
#define MINUTE_LOCA                 12          // "minute" location in one frame
#define SECOND_LOCA                 13          // "second" location in one frame
#define FRAME_LOCA                  14          // "frame" location in one frame
#define MODE_LOCA                   15          // "mode" location in one frame
#define CD_HEAD_SIZE                12          // "00 FF FF FF FF FF FF FF FF FF FF 00" 12bytes
#define CD_MOVE_SIZE                16          // Default move size
#define CD_RETRY_CNT                3           // RETRY READ Count
#define CD_KMP_RETRY_CNT            2           // RETRY KMP READ Count
#define CD_C3_CNT                   2           // CD C3decoder Count
#define MAX_SESSION_NUM             50          // Max session number in Multi session CD
#define CD_SEEK_POSITION            6           // Pick up seek position (-5)
#define CD_SEEK_TOLERANCE           CD_SEEK_POSITION*2      // Pick up seek tolerance (miss)
#define CDDA_BUFFER_SIZE            CDDA_SECTOR_SIZE*200    // CD Chunk Buffer Size
#define PREVIOUS_DATA_SKIP_COUNT    5           // Skip previous data from Deck

#define CD_CMP_BYTE			24		// CD Compare byte
#define MODE1				1		// CD MODE
#define MODE2				2

/*CD TOC Structure*/
typedef struct _CDTOC
{
	unsigned char nSession;         // Current Session Number (Check on Audio Session)
	unsigned char ControlField;     // Check Digital Data
	unsigned char nTrack;           // Point of TOC, Track Number
	unsigned char Type;             // Check CD Type, Audio CD or Data CD
	//unsigned int	rMSF;           // Relative MSF
	unsigned int	aMSF;           // Absolute MSF

} CDTOC, *pCDTOC;


/*MSF Sturcture*/
typedef union _TIME_CD
{
	unsigned long ulValue;
	struct _CD_MSF
	{
		unsigned char ucFrame;
		unsigned char ucSec;
		unsigned char ucMin;
		unsigned char ucMode;
	} t;
} TIME_CD;


extern unsigned char CD_CmpBuf1[CD_CMP_BYTE]; /* QAC : 8.12 */
extern unsigned char CD_CmpBuf2[CD_CMP_BYTE]; /* QAC : 8.12 */

/* Extern */
extern unsigned int CD_byte2sector(unsigned long byte);
extern unsigned int CD_BCDMSF2FRAME(unsigned int lMSF);
extern unsigned int CD_HEXMSF2FRAME(unsigned int lMSF);
extern void Descramble_Sector(unsigned char *DATA_BLOCK);
extern int C3_decoder(unsigned char *DATA_BLOCK, unsigned char retry_cnt);
#endif
