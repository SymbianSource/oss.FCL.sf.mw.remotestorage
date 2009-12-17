/*
* Copyright (c) 2007 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  State machine for fetching data without caching it permanently
*
*/


#include "rsfwflushstatemachine.h"
#include "rsfwinterface.h"
#include "rsfwfileentry.h"
#include "rsfwfileengine.h"
#include "rsfwrfeserver.h"
#include "rsfwvolumetable.h"
#include "rsfwfiletable.h"
#include "rsfwwaitnotemanager.h"
#include "rsfwvolume.h"
#include "mdebug.h"


// ----------------------------------------------------------------------------
// CRsfwFlushStateMachine::CRsfwFlushStateMachine
// ----------------------------------------------------------------------------
//
CRsfwFlushStateMachine::CRsfwFlushStateMachine()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwFlushStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState*
CRsfwFlushStateMachine::CompleteRequestL(TInt aError) 
    {
     DEBUGSTRING(("CRsfwFlushStateMachine::CompleteRequestL()"));
    // If we just wrote the file to the server set attributes from the cache
    // file's attributes.Even if writing the file failed, attributes should
    // reflect the local modifications
    if (Node()->CacheFileName())
        {
        FileEngine()->SetupAttributes(*Node());
        }
        
    CompleteAndDestroyState()->SetErrorCode(aError);
    return CompleteAndDestroyState();   
    } 

// ----------------------------------------------------------------------------
// CRsfwFlushStateMachine::TFlushDataToServerState::TFlushDataToServerState
// ----------------------------------------------------------------------------
//
CRsfwFlushStateMachine::TFlushDataToServerState::TFlushDataToServerState(
    CRsfwFlushStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwFlushStateMachine::TFlushDataToServerState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwFlushStateMachine::TFlushDataToServerState::EnterL() 
    {
    DEBUGSTRING(("CRsfwFlushStateMachine::TFlushDataToServerState::EnterL()"));
    
    TDesC* cacheNamep;
    if (!iOperation->Node())
        {
        User::Leave(KErrNotFound);
        }

    TRfeFlushInArgs* inArgs =
        static_cast<TRfeFlushInArgs*>(iOperation->iInArgs);
    
    
    if (iOperation->Node()->IsCancelled()) 
        {
        // user has cancelled writing this file to server even before we got to flush
        // (when the file was being written to the local cache)
        iOperation->HandleRemoteAccessResponse(0, KErrCancel);
        }
    else 
        {
        TInt firstByte = inArgs->iFirstByte;
        TInt dataLength = inArgs->iDataLength;
        TInt totalSize = inArgs->iTotalSize;  
     
        cacheNamep = iOperation->Node()->CacheFileName();
    
        _LIT8(KTextPlain, "text/plain");
        HBufC* fullName =
            iOperation->FileEngine()->FullNameLC(*(iOperation->Node()));
            
    
        // get the MIME-type of the file
        HBufC8* contentType = iOperation->FileEngine()->GetContentType(*cacheNamep);
    
        if (contentType) 
            {
            CleanupStack::PushL(contentType);  
            }
        else 
            {
            contentType = KTextPlain().AllocLC();
            }
        
        if  ((firstByte == 0) &&
            (dataLength == totalSize)) 
            {
            // non-partial put
            TUint transactionId 
                = iOperation->FileEngine()->RemoteAccessL()->PutFileL(*cacheNamep,
                                                            *fullName,
                                                            *contentType,
                                                            iOperation);
        
            }
        else 
            {
            // partial put
            TUint transactionId 
            = iOperation->FileEngine()->RemoteAccessL()->PutFileL(*cacheNamep,
                                                            *fullName,
                                                            *contentType,
                                                            firstByte,
                                                   dataLength-firstByte,
                                                            totalSize,
                                                            iOperation);
            }
 
        CleanupStack::PopAndDestroy(2); // fullName, contentType        
        }

    }

// ----------------------------------------------------------------------------
// CRsfwFlushStateMachine::TFlushDataToServerState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState*
CRsfwFlushStateMachine::TFlushDataToServerState::CompleteL()
    {  
    DEBUGSTRING(("CRsfwFlushStateMachine::TFlushDataToServerState::CompleteL()"));
  	return iOperation->CompleteRequestL(KErrNone); 
    }

// ----------------------------------------------------------------------------
// CRsfwFlushStateMachine::TFlushDataToServerState::ErrorL
// ----------------------------------------------------------------------------
//    
CRsfwRfeStateMachine::TState*
CRsfwFlushStateMachine::TFlushDataToServerState::ErrorL(TInt aCode)
    {
    DEBUGSTRING(("CRsfwFlushStateMachine::TFlushDataToServerState::ErrorL() %d", aCode));
  	return iOperation->CompleteRequestL(aCode); 
    }


// End of file

