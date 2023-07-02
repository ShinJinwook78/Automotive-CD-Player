/****************************************************************************
 *   FileName    : SingleBuffer.h
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

#ifndef __SINGLE_BUFFER_H__
#define __SINGLE_BUFFER_H__
//****************************************************************************
//
// The perisistent state of the circular buffer.  Do not re-arrange this
// structure as there is assembly which depends upon the layout not changing.
// There is a function pointer in this structure, making it appear that this
// would be better done as an object in C++.  It is not done that way since
// there is a large space overhead involved in adding the C++ support code/
// memory management.  We therefore implement a C++-like object in regular C
// code.
//
//****************************************************************************
typedef struct _SingleBufferState
{
	// A pointer to the circular buffer containing the data.
	short *pSingleBuf;

	// The length of the circular buffer.
	long lLength;

	// The next point in the circular buffer where data will be read.
	long lReadPtr;

	// The next point in the circular buffer where data will be written.
	long lWritePtr;

} SingleBufferState;

//****************************************************************************
//
// Function prototypes.
//
//****************************************************************************

extern SingleBufferState sCDDIBuffer;


extern void SingleBufferInit(SingleBufferState *psBuffer);
extern long SingleBufferSet(SingleBufferState *psBuffer, short *psSingle, long lLength);
extern long SingleBufferDataAvailable(SingleBufferState *psBuffer);
extern long SingleBufferSpaceAvailable(SingleBufferState *psBuffer);
extern long SingleBufferIsEmpty(SingleBufferState *psBuffer);
extern long SingleBufferUpdateWritePointer(SingleBufferState *psBuffer, long lLength);
extern long SingleBufferUpdateReadPointer(SingleBufferState *psBuffer, long lLength);

#endif // SINGLE_BUFFER_H_

/* end of file */

