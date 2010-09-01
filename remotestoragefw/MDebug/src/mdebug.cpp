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


// INCLUDES
#include <flogger.h>

#include "mdebug.h"

// FORWARD DECLARATIONS
void DoOutput(TDesC8& aData);

void DebugStringNarrowL(const char* aFmt, ...)
    {
    VA_LIST args;
    VA_START(args, aFmt);
    
    TPtrC8 fmt(reinterpret_cast<const unsigned char *>(aFmt));
    HBufC8* buf = HBufC8::NewLC(MAX_DEBUG_STRING_LENGTH);
    buf->Des().FormatList(fmt, args);
    buf->Des().Append('\n');
    DoOutput(*buf);
    CleanupStack::PopAndDestroy(); // buf
       
    VA_END(args);
    }

void DebugStringWideL(const char* aFmt, ...)
    {
    VA_LIST args;
    VA_START(args, aFmt);
    
    TPtrC8 fmt(reinterpret_cast<const unsigned char *>(aFmt));
    HBufC* fmt16 = HBufC::NewLC(fmt.Length());
    fmt16->Des().Copy(fmt);
    HBufC* buf = HBufC::NewLC(MAX_DEBUG_STRING_LENGTH);
    buf->Des().FormatList(*fmt16, args);
    buf->Des().Append('\n');
    HBufC8* buf8 = HBufC8::NewLC(buf->Length());
    buf8->Des().Copy(*buf);
    DoOutput(*buf8);
    CleanupStack::PopAndDestroy(3); // fmt16, buf, buf8

    VA_END(args);
    }

void DebugBufferL(const TDesC8& aBuf)
    {
    DebugStringNarrowL("\"%S\"", &aBuf);
    }

void DebugBufferL(const TDesC& aBuf)
    {
    DebugStringWideL("\"%S\"", &aBuf);
    }

void DebugTimeL(const TTime& aTime)
    {
    TBuf<64> dateTimeString;
    _LIT(KDateString, "%E%D%X%N%Y %1 %2 %3");
    aTime.FormatL(dateTimeString, KDateString);
    DebugBufferL(dateTimeString);
    _LIT(KTimeString, "%-B%:0%J%:1%T%:2%S%.%*C4%:3%+B");
    aTime.FormatL(dateTimeString, KTimeString);
    DebugBufferL(dateTimeString);
    }

void DoOutput(TDesC8& aData)
    {
    RFileLogger::Write(KDebugDirName,
                       KDebugFileName,
                       EFileLoggingModeAppend,
                       aData);
    }

// End of File
