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
* Description:  Handle WebDAV transactions
 *
*/


// INCLUDE FILES
//#include <HttpStringConstants.h>
#include <http/rhttpheaders.h>
#include <httperr.h>
#include <es_sock.h>

#include "rsfwremoteaccess.h"
#include "rsfwdavtransaction.h"
#include "rsfwdavsession.h"
#include "rsfwdavfileinfo.h"
#include "mdebug.h"

// ============================ MEMBER FUNCTIONS ==============================
// Destructor
CRsfwDavTransaction::~CRsfwDavTransaction()
    {
    DEBUGSTRING(("WebDAV transaction %d in destructor ...", Id()));
    iHttpTransaction.Close();
    iBodyFile.Close();
    delete iRequestBodyBuffer;
    delete iPropFindPath;
    delete iResponseBuffer;
    DEBUGSTRING(("... done"));
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction:::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
CRsfwDavTransaction* CRsfwDavTransaction::NewL(CRsfwDavSession* aWebDavSession,
                                             TWebDavOp aWebDavOp,
                                             const TUriC8& aUri,
                                             RStringF aMethod,
                                             TUint aTransactionId)
    {
    CRsfwDavTransaction* self = new (ELeave) CRsfwDavTransaction;
    CleanupStack::PushL(self);
    self->ConstructL(aWebDavSession,
                     aWebDavOp,
                     aUri,
                     aMethod,
                     aTransactionId);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::ConstructL(CRsfwDavSession* aWebDavSession,
                                    TWebDavOp aWebDavOp,
                                    const TUriC8& aUri, 
                                    RStringF aMethod,
                                    TInt aTransactionId)
    {
    iWebDavSession = aWebDavSession;
    iTransactionId = aTransactionId;
    // Borrow the file server from the parent
    iFs = iWebDavSession->FileServerSession();
    iWebDavOp = aWebDavOp;
    // User may cancel the IAP selection
    iStatus = KErrCancel;
    iHttpTransaction = iWebDavSession->HttpSession().OpenTransactionL(aUri,
                                                                      *this,
                                                                      aMethod);
    // Set body supplier as me
    switch (aWebDavOp)
        {
    case EWebDavOpPut:
    case EWebDavOpPropFindSingle:
    case EWebDavOpPropFindMulti:
    case EWebDavOpLock:
        iHttpTransaction.Request().SetBody(*this);
        break;

    default:
        break;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::SetBodyDataL
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::SetBodyData(HBufC8* aRequestBodyBuffer)
    {
    iRequestBodyBuffer = aRequestBodyBuffer;
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::SetBodyFileL
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::SetBodyFileL(const TDesC& aPath,
                                      TInt aOffset,
                                      TInt* aLength,
                                      TUint aFlags)
    {
    iBodyFilePath = aPath;
    iBodyFileOffset = aOffset;
    iBodyFileFlags = aFlags;
   
    iParsedBodyFilePath.Set(iBodyFilePath, NULL, NULL);
    DEBUGBUFFER((iParsedBodyFilePath.FullName()));
    if (iWebDavOp == EWebDavOpPut)
        {
        if (iBodyFilePath.Length())
            {
            
            // Check it exists and open a file handle
            if (!iFs.IsValidName(iBodyFilePath))
                {
                User::Leave(KErrPathNotFound);
                }
            TInt err = iBodyFile.Open(iFs,
                                      iParsedBodyFilePath.FullName(),
                                      EFileRead | EFileShareAny);
            if (err != KErrNone)
                {
                DEBUGSTRING(("Cannot set body file (err=%d)", err));
                User::Leave(err);
                }
            if ((*aLength) > 0) // partial PUT
                {
                iOverallDataSize =  *aLength;
                }
            else
                {
                User::LeaveIfError(iBodyFile.Size(iOverallDataSize));
                iOverallDataSize -= iBodyFileOffset;
                }
            iRequestBodyBuffer = HBufC8::NewL(KDefaultSubmitSize);
            }
        }
    else
        {
        // EWebDavOpGet
        iClientsLength = aLength;
        if (iClientsLength)
            {
            *iClientsLength = 0;
            }
        // store pointer to client's length variable and set it to zero
        // we will later reset it based on the content-length header
            
        if (!iFs.IsValidName(iBodyFilePath))
            {
            User::Leave(KErrPathNotFound);
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::SetDavFileInfoL
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::SetDavFileInfoL(CRsfwDavFileInfo** aDavFileInfo,
                                         const TDesC& aPath)
    {
    if (aDavFileInfo)
        {
        iDavFileInfo = CRsfwDavFileInfo::NewL();
        iDavFileInfo->SetNameL(aPath);
        *aDavFileInfo = iDavFileInfo;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::SetPropFindDirEntryArray
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::SetPropFindDirEntryArray(
    RPointerArray<CRsfwDirEnt>& aDirEnts)
    {
    // We do not get the ownership
    iDirEnts = &aDirEnts;
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::SetPropfindPath
// ----------------------------------------------------------------------------
void CRsfwDavTransaction::SetPropFindPath(HBufC* aPropFindPath)
    {
    // We get the ownership
    iPropFindPath = aPropFindPath;
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::Submit
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::SubmitL()
    {
    DEBUGSTRING(("submitting WebDav operation %d, id=%d ...",
                 iWebDavOp,
                 Id()));
    TRAPD(err, iHttpTransaction.SubmitL());
    if (err != KErrNone)
        {
        DEBUGSTRING(("WebDav transaction %d left with err %d!",
                     Id(),
                     err));
        // Release resources
        Cleanup();
        User::Leave(err);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::Cancel
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::Cancel()
    {
    DEBUGSTRING(("Canceling WebDav transaction %d ...", Id()));
    iHttpTransaction.Cancel();
    // Cancel XML parsing, if ongoing
    iWebDavSession->CancelParsing(iWebDavOp);
    
    // We don't receive any callback from the HTTP stack,
    // as we have not registered a filter to get ECancel event,
    // so we have to initiate the cleanup process ourselves...
    iStatus = KErrCancel;
    TransactionError();
    }

// ------------------------------------------------------------------
// From MHTTPTransactionCallback
// ------------------------------------------------------------------

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::MHFRunL
// Called when an action takes place on the transaction.
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::MHFRunL(RHTTPTransaction aTransaction,
                                 const THTTPEvent& aEvent)
    {
    DEBUGSTRING(("MHFRunL() transaction %d (http id = %d) status: %d",
                 iTransactionId,
                 aTransaction.Id(),
                 aEvent.iStatus));

    TBool done = EFalse;
    TBool ok = EFalse;
    switch (aEvent.iStatus)
        {
    case THTTPEvent::EGotResponseHeaders:     // process the headers
        {
        // HTTP response headers have been received.
        // We can determine now if there is
        // going to be a response body to save.
        RHTTPResponse response = aTransaction.Response();
        iStatus = response.StatusCode();
        DEBUGSTRING(("MHFRunL() THTTPEvent::EGotResponseHeaders, status %d",
                     iStatus));
        
        if (iWebDavOp == EWebDavOpOptions)
            {
            // Get a pointer to the session owned by WebDavSession
            RHTTPSession session = aTransaction.Session();
            RHTTPHeaders hdr = response.GetHeaderCollection();
            
            // Get DAV:- header,
            // which should be DAV: 1 if WebDAV v. 1 supported
            // or DAV: 1, 2 if also WebDAV v. 2 is supported
            TBuf8<KMaxDavVersionValue> davBuffer;
            _LIT8(KDav, "DAV");
            davBuffer.Append(KDav);
            RStringF davString =
                session.StringPool().OpenFStringL(davBuffer); 
            THTTPHdrVal val;
            TInt r = hdr.GetField(davString, 0, val);
            davString.Close();
            
            if (r == KErrNone)
                {
                RStringF valStr;
                valStr = val.StrF();
                TPtrC8 davTag = valStr.DesC();
                DEBUGSTRING8(("DAV: DAV=%S", &davTag));
                TLex8 lex(davTag);        
                // Skip over non-numeric chars as long as the list continues...
                TChar theChar;
                TInt supportedLevel = -1;
                while (lex.Peek()) 
                    {
                    theChar = lex.Get();
                    if (theChar.IsDigit()) 
                    	{
                    	supportedLevel = theChar.GetNumericValue();
                    	}
                    }
                if (supportedLevel == -1) 
                	{
               		// DAV not supported...
                    User::Leave(KErrNotSupported);	
                	}
                iWebDavSession->SetWebDavSupportClass(supportedLevel);
                }
            else
                {
                DEBUGSTRING(("DAV=<empty>"));
                if (iStatus == HTTPStatus::EUnauthorized)
                    {
                    User::Leave(KErrAccessDenied);
                    }
                else
                    {
                    if (iStatus == HTTPStatus::EOk) 
                    	{
                    	// everything seemed to be ok,
                    	// but no DAV: header
                    	User::Leave(KErrNotSupported);
                    	}
                    else 
                    	{
                    	User::Leave(iStatus);
                   		}
                    }
                }
            }
        // Get the content length and create iResponseBuffer,
        // where the body will be stored.
        // However, if we are copying file from the server,
        // the body (file) will just be stored into
        // the container file, no memory buffer is needed
        if (response.HasBody() &&
            // iStatus must be in 'Successful' range of HTTP codes 2xx
            (iStatus >= 200) && 
            (iStatus < 300) &&
            (iStatus != HTTPStatus::ENoContent))
            {
            DEBUGSTRING(("MHFRunL() response has a body..."));
            MHTTPDataSupplier* responseBody = response.Body();
            switch (iWebDavOp)
                {
            case EWebDavOpGet:
                {
                // Prepare to dump the body to container file
                TInt err;
                if (iStatus == HTTPStatus::EPartialContent) 
                    {
                    err = iBodyFile.Open(iFs,
                                         iParsedBodyFilePath.FullName(),
                                         EFileWrite | EFileShareAny);
                    if (err == KErrNone)
                        {
                        if (!(iBodyFileFlags &
                              KRemoteAccessOptionGetToStartOfFile))
                            {
                            TInt pos = iBodyFileOffset;
                            DEBUGSTRING(("Get: partial content received, seek to %d", pos));
                            iBodyFile.Seek(ESeekStart, pos);
                            if (pos != iBodyFileOffset)
                                {
                                // Should never happen
                                DEBUGSTRING(("Get: seek to %d failed (pos=%d)",
                                             iBodyFileOffset,
                                             pos));
                                User::Leave(KErrArgument);
                                }
                            }
                        }
                    else 
                        {
                        DEBUGSTRING(("Get: partial content received, will overwrite the cache file, err=%d", err));
                        User::LeaveIfError(
                            iBodyFile.Replace(
                                iFs,
                                iParsedBodyFilePath.FullName(),
                                EFileWrite | EFileShareAny));
                        }
                    }
                else // overwrite...
                    {
                    DEBUGSTRING(("Get: full content received"));
                    User::LeaveIfError(
                        iBodyFile.Replace(
                            iFs,
                            iParsedBodyFilePath.FullName(),
                            EFileStream | EFileWrite | EFileShareAny));
                    }
                
                // larger memory buffer
                TInt dataSize = responseBody->OverallDataSize();
                DEBUGSTRING(("MHFRunL() creating a response buffer, size=%d",
                             dataSize));
                if (dataSize <= KDefaultFileBufferSize) // 40k
                    {
                    iResponseBuffer = HBufC8::NewL(dataSize);
                    }
                else
                    {
                    iResponseBuffer = HBufC8::NewL(KDefaultFileBufferSize);
                    }
                
                }
                break;

            case EWebDavOpPropFindSingle:
            case EWebDavOpPropFindMulti:
            case EWebDavOpLock:
            case EWebDavOpRefreshLock:
                {
                if (iWebDavOp == EWebDavOpPropFindMulti)
                    {
                    PropFindResponseBeginL(KDavResourceTypeCollection);
                    }
                else if (iWebDavOp == EWebDavOpPropFindSingle)
                    {
                    PropFindResponseBeginL(KDavResourceTypeOther);
                    }
                else
                    {
                    LockQueryResponseBegin();
                    }
                }
                break;
                
            default:
                {
                // We are not interested in any body
                iDiscardBody = ETrue;
                }
                break;
                }
            }
        else
            {
            DEBUGSTRING(("MHFRunL() response does not have a body..."));
            iDiscardBody = ETrue;
            }
        }
        break;

    case THTTPEvent::EGotResponseBodyData:
        {
        DEBUGSTRING(("MHFRunL() THTTPEvent::EGotResponseBodyData"));
        // Get the body data supplier
        TBool allDone;
        
        MHTTPDataSupplier* responseBody = aTransaction.Response().Body();
        ASSERT(responseBody);
               
        // Some (more) body data has been received (in the HTTP response)   
        TPtrC8 bodyData;
        allDone = responseBody->GetNextDataPart(bodyData);
        DEBUGSTRING(("MHFRunL() body size = %d (all=%d)",
                     bodyData.Length(),
                     allDone));

        if (!iDiscardBody)
            {
            switch (iWebDavOp) 
                {
            case EWebDavOpPropFindSingle:
            case EWebDavOpPropFindMulti:
                {
                ParsePropFindResponseL(bodyData);
                if (allDone)
                    {
                    PropFindResponseEndL();
                    }             
                }
                break;
            case EWebDavOpLock:
            case EWebDavOpRefreshLock:
                {
                ParseLockResponseL(bodyData);
                if (allDone)
                    {
                    LockResponseEndL();
                    }             
                }
                break;
            case EWebDavOpGet: 
                {
                // set client's length variable
                // based on the amount of data fetched
                if (iClientsLength) 
                    {
                    *iClientsLength += bodyData.Length();
                    }
           
                TPtr8 responseBodyPtr = iResponseBuffer->Des();
                if ((responseBodyPtr.Length() + bodyData.Length()) <=
                    KDefaultFileBufferSize)
                    {
                    responseBodyPtr.Append(bodyData);
                    }
                else 
                    {
                    DEBUGSTRING(("Get: writing to cache file: %d bytes",
                                 responseBodyPtr.Length()));
                    User::LeaveIfError(iBodyFile.Write(responseBodyPtr));
                    // Reset buffer with new data
                    responseBodyPtr.Copy(bodyData);
                    }
                
                if (allDone) // this was the last chunk
                    {
                    DEBUGSTRING(("Get: writing to cache file %d bytes and closing",
                                 responseBodyPtr.Length()));
                    User::LeaveIfError(iBodyFile.Write(responseBodyPtr));
                    iBodyFile.Close();
                    }
                }
                break;
            
            default:
                break;
                }
            }
        
        // Done with that bit of body data
        responseBody->ReleaseData();
        }
        break;
        
    case THTTPEvent::EResponseComplete:
        {
        // The transaction's response is complete
        DEBUGSTRING(("Transaction Complete"));
        }
        break;

    case THTTPEvent::ESucceeded:
        {
        DEBUGSTRING(("Transaction Successful"));
        // We need to process the iStatus for the different cases
        switch (iWebDavOp)
            {
        case EWebDavOpPropFindMulti:
        case EWebDavOpPropFindSingle:
            if (iStatus == RsfwDavStatus::EMultiStatus)
                {
                iStatus = KErrNone;
                ok = ETrue;
                }
            break;                            
            // Other states which need processing of reponses
        case EWebDavOpDelete:
            {
            // DELETE contains the status of the response in a XML document
            // STATUS tag which should be parsed to produce an error
            // condition for this working is that we get back a
            // status from the server of 204: No Content
            if ((iStatus == HTTPStatus::ENoContent ||
            	iStatus == HTTPStatus::EOk))
                {
                ok = ETrue;
                }
            
            // Note that if the server reported an error they usually
            // return 207 multistatus with an xml body
            // containing the status of the DELETE
            // should parse the body here
            }
            break;

        case EWebDavOpMove:
        case EWebDavOpMkCol:
        case EWebDavOpPut:
            if ((iStatus == HTTPStatus::EOk) ||
            	(iStatus == HTTPStatus::ECreated) ||
                (iStatus == HTTPStatus::ENoContent))
                {
                // 200, 201 or 204 would be the expected response
                // that everything went well
                DEBUGSTRING(("Move/MkCol/Put: status ok"));
                ok = ETrue;
                }
            else
                {
                // 207 responses contains a status tag in xml data
                // 409 would indicate there was a conflict
                DEBUGSTRING(("Move/MkCol/Put: bad status!"));
                }
            break;

        case EWebDavOpGet:
            if ((iStatus == HTTPStatus::EOk) ||
                (iStatus == HTTPStatus::EPartialContent))
                {
                ok = ETrue;
                }
            break;
                
        case EWebDavOpOptions: 
            if (iStatus == 200) 
                {
                ok = ETrue;
                }
            break;
            
        case EWebDavOpLock:
        case EWebDavOpRefreshLock:
            {
            if ((iStatus == HTTPStatus::ECreated) || 
                (iStatus == HTTPStatus::EOk)) 
                {
                ok = ETrue;
                }
            }
            break;

        case EWebDavOpUnlock:
            ok = ETrue;
            break;

        default:
            break;
            }

        done = ETrue;
        }
        break;
        
    case THTTPEvent::EFailed:
        {
        switch (iWebDavOp)
            {
        case EWebDavOpOptions:
            iStatus = KErrAccessDenied;
            break;
            
        default:
            break;
            }
        
        DEBUGSTRING(("Transaction failed"));
        done = ETrue;
        }
        break;
        
    case THTTPEvent::ERedirectedPermanently:
    case KErrHttpRedirectNoLocationField:
        {
        DEBUGSTRING(("Permanent Redirection"));
        iStatus = HTTPStatus::EMovedPermanently;
        done = ETrue;
        }
        break;

    case THTTPEvent::ERedirectedTemporarily:
        {
        DEBUGSTRING(("Temporary Redirection"));
        iStatus = HTTPStatus::ETemporaryRedirect;
        done = ETrue;
        }
        break;
        
    case THTTPEvent::ERedirectRequiresConfirmation:
        iStatus = HTTPStatus::EMovedPermanently;
        DEBUGSTRING(("Requires Confirmation"));
        // we don't want to resend the request with the new url
        // let's just close the request
        iHttpTransaction.Close();
        done = ETrue;
        break;
        
    case THTTPEvent::EUnrecoverableError:
        DEBUGSTRING(("Unrecoverable error"));
        // Go on - we will end up to EFailed later
        break;
        
    case THTTPEvent::ETooMuchRequestData:
        DEBUGSTRING(("Too much request data"));
        break;
    // ESock errors:
   	case KErrConnectionTerminated:
        DEBUGSTRING(("Connection Terminated"));	
        iStatus = KErrCommsLineFail;
    	break;
    default:
        {
        // Check the httperr.h header for the meaning of the different fields
        DEBUGSTRING(("unrecognized event: %d", aEvent.iStatus));
        iStatus = aEvent.iStatus;
        // Close off the transaction if it's an error
        if (iStatus < 0)
            {
            done = ETrue;
            }       
        }
        break;
        }

    if (done)
        {
        if (ok)
            {
            TransactionCompleteL();
            }
        else
            {
            TransactionError();
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::MHFRunError
// ----------------------------------------------------------------------------
//
TInt CRsfwDavTransaction::MHFRunError(TInt aError,
                                     RHTTPTransaction /* aTransaction*/ ,
                                     const THTTPEvent& /* aEvent */)
    {    
    DEBUGSTRING(("MHFRunError() http transaction fired with error %d",
                 aError));

    iStatus = aError;
    TransactionError();
    return KErrNone;
    }

// ------------------------------------------------------------------
// From MHTTPDataSupplier
// ------------------------------------------------------------------

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::GetNextDataPart
// return ETrue if all data has been sent, EFalse otherwise
// ----------------------------------------------------------------------------
//
TBool CRsfwDavTransaction::GetNextDataPart(TPtrC8& aDataPart)
    {
    DEBUGSTRING(("GetNextDataPart() (iMoreToCome = EFalse)"));
    // Read from the request body file
    iMoreToCome = EFalse;

    // we cannot supply more data if the condition is true
    if ((!iRequestBodyBuffer) ||
        (iOverallDataSize == 0 && iWebDavOp == EWebDavOpPut)) 
        {
        DEBUGSTRING(("All data supplied"));
        return !iMoreToCome;
        }

    switch (iWebDavOp)
        {
    case EWebDavOpOptions:
    case EWebDavOpPropFindSingle:
    case EWebDavOpPropFindMulti:
    case EWebDavOpLock:
        {
        // Called when the request body is being filled
        aDataPart.Set(*iRequestBodyBuffer);
        }
        break;
        
    case EWebDavOpPut:
        {
        DEBUGSTRING(("Put: GetNextDataPart()"));
        if (iSendDataCount == 0) 
            {
            // first run
            DEBUGSTRING(("first run"));
            TInt pos = iBodyFileOffset;
            iBodyFile.Seek(ESeekStart, pos);            
            }
        else 
            {
            DEBUGSTRING(("%d bytes of data have been sent", iSendDataCount));
            }
            
        // We read data that will be given to the stack next time,
        // or we will find out that there is no more data...
        TInt readLength;
        if ((iOverallDataSize - iSendDataCount) >= KDefaultSubmitSize)
            {
            readLength = KDefaultSubmitSize;
            }
        else
            {
            readLength = iOverallDataSize - iSendDataCount;
            }
            
        
        TPtr8 requestBodyBufferPtr = iRequestBodyBuffer->Des();
        TInt err = iBodyFile.Read(requestBodyBufferPtr, readLength);
        iSendDataCount = iSendDataCount + iRequestBodyBuffer->Length();
        if (err == KErrNone) 
            {
            DEBUGSTRING(("passing %d bytes of data to the HTTP stack", iRequestBodyBuffer->Length()));
            if ((iSendDataCount < iOverallDataSize) &&  iRequestBodyBuffer->Length() > 0)
                {  
                DEBUGSTRING(("Put: More data to come (iMoreToCome = ETrue)"));
                iMoreToCome = ETrue;
                }
             else 
                {
                DEBUGSTRING(("Put: all data has been exhausted"));
                iMoreToCome = EFalse;
                }
            }
        else
            {
            DEBUGSTRING(("Put: failed to read the local file (err=%d)",
                         err));
            iMoreToCome = EFalse;
            }
        aDataPart.Set(*iRequestBodyBuffer); 
            
        break;
        }

    default:
        break;
        }
    
    return !iMoreToCome;
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::ReleaseData
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::ReleaseData()
    {
    if (iMoreToCome) 
        {
        TRAP_IGNORE(iHttpTransaction.NotifyNewRequestBodyPartL());
        }
    else 
        {
        DEBUGSTRING(("Releasing request body buffer"));
        delete iRequestBodyBuffer;
        iRequestBodyBuffer = NULL;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::OverallDataSize
// ----------------------------------------------------------------------------
//
TInt CRsfwDavTransaction::OverallDataSize()
    {
    TInt ods;
    switch (iWebDavOp)
        {
    case EWebDavOpPut:
        DEBUGSTRING(("Put: OverallDataSize returned %d",
                     iOverallDataSize));
        // the size of the file to be copied
        ods = iOverallDataSize;
        break;
        
    default:
        if (!iRequestBodyBuffer) 
            {
            ods = 0;
            }
        else 
            {
            ods = iRequestBodyBuffer->Length();
            }
        
        break;
        }
    return ods;
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::Reset
// ----------------------------------------------------------------------------
//
TInt CRsfwDavTransaction::Reset()
    {
    DEBUGSTRING(("Reset data suplier"));
    switch (iWebDavOp)
        {
    case EWebDavOpPut:
        iSendDataCount = 0;
        break;
        
    default:
        break;
        }
    return KErrNone;
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::TransactionComplete
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::TransactionCompleteL()
    {
    iStatus = KErrNone;
    if (iWebDavOp == EWebDavOpOptions)
        {
        iWebDavSession->SetConnected(ETrue);
        }
    iWebDavOp = EWebDavOpNone;
    iWebDavSession->WebDavTransactionCompleteL(this);
    // Must not do anything with this transaction object
    // after calling the completion callback because this will be destroyed
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::TransactionError
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::TransactionError()
    {
    Cleanup();
    iWebDavOp = EWebDavOpNone;
    iWebDavSession->WebDavTransactionError(this);
    // Must not do anything with this transaction object
    // after calling the error callback because this will be destroyed
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::Cleanup
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::Cleanup()
    { 
    // Cleanup (e.g., after a failed transaction).
    // Only release resources that may block further transactions
    // before this transaction has been destroyed.
    switch (iWebDavOp)
        {
    case EWebDavOpGet:
    case EWebDavOpPut:
        iBodyFile.Close();
        break;
        
    default:
        break;
        }
    }
    
// ----------------------------------------------------------------------------
// CRsfwDavTransaction::PropFindResponseBeginL
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::PropFindResponseBeginL(TInt aDepth) 
    {
    iWebDavSession->SetPropFindParametersL(iDirEnts,
                                          *iPropFindPath,
                                          aDepth);
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::LockQueryResponseBeginL
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::LockQueryResponseBegin() 
    {
    iWebDavSession->SetLockQueryParameters(iDavFileInfo);
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::PropFindResponseEndL
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::PropFindResponseEndL()
    {
    iWebDavSession->PropFindResponseEndL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::LockResponseEndL
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::LockResponseEndL()
    {
    iWebDavSession->LockResponseEndL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::ParsePropFindResponseL
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::ParsePropFindResponseL(const TDesC8& aResponse)
    { 
    iWebDavSession->ParsePropFindResponseL(aResponse);
    }

// ----------------------------------------------------------------------------
// CRsfwDavTransaction::ParseLockResponseL
// ----------------------------------------------------------------------------
//
void CRsfwDavTransaction::ParseLockResponseL(const TDesC8& aResponse)
    { 
    iWebDavSession->ParseLockResponseL(aResponse);
    }

//  End of File
