/****************************************************************************
 *   FileName    : SingleBuffer.c
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
/****************************************************************************/
/* MODULE:                                                                  								*/
/*   buffer.c - BufferInit initializes a	circular buffer.                           					*/
/****************************************************************************/

#include "SingleBuffer.h"


void SingleBufferInit(SingleBufferState *psBuffer)
{
	// Initialize the circular buffer.
	psBuffer->pSingleBuf = 0;
	psBuffer->lLength = 0;

	// Initialize the circular buffer read and write pointers.
	psBuffer->lReadPtr = 0;
	psBuffer->lWritePtr = 0;
}

//****************************************************************************
//
// SingleBufferSet sets	the	address	and	length of the data buffer for this
// circular buffer.
//
//****************************************************************************
long SingleBufferSet(SingleBufferState *psBuffer, short *psSingle, long lLength)
{
	long lRet;

	// Make	sure that the size of the buffer is	a multiple of four samples.

	if (((unsigned long)lLength & 3) != 0)
	{
		lRet = 0;
	}
	else
	{
		// Remember	the	address	and	size of	the	buffer.
		psBuffer->pSingleBuf = psSingle;
		psBuffer->lLength = lLength;

		// Reset the read and write	pointers.
		psBuffer->lReadPtr = 0;
		psBuffer->lWritePtr = 0;

		lRet = 1;
	}

	// Return to the caller.
	return(lRet);
}

//****************************************************************************
//
// SingleBufferDataAvailable determines the number of	samples	that are currently in
// the circular	buffer.
//
//****************************************************************************
long SingleBufferDataAvailable(SingleBufferState	*psBuffer)
{
	long lRet, lRead, lWrite;

	// Get the current read	and	write pointers.
	lRead = psBuffer->lReadPtr;
	lWrite = psBuffer->lWritePtr;

	// Determine the samples available.

	if (lWrite >= lRead)
	{
		// The read pointer is before the write	pointer in the bufer.
		lRet = lWrite -	lRead;
	}
	else
	{
		// The write pointer is before the read	pointer in the buffer.
		lRet = psBuffer->lLength - (lRead - lWrite);
	}

	// Return to the caller.
	return(lRet);
}

//****************************************************************************
//
// SingleBufferSpaceAvailable determines the number of samples that can be added to
// the circular	buffer.
//
//****************************************************************************
long SingleBufferSpaceAvailable(SingleBufferState *psBuffer)
{
	long lRet, lRead, lWrite;

	lRead =	psBuffer->lReadPtr;
	lWrite = psBuffer->lWritePtr;

	if (lWrite == lRead)
	{
		lRet = psBuffer->lLength - 4;
	}
	else if (lWrite > lRead)
	{
		lRet = psBuffer->lLength - (lWrite - lRead) - 4;
	}
	else
	{
		lRet = lRead - lWrite - 4;
	}

	return(lRet);
}

//****************************************************************************
//
// SingleBufferIsEmpty determines if the circular buffer is empty.
//
//****************************************************************************
long SingleBufferIsEmpty(SingleBufferState *psBuffer)
{
	long lRet;

	// The circular buffer is empty if its read pointer	and	write pointer are identical.
	if (psBuffer->lReadPtr == psBuffer->lWritePtr)
	{
		// The buffer is empty.
		lRet = 1;
	}
	else
	{
		// The buffer is not empty.
		lRet = 0;
	}

	// Return to the caller.
	return(lRet);
}


//****************************************************************************
//
// SingleBufferUpdateWritePointer advances the write pointer for the buffer by the
// specified number of samples.
//
//****************************************************************************
long SingleBufferUpdateWritePointer(SingleBufferState *psBuffer, long lLength)
{
	long lRet, lRead, lMaxLength;
	long	lWrite;

	lLength = (long)((unsigned long)lLength & (~3UL));	// 20041207

	lRead =	psBuffer->lReadPtr;
	lWrite = psBuffer->lWritePtr;

	if (lRead > lWrite)
	{
		lMaxLength = lRead - lWrite	- 4;
	}
	else if (lRead == 0)
	{
		lMaxLength = psBuffer->lLength - lWrite - 4;
	}
	else
	{
		lMaxLength = psBuffer->lLength - lWrite;
	}

	if (lLength > lMaxLength)
	{
		lRet = 0;
	}
	else
	{
		lWrite += lLength;

		if (lWrite == psBuffer->lLength)
		{
			lWrite = 0;
		}

		psBuffer->lWritePtr = lWrite ;
		lRet = 1;
	}

	// Return to the caller.
	return(lRet);
}


//****************************************************************************
//
// Single BufferUpdateReadPointer advances the read pointer for the buffer by the
// specified number of samples.
//
//****************************************************************************
long SingleBufferUpdateReadPointer(SingleBufferState *psBuffer, long lLength)
{
	long lRet;
	
	// Update the read pointer.
	psBuffer->lReadPtr += lLength;

	if (psBuffer->lReadPtr > psBuffer->lLength)
	{
		psBuffer->lReadPtr = psBuffer->lReadPtr - psBuffer->lLength;
	}
	else if (psBuffer->lReadPtr == psBuffer->lLength)
	{
		psBuffer->lReadPtr = 0;
	}
	else
	{
		/* no statement */
	}

	if (psBuffer->lReadPtr >= psBuffer->lLength)
	{
		psBuffer->lReadPtr = psBuffer->lLength - psBuffer->lReadPtr;
}

	lRet = psBuffer->lReadPtr;

	// Return to the caller.
	return(lRet);
}

