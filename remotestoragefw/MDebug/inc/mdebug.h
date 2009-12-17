/*
* Copyright (c) 2002-2004 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:  Debug printing to a log file
 *
*/


#ifndef MDEBUG_H
#define MDEBUG_H

// INCLUDES
#include <e32std.h>

#include "mydebug.h"

// Take care that the correct mydebug.h will be included
// In mydebug.h in your module inc directory,
// define your application-specific configuration like this:
// ----------------------------------------------------------
// Debug file - debug output is disabled if the parent dir does not exist
// _LIT(KDebugFileName, "C:\\logs\\remotefileengine\\remotefileengine.txt");

// Replace the current debug file - if not defined appends to the file
#ifndef APPEND_TO_DEBUG_FILE
#define REPLACE_DEBUG_FILE
#endif

// Maximum formatted size resulting from a single DEBUG* call
#ifndef MAX_DEBUG_STRING_LENGTH
#define MAX_DEBUG_STRING_LENGTH 4096
#endif
// ----------------------------------------------------------

// FUNCTION PROTOTYPES
void DebugInit(TBool aDebugSuspended = EFalse);
void SetDebugEnabled(TBool aValue);
void SetDebugSuspended(TBool aValue);
void DebugStringNarrowL(const char* aFmt, ...);
void DebugStringWideL(const char* aFmt, ...);
void DebugBufferL(const TDesC8& aBuf);
void DebugBufferL(const TDesC& aBuf);
void DebugTimeL(const TTime& aTime);

// MACROS
// If you output one or more narrow descriptors by using '%S',
//    use DEBUGSTRING8
// else if you output one or more unicode descriptors by using '%S',
//    use DEBUGSTRING16
// else
//    use DEBUGSTRING
//
// Narrow and unicode cannot be mixed in a single DEBUGSTRINGx call.

#ifdef _DEBUG
#define DEBUGINIT() DebugInit()
#define DEBUGINITSUSPENDED() DebugInit(ETrue)
#define DEBUGENABLE() SetDebugEnabled(ETrue)
#define DEBUGDISABLE() SetDebugEnabled(EFalse)
#define DEBUGSUSPEND() SetDebugSuspended(ETrue)
#define DEBUGCONTINUE() SetDebugSuspended(EFalse)
#define DEBUGSTRING(x) DebugStringNarrowL x
#define DEBUGSTRING8(x) DebugStringNarrowL x
#define DEBUGSTRING16(x) DebugStringWideL x
#define DEBUGBUFFER(x) DebugBufferL x
#define DEBUGTIME(x) DebugTimeL x
#else
#define DEBUGINIT()
#define DEBUGINITSUSPENDED()
#define DEBUGENABLE()
#define DEBUGDISABLE()
#define DEBUGSUSPEND()
#define DEBUGCONTINUE()
#define DEBUGSTRING(x)
#define DEBUGSTRING8(x)
#define DEBUGSTRING16(x)
#define DEBUGBUFFER(x)
#define DEBUGTIME(x)
#endif

#endif // MDEBUG_H

// End of File
