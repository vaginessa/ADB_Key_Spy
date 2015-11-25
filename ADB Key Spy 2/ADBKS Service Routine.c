	//	//	ADB Key Spy Service Routine � Pete Gontier	//	Macintosh Developer Technical Support	//	��1995,1996,1998 Apple Computer, Inc.	//	//	Changes:	//	//		when		who		what	//		--------------------------------------------------------------	//		08/23/95	PG		history begins	//		08/24/95	PG		service routines split off into separate	//							code resource for use with PowerPC	//		09/11/95	PG		first cut at virtual key map support	//		09/11/95	PG		better third-party compatibility	//		01/11/96	PG		moved to extension-based project	//#ifndef __A4STUFF__#	include <A4Stuff.h>#endif#include "ADBKS Common.h"static pascal void KeyboardServiceRoutineC (ConstStr255Param, tKeyboardInfo *);asm void __Startup__ (void);	//	//	myDebugStr	//	DebugStrIf	//	//	These are two simple macros to aid in debugging the service routine.	//	You pretty much can't expect to use MacsBug while the service routine	//	(or any code it calls) is running. Why? Because MacsBug uses ADB to	//	talk to keyboards, and if ADB (this code) is buggy... well, you can't	//	type. If you need to debug the service routine, I suggest embedding	//	MacsBug commands in DebugStr strings followed by '; g', and	//	don't change any keys while the command is running.	//#define myDebugStr(x) \	do { long oldA4 = SetCurrentA4 ( ); DebugStr (x ";g"); SetA4 (oldA4); } while (0)#define myDebugStrIf(x) \	do { if (x) myDebugStr("\p" #x); } while (0)	//	//	__Startup__	//	//	The basic idea here is to extract the information from the	//	ADB interrupt message, then call the old service routine so	//	that GetKeys still works for other folks. The interface for	//	this routine is documented in Inside Mac V-371.	//asm void __Startup__ (void){	//	//	is it a 'Talk' message?	//	BTST		#2,D0	BEQ.S		bail						// no	BTST		#3,D0	BEQ.S		bail						// no	//	//	is it for register 0?	//	BTST		#0,D0	BNE.S		bail						// no	BTST		#1,D0	BNE.S		bail						// no	//	//	call C function to track key change	//	MOVEM.L		A0-A1/D0-D1,-(A7)			// save registers	MOVE.L		A0,-(A7)					// pass ADB data buffer	MOVE.L		A2,-(A7)					// pass private data address	JSR			KeyboardServiceRoutineC		// Geronimo!	MOVEM.L		(A7)+,A0-A1/D0-D1			// restore registersbail:	MOVE.L	(A2),-(A7)						// push the previous service routine address	MOVE.L	4(A2),A2						// set up the previous private data address	RTS										// jump to old service routine}	//	//	KeyboardServiceRoutineC	//	//	This routine is called from the (68K assembly) service routine.	//	Given a pointer to a copy of the keyboard's register 0 and	//	the keyboard map for the keyboard, it updates the map.	//	Note that A5 (or A4, as the case may be) is not valid here,	//	so you will have to perform arcane rites to access global variables.	//#pragma parameter SetD1 (__D1)void SetD1 (long) = { 0x4E71 };static pascal void KeyboardServiceRoutineC (ConstStr255Param adbDataBuffer, tKeyboardInfo *kbInfo){	//	//	Sanity check: have we been passed the expected number of bytes?	//	if (adbDataBuffer [0] == 2)	{		//		//	Find the the virtual key map entry affected by the		//	given raw key code. If there is no translator resource,		//	don't translate.		//			unsigned char		*keyMapEntry		= kbInfo->virtualKeyMap;		tKeyMapResourceH	keyMapResH			= kbInfo->keyMapResH;		unsigned char		adbDataBufferIndex	= 1;		do		{			unsigned char adbByte = adbDataBuffer [adbDataBufferIndex];			if (adbByte != 0xFF)			{				unsigned char rawKeyCodeIndex = 0x7F & adbByte;							if (!keyMapResH)					keyMapEntry += rawKeyCodeIndex;				else					keyMapEntry += (**keyMapResH).map [rawKeyCodeIndex];							//				//	If a key is going down, increment its counter.				//	If instead it's going up, make sure its counter				//	can be decremented before decrementing it.				//							if (!(0x80 & adbByte))					*keyMapEntry += 1;				else if (*keyMapEntry)					*keyMapEntry -= 1;			}		}		while (++adbDataBufferIndex <= 2);	}}