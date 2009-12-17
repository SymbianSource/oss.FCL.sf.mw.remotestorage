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
* Description:  WebDAV specific constant definitions
*
*/


#ifndef RSFWDAVDEFS_H
#define RSFWDAVDEFS_H

// CONSTANTS
// max name for the server 
const TInt KMaxServerNameLen           = 210;

// max length for each connection parameter 
// (username, password etc.)
const TInt KMaxConnParameter           = 64;

// Size of buffer used when submitting request bodies (PUT, PROPFIND...)
const TInt KDefaultSubmitSize          = 14000; 

// Size of buffer used when reading a reply to file (like GET)
const TInt KDefaultFileBufferSize      = 81800;

// maximum length for range, content-range
// or timeout header values
const TInt KMaxFieldValueLength         = 64;

// length for the DAV version, which should be simply DAV: 1 or DAV: 1,2
const TInt KMaxDavVersionValue          = 15;

const TInt KDavResourceTypeCollection = 1;
const TInt KDavResourceTypeOther      = 0;
const TInt KDavVersionTwo              = 2;

// expat XML-parser wants the data in the chunks of 2k
// actually crashes otherwise...
const TInt KSymbianXmlParserMaxData    = 2048;

// overhead of UTF-8 encoding
const TInt KEncodingOverhead = 2;

// length of "http://"
const TInt KProtocolPrefix   = 7;

// when building "<target-url> (<target-token>)"
const TInt KTaggedLockTokenOverhead = 7;

// when building "<target-token>"
const TInt KLockTokenOverhead = 2;

_LIT8(KUserAgent,"S60 Remote Storage WebDav client");
_LIT8(KAccept, "*/*");
_LIT8(KTextXml, "text/xml");
_LIT8(KTextPlain, "text/plain");
_LIT8(KSecondDash, "Second-");

_LIT8(KParenthAngleFormat, "(<%S>)");
_LIT8(KTaggedParenthAngleFormat, "<%S> (<%S>)");
_LIT(KDateFormat,"%D%M%Y%/0%1%/1%2%/2%3%/3 %:0%H%:1%T%:2%S.%C%:3");

_LIT(KWebDavClientPanic, "WEBDAV-EC");

_LIT8(KWebDavPropFind,  "PROPFIND");
_LIT8(KWebDavMkCol,     "MKCOL");
_LIT8(KWebDavDelete,    "DELETE");
_LIT8(KWebDavCopy,      "COPY");
_LIT8(KWebDavPut,       "PUT");
_LIT8(KWebDavOptions,   "OPTIONS");
_LIT8(KWebDavMove,      "MOVE");
_LIT8(KWebDavLock,      "LOCK");
_LIT8(KWebDavUnlock,    "UNLOCK");

_LIT8(KWebDavDepth,     "Depth");
_LIT8(KWebDavIf,        "If");
_LIT8(KWebDavDest,      "Destination");
_LIT8(KWedDavLockToken, "Lock-Token");
_LIT8(KWebDavTimeout,   "Timeout");
_LIT8(KWebDavOverwrite, "Overwrite");
_LIT8(KWebDavOverwriteY,"T");
_LIT8(KWebDavOverwriteN,"F");
_LIT8(KWebDavNoProxy, "no-cache");
_LIT8(KKeepAlive, "keep-alive");


class RsfwDavStatus
/** 
Status code extensions to HTTP/1.1
Defined in RFC 2518 (WebDAV)
*/
    {
public:
    enum TRsfwDavStatus
        {
        /** 'Informational' range of codes 1xx */
        EProcessing                     = 102,
        /** 'Successful' range of codes 2xx */
        EMultiStatus                    = 207,
        /** 'Client Error' range of codes 4xx */
        EUnprocessableEntity            = 422,
        ELocked                         = 423,
        EFailedDependency               = 424, 
        /** 'Server Error' range of codes 5xx */
        EInsufficientStorage            = 507
        };
    };


// DATA TYPES
enum TWebDavClientPanics
    {
    EReqBodySumitBufferNotAllocated,
    KBodyWithInvalidSize,
    KCouldntNotifyBodyDataPart,
    KOutOfMemory
    };

enum TWebDavOp
    {
    EWebDavOpNone,
    EWebDavOpOptions,      
    EWebDavOpGet,
    EWebDavOpPut,
    EWebDavOpDelete,
    EWebDavOpPropFindSingle,
    EWebDavOpPropFindMulti,
    EWebDavOpMkCol,
    EWebDavOpMove,
    EWebDavOpLock,
    EWebDavOpUnlock,
    EWebDavOpRefreshLock
    };


#endif // RSFWDAVDEFS_H

// End of File
