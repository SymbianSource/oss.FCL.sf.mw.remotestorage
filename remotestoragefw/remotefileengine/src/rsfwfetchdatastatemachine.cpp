/*
* Copyright (c) 2005-2006 Nokia Corporation and/or its subsidiary(-ies).
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


#include "rsfwfetchdatastatemachine.h"
#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwinterface.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwrfeserver.h"
#include "mdebug.h"
#include "rsfwfileengine.h"


// ----------------------------------------------------------------------------
// CRsfwFetchDataStateMachine::CRsfwFetchDataStateMachine
// ----------------------------------------------------------------------------
//
CRsfwFetchDataStateMachine::CRsfwFetchDataStateMachine()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwFetchDataStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState*
CRsfwFetchDataStateMachine::CompleteRequestL(TInt aError) 
    {
    TRfeFetchDataOutArgs* outArgs =
        static_cast<TRfeFetchDataOutArgs*>(iOutArgs);
    if(!aError) 
        {
        outArgs->iTempPath.Copy(*iCacheName);
        }
        
    CompleteAndDestroyState()->SetErrorCode(aError);
    return CompleteAndDestroyState();   
    } 

// ----------------------------------------------------------------------------
// CRsfwFetchDataStateMachine::TFetchDataState::TFetchDataState
// ----------------------------------------------------------------------------
//
CRsfwFetchDataStateMachine::TFetchDataState::TFetchDataState(
    CRsfwFetchDataStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwFetchDataStateMachine::TFetchDataState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwFetchDataStateMachine::TFetchDataState::EnterL() 
    {
    TInt err = KErrNone;
    TRfeFetchDataInArgs* inArgs =
        static_cast<TRfeFetchDataInArgs*>(iOperation->iInArgs);
    TRfeFetchDataOutArgs* outArgs =
        static_cast<TRfeFetchDataOutArgs*>(iOperation->iOutArgs);
    TInt firstByte = inArgs->iFirstByte;
    TInt lastByte = inArgs->iLastByte;

  
    TCachingMode cachingMode =
        iOperation->Node()->iFileTable->Volume()->iVolumeTable->iCachingMode;
    
    if (iOperation->Node())
        {
        DEBUGSTRING(("Fetch without caching fid %d, bytes %d - %d",
                     iOperation->Node()->Fid().iNodeId,
                     firstByte,
                     lastByte));
        
        if (cachingMode == EWholeFileCaching) 
            {
            outArgs->iUseTempPath = EFalse; 
            // in this mode we always fetch the whole file to the normal cache
            iOperation->iCacheName = iOperation->Node()->CacheFileName();
            iOperation->iLength = iOperation->Node()->Size() -
                iOperation->Node()->iCachedSize +
                1;
            TUint transactionId = iOperation->
                FileEngine()->FetchAndCacheL(*iOperation->Node(), 
                                             iOperation->Node()->iCachedSize, 
                                             &iOperation->iLength,
                                             &(iOperation->iDirEnts),
                                             iOperation); 
            // transactionId = 0 means syncronous non-cancellable operation    
    		if (transactionId > 0) 
   				{
    			iOperation->iTransactionId = transactionId;
    			}      
            }
        else 
            {
            // reset and use the temporary cache file...
            outArgs->iUseTempPath = ETrue;
            TParse parser;
            parser.Set(*iOperation->Node()->CacheFileName(), NULL, NULL);
            HBufC* tempPath = HBufC::NewLC(KMaxPath);
            TPtr tempfile = tempPath->Des();
            tempfile.Append(parser.DriveAndPath());
            tempfile.Append(KTempFileName);
            iOperation->iCacheName = iOperation->Node()->CacheFileName();
            // This much will be added to the cache by this fetch
            iOperation->iLength =lastByte - firstByte + 1;
            if (!iOperation->
                Node()->
                iFileTable->
                Volume()->
                iVolumeTable->EnsureCacheCanBeAddedL(iOperation->iLength))
                {
                User::Leave(KErrDiskFull);  
                }
            RFile f;
            err = f.Replace(CRsfwRfeServer::Env()->iFs,
                            *tempPath,
                            EFileShareAny | EFileWrite);
            if (err == KErrNone)
                {
                f.Close();
                HBufC* fullName =
                    iOperation->FileEngine()->FullNameLC(*iOperation->Node());
                TUint transactionId = iOperation->FileEngine()->RemoteAccessL()->
                    GetFileL(*fullName,
                             *tempPath,
                             firstByte,
                             &iOperation->iLength,
                             KRemoteAccessOptionGetToStartOfFile,
                             iOperation);  
                // transactionId = 0 means syncronous non-cancellable operation     
    			if (transactionId > 0) 
   					{
    				iOperation->iTransactionId = transactionId;
    				}              
                CleanupStack::PopAndDestroy(fullName);
                }
            CleanupStack::PopAndDestroy(tempPath);   
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFetchDataStateMachine::TFetchDataState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwFetchDataStateMachine::TState*
CRsfwFetchDataStateMachine::TFetchDataState::CompleteL()
    {      
    return iOperation->CompleteRequestL(KErrNone); 
    }

// ----------------------------------------------------------------------------
// CRsfwFetchDataStateMachine::TFetchDataState::ErrorL
// ----------------------------------------------------------------------------
//    
CRsfwFetchDataStateMachine::TState*
CRsfwFetchDataStateMachine::TFetchDataState::ErrorL(TInt aCode)
    {
    return iOperation->CompleteRequestL(aCode); 
    }


