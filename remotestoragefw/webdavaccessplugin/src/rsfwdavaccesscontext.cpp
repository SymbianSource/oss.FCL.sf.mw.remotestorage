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
* Description:  Maintain contexts for WebDAV transactions
 *
*/


// INCLUDE FILES
#include <httperr.h>
#include <dnd_err.h>
//#include "rsfwdavtransaction.h"
#include "rsfwdavaccesscontext.h"
#include "rsfwdavaccess.h"
#include "rsfwdavfileinfo.h"
#include "mdebug.h"
#include "xml/xmlparsererrors.h"
#include "rsfwdirentattr.h"
#include "rsfwdirent.h"


// CRsfwDavAccessContext
// ============================ MEMBER FUNCTIONS ==============================
// Destructor
CRsfwDavAccessContext::~CRsfwDavAccessContext()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContext::SubmitL
// Submit the operation.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContext::SubmitL()
    {
    iWebDavTransactionId = iWebDavTransaction->Id();
    iWebDavTransaction->SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContext::Retry
// Retry the operation.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContext::Retry()
    {
    TRAPD(err, StartL());
    if (err != KErrNone)
        {
        iStatus = err;
        iDone = ETrue;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContext::MapError
// Map an HTTP error code to SymbianOS error code.
// Whenever possible, these default mappings should be overriden
// in the derived classes.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContext::MapError(TInt& aError)
    {
    switch (aError)
        {
    case HTTPStatus::EBadRequest:
        aError = KErrBadName;
        break;

    case HTTPStatus::EMovedPermanently:
    case HTTPStatus::ETemporaryRedirect:
        aError = KErrNotFound;
        break;

    default:
        if (HTTPStatus::IsServerError(aError))
            {
            aError = KErrNotSupported;
            }
        else if (HTTPStatus::IsClientError(aError))
            {
            aError = KErrAccessDenied;
            }
        else if (HTTPStatus::IsRedirection(aError))
            {
            aError = KErrNotFound;
            }
        else
            {
            if (aError > 0)
                {
                // An arbitrary choice for error codes that should not occur
                aError = KErrAccessDenied;
                }
            }
        break;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContext::TargetPath
// return target path, to support cancel by path from 
// FM local dialogs
// This can be NULL for some non-cancellable operations like OPTIONS
// but note that for example when reading files the active operation may be 
// PROPFIND when user presses cancel.
// ----------------------------------------------------------------------------
//
const TDesC& CRsfwDavAccessContext::TargetPath()
    {
    return iRemotePathName;
    }


// ----------------------------------------------------------------------------
// Derived access contexts
// ----------------------------------------------------------------------------

// CRsfwDavAccessContextOptions
// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// CRsfwDavAccessContextOptions::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavAccessContextOptions*
CRsfwDavAccessContextOptions::NewL(CRsfwDavAccess* aDavAccess,
                               MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    CRsfwDavAccessContextOptions* self = new (ELeave) CRsfwDavAccessContextOptions;
    CleanupStack::PushL(self);
    self->ConstructL(aDavAccess, aResponseHandler);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextOptions::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextOptions::ConstructL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    DEBUGSTRING(("======= START ======="));
    iDavAccess = aDavAccess;
    iResponseHandler = aResponseHandler;
    iTryCount = KCommRetries;
    iRemotePathName = KNullDesC;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextOptions::StartL
// Submit the WebDAV transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextOptions::StartL()
    {
    DEBUGSTRING(("DAV: Options StartL"));
    iWebDavTransaction = iDavAccess->WebDavSession()->OptionsL();
    SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextOptions::TransactionCompleteL
// Handle a successfully completed transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextOptions::TransactionCompleteL()
    {
    DEBUGSTRING(("DAV: OptionsL done"));
    iStatus = KErrNone;
    iDone = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextOptions::TransactionError
// Handle a transaction fault
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextOptions::TransactionError(TInt aError)
    {
    DEBUGSTRING(("DAV: OptionsL raw err=%d", aError));
    if ((aError == KErrCommsLineFail) ||
        (aError == KErrNotReady) ||
        (aError == KErrDisconnected))
        {
        iTryCount--;
        }
    else
        {
        // Map protocol specific error codes into symbian error codes
        switch (aError)
            {
        case HTTPStatus::EMethodNotAllowed:
            // Server doesn't support DAV
            aError = KErrNotSupported;
            break;
            
        case HTTPStatus::EBadRequest:
            aError = KErrPathNotFound;
            break;
            
        case HTTPStatus::EBadGateway: // Proxy didn't find the server
        case HTTPStatus::ENotFound:
        case KErrDndNameNotFound: // name resolver didn't find the server
        case KErrDndAddrNotFound: 
        case KErrHttpCannotEstablishTunnel: // ssl error when server not found  
            aError = KErrNotFound;
            break;

        default:
            MapError(aError);
            break;
            }
        
        DEBUGSTRING(("DAV: OptionsL err=%d", aError));
        iStatus = aError;
        iTryCount = 0;
        }

    if (iTryCount)
        {
        DEBUGSTRING(("DAV: Retry %d", iTryCount));
        Retry();
        }
    else
        {
        iStatus = aError;
        iDone = ETrue;
        }
    }

// CRsfwDavAccessContextPropFindDir
// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPropFindDir::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavAccessContextPropFindDir* CRsfwDavAccessContextPropFindDir::NewL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aPathName,
    TInt aDepth,
    CRsfwDirEntAttr** aDirEntAttr,
    RPointerArray<CRsfwDirEnt>* aDirEnts)
    {
    CRsfwDavAccessContextPropFindDir* self =
        new (ELeave) CRsfwDavAccessContextPropFindDir;
    CleanupStack::PushL(self);
    self->ConstructL(aDavAccess,
                     aResponseHandler,
                     aPathName,
                     aDepth,
                     aDirEntAttr,
                     aDirEnts);
    CleanupStack::Pop(self);
    return self;
    }

CRsfwDavAccessContextPropFindDir::~CRsfwDavAccessContextPropFindDir()
    {
    iOwnDirEnts.ResetAndDestroy();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPropFindDir::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPropFindDir::ConstructL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aPathName,
    TInt aDepth,
    CRsfwDirEntAttr** aDirEntAttr,
    RPointerArray<CRsfwDirEnt>* aDirEnts)
    {
    DEBUGSTRING(("CRsfwDavAccessContextPropFindDir::ConstructL"));
    DEBUGSTRING16(("aPathName ='%S'", &aPathName));
    iDavAccess = aDavAccess;
    iResponseHandler = aResponseHandler;
    iDepth = aDepth;
    iDirEntAttr = aDirEntAttr;
    iDirEnts = aDirEnts;
    if (aDirEnts)
        {
        iDirEnts = aDirEnts;
        }
    else
        {
        iDirEnts = &iOwnDirEnts;
        }
    iTryCount = KCommRetries;
    iRemotePathName = aPathName;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPropfindDir::StartL
// Submit the WebDAV transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPropFindDir::StartL()
    {
    /*
      Preconditions:
      - we haven't seen the directory at all (there is no CRsfwDavFileInfo for it)
      - we have seen the directory and now the e-tag,
      but haven't fetched the contents
      - we have cached the contents also
    */  
    DEBUGSTRING(("DAV: PropFindDir StartL"));
    iDirEnts->ResetAndDestroy();
    iWebDavTransaction = iDavAccess->WebDavSession()->PropFindL(iRemotePathName,
                                                                iDepth,
                                                                ETrue,
                                                                *iDirEnts);
    SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPropFindDir::TransactionCompleteL
// Handle a successfully completed transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPropFindDir::TransactionCompleteL()
    {
    TInt err = KErrNone;
    DEBUGSTRING(("DAV: PropFindDir complete"));
        
    if (iDirEntAttr)
        {
        if (iDirEnts->Count())
            {
            *iDirEntAttr = (*iDirEnts)[0]->ExtractAttr();
            }
        }

#ifdef _DEBUG
    DEBUGSTRING(("DAV: DoPropFindDir: returning %d", err));
    TInt i;
    for (i = 0; i < iDirEnts->Count(); i++)
        {
        CRsfwDirEnt* d = (*iDirEnts)[i];
        TPtrC name(*d->Name());
        DEBUGSTRING16(("'%S' - (size=%d, attr=0x%x)",
                       &name,
                       d->Attr()->Size(),
                       d->Attr()->Att()));
        }
#endif // DEBUG

    iStatus = err;
    iDone = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPropFindDir::TransactionError
// Handle a transaction fault
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPropFindDir::TransactionError(TInt aError)
    {
    DEBUGSTRING(("DAV: PropFindDir raw err=%d", aError));
    if ((aError == KErrCommsLineFail) ||
        (aError == KErrNotReady) ||
        (aError == KErrDisconnected))
        {
        iTryCount--;
        }
    else
        {
        // Map XML parser errors to KErrCorrupt
        if ((aError <= EXmlFeatureLockedWhileParsing) &&
        	(aError >= EXmlParserError)) 
        	{
        	aError = EXmlParserError;
        	}
        
        // Map protocol specific error codes into symbian error codes
        switch (aError)
            {
       case EXmlParserError:
      		aError = KErrCorrupt;
      		break;      	
        case HTTPStatus::ENotFound:
            aError = KErrNotFound;
            break;
            
        case HTTPStatus::EForbidden:
        case HTTPStatus::EUnauthorized:
            aError = KErrAccessDenied;
            break; 
        case HTTPStatus::EMovedPermanently:
        case HTTPStatus::ETemporaryRedirect:
            // The object we are looking for exists, but has a different
            // type (e.g. we were looking for a file but it is a directory).
            // PROPFIND should return not found...
            aError = KErrNotFound;
            break;    
        default:
            MapError(aError);
            break;
            }

        DEBUGSTRING(("DAV: PropFindDir err=%d", aError));
        iStatus = aError;
        iTryCount = 0;
        }

    if (iTryCount)
        {
        DEBUGSTRING(("DAV: Retry %d", iTryCount));
        Retry();
        }
    else
        {
        iStatus = aError;
        iDone = ETrue;
        }
    }

// CRsfwDavAccessContextPropFindFile
// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPropFindFile::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavAccessContextPropFindFile* CRsfwDavAccessContextPropFindFile::NewL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aPathName,
    CRsfwDirEntAttr** aDirEntAttr)
    {
    CRsfwDavAccessContextPropFindFile* self =
        new (ELeave) CRsfwDavAccessContextPropFindFile;
    CleanupStack::PushL(self);
    self->ConstructL(aDavAccess, aResponseHandler, aPathName, aDirEntAttr);
    CleanupStack::Pop(self);
    return self;
    }

CRsfwDavAccessContextPropFindFile::~CRsfwDavAccessContextPropFindFile()
    {
    iOwnDirEnts.ResetAndDestroy();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPropfindFile::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPropFindFile::ConstructL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aPathName,
    CRsfwDirEntAttr** aDirEntAttr)
    {
    DEBUGSTRING(("CRsfwDavAccessContextPropFindFile::ConstructL"));
    DEBUGSTRING16(("aPathName ='%S'", &aPathName));
    iDavAccess = aDavAccess;
    iResponseHandler = aResponseHandler;
    iDirEntAttr = aDirEntAttr;
    iTryCount = KCommRetries;
    iRemotePathName = aPathName;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPropFindFile::StartL
// Submit the WebDAV transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPropFindFile::StartL()
    {
    DEBUGSTRING(("DAV: PropFindFile StartL"));
    iOwnDirEnts.ResetAndDestroy();
    iWebDavTransaction = iDavAccess->WebDavSession()->PropFindL(iRemotePathName,
                                                                0,
                                                                EFalse,
                                                                iOwnDirEnts);
    SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPropFindFile::TransactionCompleteL
// Handle a successfully completed transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPropFindFile::TransactionCompleteL()
    {
    DEBUGSTRING(("DAV: PropFindFile complete"));

    if (iOwnDirEnts.Count() > 0)
        {
        *iDirEntAttr = iOwnDirEnts[0]->ExtractAttr();
        }
    
    iStatus = KErrNone;
    iDone = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPropFindFile::TransactionError
// Handle a transaction fault
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPropFindFile::TransactionError(TInt aError)
    {
    DEBUGSTRING(("DAV: PropFindFile raw err=%d", aError));
    if ((aError == KErrCommsLineFail) ||
        (aError == KErrNotReady) ||
        (aError == KErrDisconnected))
        {
        iTryCount--;
        }
    else
        {
        // Map XML parser errors to KErrCorrupt
        if ((aError <= EXmlFeatureLockedWhileParsing) &&
        	(aError >= EXmlParserError)) 
        	{
        	aError = EXmlParserError;
        	}
        // Map protocol specific error codes into symbian error codes
        switch (aError)
            {
       case EXmlParserError:
      		aError = KErrCorrupt;
      		break;  
            
        case HTTPStatus::ENotFound:
            aError = KErrNotFound;
            break;
            
        case HTTPStatus::EForbidden:
        case HTTPStatus::EUnauthorized:
            aError = KErrAccessDenied;
            break;
        case HTTPStatus::EMovedPermanently:
        case HTTPStatus::ETemporaryRedirect:
            // The object we are looking for exists, but has a different
            // type (e.g. we were looking for a file but it is a directory).
            // PROPFIND should return not found...
            aError = KErrNotFound;
            break;        
        default:
            MapError(aError);
            break;
            }
        
        DEBUGSTRING(("DAV: PropFindFile err=%d", aError));
        iStatus = aError;
        iTryCount = 0;
        }

    if (iTryCount)
        {
        DEBUGSTRING(("DAV: Retry %d", iTryCount));
        Retry();
        }
    else
        {
        iStatus = aError;
        iDone = ETrue;
        }
    }

// CRsfwDavAccessContextGet
// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// CRsfwDavAccessContextGet::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavAccessContextGet*
CRsfwDavAccessContextGet::NewL(CRsfwDavAccess* aDavAccess,
                           MRsfwRemoteAccessResponseHandler* aResponseHandler,
                           const TDesC& aRemotePathName,
                           const TDesC& aLocalPathName,
                           TInt aOffset,
                           TInt* aLength,
                           TUint aFlags)
    {
    CRsfwDavAccessContextGet* self = new (ELeave) CRsfwDavAccessContextGet;
    CleanupStack::PushL(self);
    self->ConstructL(aDavAccess,
                     aResponseHandler,
                     aRemotePathName,
                     aLocalPathName,
                     aOffset,
                     aLength,
                     aFlags);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextGet::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextGet::ConstructL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aRemotePathName,
    const TDesC& aLocalPathName,
    TInt aOffset,
    TInt* aLength,
    TUint aFlags)
    {
    iDavAccess = aDavAccess;
    iResponseHandler = aResponseHandler;
    iRemotePathName = aRemotePathName;
    iLocalPathName = aLocalPathName;
    iOffset = aOffset;
    iLength = aLength;
    iFlags = aFlags;
    iTryCount = KCommRetries;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextGet::StartL
// Submit the WebDAV transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextGet::StartL()
    {
    DEBUGSTRING(("DAV: Get StartL"));
    iWebDavTransaction = iDavAccess->WebDavSession()->GetL(iRemotePathName,
                                                           iLocalPathName,
                                                           iOffset,
                                                           iLength,
                                                           iFlags);
    SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextGet::TransactionCompleteL
// Handle a successfully completed transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextGet::TransactionCompleteL()
    {
    DEBUGSTRING(("DAV: Get done"));
    
    iStatus = KErrNone;
    iDone = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextGet::TransactionError
// Handle a transaction fault
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextGet::TransactionError(TInt aError)
    {
    DEBUGSTRING(("DAV: Get raw err=%d", aError));
    if ((aError == KErrCommsLineFail) ||
        (aError == KErrNotReady) ||
        (aError == KErrDisconnected))
        {
        iTryCount--;
        }
    else
        {
        // Map protocol specific error codes into symbian error codes
        switch (aError)
            {
        case HTTPStatus::ENotFound:
            aError = KErrNotFound;
            break;

        case HTTPStatus::ENotModified:
            aError = KErrNone;
            break;

        case HTTPStatus::EForbidden:
            aError = KErrAccessDenied;
            break;

        default:
            MapError(aError);
            break;
            }

        DEBUGSTRING(("DAV: Get err=%d", aError));
        iStatus = aError;
        iTryCount = 0;
        }

    if (iTryCount)
        {
        DEBUGSTRING(("DAV: Retry %d", iTryCount));
        Retry();
        }
    else
        {
        iStatus = aError;
        iDone = ETrue;
        }
    }



// CRsfwDavAccessContextPut
// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPut::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavAccessContextPut*
CRsfwDavAccessContextPut::NewL(CRsfwDavAccess* aDavAccess,
                           MRsfwRemoteAccessResponseHandler* aResponseHandler,
                           const TDesC& aLocalPathName,
                           const TDesC& aRemotePathName,
                           const TDesC8& aMimeType,
                           TInt aOffset,
                           TInt aLength,
                           TInt aTotalLength,
                           const TDesC8* aLockToken)
    {
    CRsfwDavAccessContextPut* self = new (ELeave) CRsfwDavAccessContextPut;
    CleanupStack::PushL(self);
    self->ConstructL(aDavAccess,
                     aResponseHandler,
                     aLocalPathName,
                     aRemotePathName,
                     aMimeType,
                     aOffset,
                     aLength,
                     aTotalLength,
                     aLockToken);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPut::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPut::ConstructL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aLocalPathName,
    const TDesC& aRemotePathName,
    const TDesC8& aMimeType,
    TInt aOffset,
    TInt aLength,
    TInt aTotalLength,
    const TDesC8* aLockToken) 
    {
    iDavAccess = aDavAccess;
    iResponseHandler = aResponseHandler;
    iLocalPathName = aLocalPathName;
    iRemotePathName = aRemotePathName;
    iMimeType = aMimeType;
    iOffset = aOffset;
    iLength = aLength;
    iTotalLength = aTotalLength;
    iLockToken = aLockToken;
    iTryCount = KCommRetries;
    iContentRangeSupported = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPut::StartL
// Submit the WebDAV transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPut::StartL()
    {
    DEBUGSTRING(("DAV: Put StartL"));
    iWebDavTransaction =
        iDavAccess->
        WebDavSession()->
        PutL(iLocalPathName,
             iRemotePathName,
             iMimeType,
             iOffset,
             iLength,
             iTotalLength,
             iContentRangeSupported,
             iLockToken);
                                                        
    SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPut::TransactionCompleteL
// Handle a successfully completed transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPut::TransactionCompleteL()
    {
    DEBUGSTRING(("DAV: Put done"));
    iStatus = KErrNone;
    iDone = ETrue;
    }


// ----------------------------------------------------------------------------
// CRsfwDavAccessContextPut::TransactionError
// Handle a transaction fault
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextPut::TransactionError(TInt aError)
    {
    DEBUGSTRING(("DAV: Put raw err=%d", aError));
    if ((aError == KErrCommsLineFail) ||
        (aError == KErrNotReady) ||
        (aError == KErrDisconnected) ||
        (aError == HTTPStatus::ENotImplemented))
        {
        iTryCount--;
        }
    else
        {
        // Map protocol specific error codes into Symbian error codes
        switch (aError)
            {
        case HTTPStatus::EForbidden:
            aError = KErrAccessDenied;
            break;
            
        case HTTPStatus::EMethodNotAllowed:
        case HTTPStatus::EConflict:
            // EConflict can mean either resource already exists or
            // the parent does not exist.
            // However, in our code we make sure that the parent exists.
            aError = KErrAlreadyExists;
            break;

        case HTTPStatus::EUnsupportedMediaType:
            aError = KErrNotSupported;
            break;
        case HTTPStatus::EInternalServerError:
        case RsfwDavStatus::EInsufficientStorage:
            aError = KErrDiskFull;
            break;
        case HTTPStatus::EMovedPermanently:
        case HTTPStatus::ETemporaryRedirect:
            // It is not allowed to write a file with the same name
            // as an existing directory.
            aError = KErrAccessDenied;    
            break;
        default:
            MapError(aError);
            break;
            }

        DEBUGSTRING(("DAV: Put err=%d", aError));
        iStatus = aError;
        iTryCount = 0;
        }
    
    if (aError == HTTPStatus::ENotImplemented) 
        {
        // we assume that the server does not support Content-Range: with PUT
        iContentRangeSupported = EFalse;
        }
    
    if (iTryCount)
        {
        DEBUGSTRING(("DAV: Retry %d", iTryCount));
        Retry();
        }
    else
        {
        iStatus = aError;
        iDone = ETrue;
        }
    }

// CRsfwDavAccessContextMkDir
// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// CRsfwDavAccessContextMkDir::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavAccessContextMkDir*
CRsfwDavAccessContextMkDir::NewL(CRsfwDavAccess* aDavAccess,
                             MRsfwRemoteAccessResponseHandler* aResponseHandler,
                             const TDesC& aPathName)
    {
    CRsfwDavAccessContextMkDir* self = new (ELeave) CRsfwDavAccessContextMkDir;
    CleanupStack::PushL(self);
    self->ConstructL(aDavAccess, aResponseHandler, aPathName);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextMkDir::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextMkDir::ConstructL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aPathName)
    {
    iDavAccess = aDavAccess;
    iResponseHandler = aResponseHandler;
    iTryCount = KCommRetries;
    iRemotePathName = aPathName;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextMkDir::StartL
// Submit the WebDAV transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextMkDir::StartL()
    {
    DEBUGSTRING(("DAV: MkDir StartL"));
    iWebDavTransaction = iDavAccess->WebDavSession()->MkDirL(iRemotePathName);
    SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextMkDir::TransactionCompleteL
// Handle a successfully completed transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextMkDir::TransactionCompleteL()
    {
    DEBUGSTRING(("DAV: MkDir done"));
    iStatus = KErrNone;
    iDone = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextMkDir::TransactionError
// Handle a transaction fault
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextMkDir::TransactionError(TInt aError)
    {
    DEBUGSTRING(("DAV: MkDir raw err=%d", aError));
    if ((aError == KErrCommsLineFail) ||
        (aError == KErrNotReady) ||
        (aError == KErrDisconnected))
        {
        iTryCount--;
        }
    else
        {
        /*
          Map protocol specific error codes into Symbian error codes
          KErrAlreadyExists dirD already exists in /dirA/dirB/dirC/
          KErrAccessDenied dirD already exists but is not a directory.
          KErrDirFull There is no room in /dirA/dirB/dirC/ for the new entry,
          which is especially applicable to the root directory.
        */
        switch (aError)
            {
        case HTTPStatus::EForbidden:
        case HTTPStatus::EConflict:
            /*
              EConflict can mean either resource already exists
              but is not a collection  or the parent does not exist.
              However, in our code we make sure that the parent exists.
            */
            aError = KErrAccessDenied;
            break;

        case HTTPStatus::EMethodNotAllowed:
        case HTTPStatus::EMovedPermanently:
            aError = KErrAlreadyExists;
            break;

        case HTTPStatus::EUnsupportedMediaType:
            aError = KErrNotSupported;
            break;
    
        case HTTPStatus::EInternalServerError:
        case RsfwDavStatus::EInsufficientStorage:
            aError = KErrDirFull;
            break;

        default:
            MapError(aError);
            break;
            }

        DEBUGSTRING(("DAV: MkDir err=%d", aError));
        iStatus = aError;
        iTryCount = 0;
        }

    if (iTryCount)
        {
        DEBUGSTRING(("DAV: Retry %d", iTryCount));
        Retry();
        }
    else
        {
        iStatus = aError;
        iDone = ETrue;
        }
    }

// CRsfwDavAccessContextDelete
// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// CRsfwDavAccessContextDelete::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavAccessContextDelete*
CRsfwDavAccessContextDelete::NewL(CRsfwDavAccess* aDavAccess,
                              MRsfwRemoteAccessResponseHandler* aResponseHandler,
                              const TDesC& aPathName,
                              TBool aIsDir,
                              const TDesC8* aLockToken)
    {
    CRsfwDavAccessContextDelete* self = new (ELeave) CRsfwDavAccessContextDelete;
    CleanupStack::PushL(self);
    self->ConstructL(aDavAccess, aResponseHandler, aPathName, aIsDir, aLockToken);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextDelete::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextDelete::ConstructL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aPathName,
    TBool aIsDir,
    const TDesC8* aLockToken)
    {
    iDavAccess = aDavAccess;
    iResponseHandler = aResponseHandler;
    iIsDir = aIsDir;
    iLockToken = aLockToken;
    iTryCount = KCommRetries;
    iRemotePathName = aPathName;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextDelete::StartL
// Submit the WebDAV transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextDelete::StartL()
    {
    DEBUGSTRING(("DAV: Delete StartL"));
    iWebDavTransaction = iDavAccess->WebDavSession()->DeleteL(iRemotePathName,
                                                              iIsDir,
                                                              iLockToken);
    SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextDelete::TransactionCompleteL
// Handle a successfully completed transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextDelete::TransactionCompleteL()
    {
    DEBUGSTRING(("DAV: Delete done"));
    iDavAccess->RemoveDavFileInfoL(iRemotePathName);
    iStatus = KErrNone;
    iDone = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextDelete::TransactionError
// Handle a transaction fault
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextDelete::TransactionError(TInt aError)
    {
    DEBUGSTRING(("DAV: Delete raw err=%d", aError));
    if ((aError == KErrCommsLineFail) ||
        (aError == KErrNotReady) ||
        (aError == KErrDisconnected))
        {
        iTryCount--;
        }
    else
        {
        switch (aError)
            {
        case HTTPStatus::ENotFound:
            aError = KErrNotFound;
            break;

        case HTTPStatus::EForbidden:
        case RsfwDavStatus::ELocked:    
            aError = KErrAccessDenied;
            break;
        case HTTPStatus::EMovedPermanently:
        case HTTPStatus::ETemporaryRedirect:
            // We are attempting to delete a file
            // with the same name as an existing directory
            aError = KErrBadName;    
            break;

        default:
            MapError(aError);
            break;
            }

        DEBUGSTRING(("DAV: Delete err=%d", aError));
        iStatus = aError;
        iTryCount = 0;
        }

    if (iTryCount)
        {
        DEBUGSTRING(("DAV: Retry %d", iTryCount));
        Retry();
        }
    else
        {
        iStatus = aError;
        iDone = ETrue;
        }
    }

// CRsfwDavAccessContextMove
// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// CRsfwDavAccessContextMove::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavAccessContextMove*
CRsfwDavAccessContextMove::NewL(CRsfwDavAccess* aDavAccess,
                            MRsfwRemoteAccessResponseHandler* aResponseHandler,
                            const TDesC& aSrcPathName,
                            const TDesC& aDstPathName,
                            TBool aOverwrite,
                            const TDesC8* aSrcLockToken,
                            const TDesC8* aDstLockToken)
    {
    CRsfwDavAccessContextMove* self = new (ELeave) CRsfwDavAccessContextMove;
    CleanupStack::PushL(self);
    self->ConstructL(aDavAccess,
                     aResponseHandler,
                     aSrcPathName,
                     aDstPathName,
                     aOverwrite,
                     aSrcLockToken,
                     aDstLockToken);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextMove::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextMove::ConstructL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aSrcPathName,
    const TDesC& aDstPathName,
    TBool aOverwrite,
    const TDesC8* aSrcLockToken,
    const TDesC8* aDstLockToken)
    {
    iDavAccess = aDavAccess;
    iResponseHandler = aResponseHandler;
    iSrcPathName = aSrcPathName;
    iOverwrite = aOverwrite;
    iSrcLockToken = aSrcLockToken;
    iDstLockToken = aDstLockToken;
    iTryCount = KCommRetries;
    iRemotePathName = aDstPathName;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextMove::StartL
// Submit the WebDAV transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextMove::StartL()
    {
    DEBUGSTRING(("DAV: Move StartL"));
    iWebDavTransaction = iDavAccess->WebDavSession()->MoveL(iSrcPathName,
                                                            iRemotePathName,
                                                            iOverwrite,
                                                            iSrcLockToken,
                                                            iDstLockToken);
    SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextMove::TransactionCompleteL
// Handle a successfully completed transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextMove::TransactionCompleteL()
    {
    DEBUGSTRING(("DAV: Move done"));
    // move will delete the old file and does not lock the new one
    // so we just remove the token lock info
    iDavAccess->RemoveDavFileInfoL(iSrcPathName);
    iStatus = KErrNone;
    iDone = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextMove::TransactionError
// Handle a transaction fault
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextMove::TransactionError(TInt aError)
    {
    DEBUGSTRING(("DAV: Move raw err=%d", aError));
    if ((aError == KErrCommsLineFail) ||
        (aError == KErrNotReady) ||
        (aError == KErrDisconnected))
        {
        iTryCount--;
        }
    else
        {
        // Map protocol specific error codes into Symbian error codes
        switch (aError)
            {
        case HTTPStatus::ENotFound:
            aError = KErrNotFound;
            break;

        case HTTPStatus::EConflict: // source file not found
            aError = KErrNotFound;
            break;
        
        case HTTPStatus::EBadGateway:
        case HTTPStatus::EForbidden:
            aError = KErrBadName;
            break;
        
        case HTTPStatus::EPreconditionFailed:
            // Not performed due to the value of the Overwrite header
            aError = KErrAlreadyExists;     
            break;

        case RsfwDavStatus::ELocked:
            aError = KErrAccessDenied;
            break;
            
        case HTTPStatus::EMovedPermanently:
        case HTTPStatus::ETemporaryRedirect:
            // The operation failed as object exists, although its type 
            // didn't match the type of our parameter (file vs. dir)
            aError = KErrAlreadyExists;    
            break;
        default:
            MapError(aError);
            break;
            }

        DEBUGSTRING(("DAV: Move err=%d", aError));
        iStatus = aError;
        iTryCount = 0;
        }
    
    if (iTryCount)
        {
        DEBUGSTRING(("DAV: Retry %d", iTryCount));
        Retry();
        }
    else
        {
        iStatus = aError;
        iDone = ETrue;
        }
    }

// CRsfwDavAccessContextLock
// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// CRsfwDavAccessContextLock::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavAccessContextLock*
CRsfwDavAccessContextLock::NewL(CRsfwDavAccess* aDavAccess,
                            MRsfwRemoteAccessResponseHandler* aResponseHandler,
                            const TDesC& aPathName,
                            TUint aLockFlags,
                            TUint& aTimeout,
                            TDesC8** aLockToken)
    {
    CRsfwDavAccessContextLock* self = new (ELeave) CRsfwDavAccessContextLock;
    CleanupStack::PushL(self);
    self->ConstructL(aDavAccess,
                     aResponseHandler,
                     aPathName,
                     aLockFlags,
                     aTimeout,
                     aLockToken);
    CleanupStack::Pop(self);
    return self;
    }

CRsfwDavAccessContextLock::~CRsfwDavAccessContextLock()
    {
    delete iDavFileInfo;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextLock::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextLock::ConstructL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aPathName,
    TUint aLockFlags,
    TUint& aTimeout,
    TDesC8** aLockToken)
    {
    iDavAccess = aDavAccess;
    iResponseHandler = aResponseHandler;
    iLockFlags = aLockFlags;
    iTimeout = &aTimeout;
    iLockToken = aLockToken;
    iTryCount = KCommRetries;
    iRemotePathName = aPathName;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextLock::StartL
// Submit the WebDAV transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextLock::StartL()
    {
    DEBUGSTRING(("DAV: Lock StartL"));
    iWebDavTransaction = iDavAccess->WebDavSession()->LockL(iRemotePathName,
                                                            iLockFlags,
                                                            *iTimeout,
                                                            &iDavFileInfo);
    SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextLock::TransactionCompleteL
// Handle a successfully completed transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextLock::TransactionCompleteL()
    {
    DEBUGSTRING(("DAV: Lock done"));

    /*
      Preconditions:
      - we haven't seen the file at all (there is no fileInfo for it)
      or
      - we have cached it and there is fileinfo with a
      lock token (lock token is never assumed to be valid,
      if FileEngine decides to call this function)
    */
    *iTimeout = iDavFileInfo->Timeout();

    CRsfwDavFileInfo* storedFileInfo = iDavAccess->DavFileInfoL(iRemotePathName);
    HBufC8* lockToken;
    if (storedFileInfo)
        {
        // Lock operation succesful, set lock timeout
        if (iDavFileInfo->Name()) 
            {
            storedFileInfo->SetNameL(*(iDavFileInfo->Name()));
            }
        if (iDavFileInfo->LockToken()) 
            {
            storedFileInfo->SetLockTokenL(*(iDavFileInfo->LockToken()));
            }
        storedFileInfo->SetTimeout(iDavFileInfo->Timeout());
        lockToken = storedFileInfo->LockToken();
        }
    else 
        {
        // new file
        iDavAccess->AddDavFileInfo(iDavFileInfo);
        lockToken = iDavFileInfo->LockToken();
        // Ownership transferred to CRsfwDavAccess
        iDavFileInfo = NULL;
        }

    if (iLockToken)
        {
        // Allocate and return the lock token to the caller
        *iLockToken = lockToken->Des().AllocL();
        }
    
    iStatus = KErrNone;
    iDone = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextLock::TransactionError
// Handle a transaction fault
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextLock::TransactionError(TInt aError)
    {
    DEBUGSTRING(("DAV: Lock raw err=%d", aError));
    if ((aError == KErrCommsLineFail) ||
        (aError == KErrNotReady) ||
        (aError == KErrDisconnected))
        {
        iTryCount--;
        }
    else
        {
        // Map XML parser errors to KErrCorrupt
        if ((aError <= EXmlFeatureLockedWhileParsing) &&
        	(aError >= EXmlParserError)) 
        	{
        	aError = EXmlParserError;
        	}
        	
        // Map protocol specific error codes into Symbian error codes
        switch (aError)
            {
        case EXmlParserError:
        	 aError = KErrCorrupt;
             break;	
        case HTTPStatus::ENotFound:
            aError = KErrNotFound;
            break;

        case RsfwDavStatus::ELocked:
            aError = KErrAccessDenied;
            break;
            
        default:
            MapError(aError);
            break;
            }

        // Lock operation not successful.
        // If we have file info for this let's "reset" relevant parts
        // DavFileInfoL may leave if the path is over 255 chars
        // that shouldn't happen anymore here as we have already used the path 
        CRsfwDavFileInfo* davFileInfo = NULL;
        TRAPD(err, davFileInfo = iDavAccess->DavFileInfoL(iRemotePathName));
        if (!err && davFileInfo)
            {
            davFileInfo->ResetLockToken();
            davFileInfo->SetTimeout(0); 
            }
        
        DEBUGSTRING(("DAV: Lock err=%d", aError));
        iStatus = aError;
        iTryCount = 0;
        }
    
    if (iTryCount)
        {
        DEBUGSTRING(("DAV: Retry %d", iTryCount));
        Retry();
        }
    else
        {
        iStatus = aError;
        iDone = ETrue;
        }
    }

// CRsfwDavAccessContextRefreshLock
// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// CRsfwDavAccessContextRefreshLock::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavAccessContextRefreshLock* CRsfwDavAccessContextRefreshLock::NewL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aPathName,
    const TDesC8*  aLockToken,
    TUint& aTimeout)
    {
    CRsfwDavAccessContextRefreshLock* self =
        new (ELeave) CRsfwDavAccessContextRefreshLock;
    CleanupStack::PushL(self);
    self->ConstructL(aDavAccess,
                     aResponseHandler,
                     aPathName,
                     aLockToken,
                     aTimeout);
    CleanupStack::Pop(self);
    return self;
    }

CRsfwDavAccessContextRefreshLock::~CRsfwDavAccessContextRefreshLock()
    {
    delete iDavFileInfo;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextRefreshLock::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextRefreshLock::ConstructL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aPathName,
    const TDesC8* aLockToken,
    TUint& aTimeout)
    {
    iDavAccess = aDavAccess;
    iResponseHandler = aResponseHandler;
    iPathName = aPathName;
    iLockToken = aLockToken;
    iTimeout = &aTimeout;
    iTryCount = KCommRetries;
    
    // set remotepathname to null, so that this request cannot be cancelled
    // cancellations come from the UI and this is internal to RFE
    iRemotePathName = KNullDesC;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextRefreshLock::StartL
// Submit the WebDAV transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextRefreshLock::StartL()
    {
    DEBUGSTRING(("DAV: RefreshLock StartL"));
    iWebDavTransaction =
        iDavAccess->
        WebDavSession()->
        RefreshLockL(iPathName,
                     *iTimeout,
                     iLockToken,
                     &iDavFileInfo);
    SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextRefreshLock::TransactionCompleteL
// Handle a successfully completed transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextRefreshLock::TransactionCompleteL()
    {
    DEBUGSTRING(("DAV: RefreshLock done"));

    // The server may have changed the timeout,
    *iTimeout = iDavFileInfo->Timeout();
    iStatus = KErrNone;
    iDone = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextRefreshLock::TransactionError
// Handle a transaction fault
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextRefreshLock::TransactionError(TInt aError)
    {
    DEBUGSTRING(("DAV: RefreshLock raw err=%d", aError));
    if ((aError == KErrCommsLineFail) ||
// ?       (aError == KErrNotReady) ||
        (aError == KErrDisconnected))
        {
        iTryCount--;
        }
    else
        {            
        // Map XML parser errors to KErrCorrupt
        if ((aError <= EXmlFeatureLockedWhileParsing) &&
        	(aError >= EXmlParserError)) 
        	{
        	aError = EXmlParserError;
        	}
        	
        // Map protocol specific error codes into Symbian error codes
        switch (aError)
            {
        case EXmlParserError:
        	 aError = KErrCorrupt;
             break;	
        case HTTPStatus::ENotFound:
            aError = KErrNotFound;
            break;

        case HTTPStatus::EPreconditionFailed:
            aError = KErrArgument;
            break;

        case RsfwDavStatus::ELocked:
            aError = KErrLocked;
            break;
            
        default:
            MapError(aError);
            break;
            }

        DEBUGSTRING(("DAV: RefreshLock err=%d", aError));
        iStatus = aError;
        iTryCount = 0;
        }
    
    if (iTryCount)
        {
        DEBUGSTRING(("DAV: Retry %d", iTryCount));
        Retry();
        }
    else
        {
        iStatus = aError;
        iDone = ETrue;
        }
    }

// CRsfwDavAccessContextUnlock
// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// CRsfwDavAccessContextUnlock::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavAccessContextUnlock*
CRsfwDavAccessContextUnlock::NewL(CRsfwDavAccess* aDavAccess,
                              MRsfwRemoteAccessResponseHandler* aResponseHandler,
                              const TDesC& aPathName,
                              const TDesC8* aLockToken)
    {
    CRsfwDavAccessContextUnlock* self = new (ELeave) CRsfwDavAccessContextUnlock;
    CleanupStack::PushL(self);
    self->ConstructL(aDavAccess,
                     aResponseHandler,
                     aPathName,
                     aLockToken);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextUnlock::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextUnlock::ConstructL(
    CRsfwDavAccess* aDavAccess,
    MRsfwRemoteAccessResponseHandler* aResponseHandler,
    const TDesC& aPathName,
    const TDesC8* aLockToken)
    {
    iDavAccess = aDavAccess;
    iResponseHandler = aResponseHandler;
    iLockToken = aLockToken;
    iTryCount = KCommRetries;
    iRemotePathName = aPathName;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextUnlock::StartL
// Submit the WebDAV transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextUnlock::StartL()
    {
    DEBUGSTRING(("DAV: Unlock StartL"));
    iWebDavTransaction = iDavAccess->WebDavSession()->UnlockL(iRemotePathName,
                                                              iLockToken);
    SubmitL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextUnlock::TransactionCompleteL
// Handle a successfully completed transaction
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextUnlock::TransactionCompleteL()
    {
    DEBUGSTRING(("DAV: Unlock done"));

    // Remove the lock
    CRsfwDavFileInfo* davFileInfo = iDavAccess->DavFileInfoL(iRemotePathName);
    if (davFileInfo)
        {
        davFileInfo->ResetFlag(TRsfwDavFileInfoFlags::EUnlockPending);
        davFileInfo->ResetLockToken();
        iDavAccess->RemoveDavFileInfoL(iRemotePathName);
        } 

    iStatus = KErrNone;
    iDone = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccessContextUnlock::TransactionError
// Handle a transaction fault
// ----------------------------------------------------------------------------
//
void CRsfwDavAccessContextUnlock::TransactionError(TInt aError)
    {
    DEBUGSTRING(("DAV: Unlock raw err=%d", aError));
    
    // Currently we don't remove the lock token (CRsfwDavFileInfo struct)
    // if UNLOCK returns an error for some reason.
    // If lock timeouts are smallish, deleting would make sense
    // as the lock may have timed out in which case server returns an
    // error for UNLOCK and our client might not even care about
    // the result of UNLOCK.
    // In this case we have a risk of unwanted never-to-be-deleted
    // CRsfwDavFileInfos.
    // On the other hand,
    // if infinite timeout is used, the current behaviour is better 
    // (if UNLOCK fails for some reason the client must re-try).
    
    // at least remove 'unlock pending' flag
    CRsfwDavFileInfo* davFileInfo = NULL;
    TRAP_IGNORE(davFileInfo = iDavAccess->DavFileInfoL(iRemotePathName));
    
    if (davFileInfo)
        {
        davFileInfo->ResetFlag(TRsfwDavFileInfoFlags::EUnlockPending);
        } 
    
    if ((aError == KErrCommsLineFail) ||
        (aError == KErrNotReady) ||
        (aError == KErrDisconnected))
        {
        iTryCount--;
        }
    else
        {
        // Map protocol specific error codes into Symbian error codes
        MapError(aError);
        DEBUGSTRING(("DAV: Unlock err=%d", aError));
        iStatus = aError;
        iTryCount = 0;
        }
    
    if (iTryCount)
        {
        DEBUGSTRING(("DAV: Retry %d", iTryCount));
        Retry();
        }
    else
        {
        iStatus = aError;
        iDone = ETrue;
        }
    }

//  End of File
