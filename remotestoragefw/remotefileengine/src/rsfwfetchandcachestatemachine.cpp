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
* Description:  State machine for fetching and caching files and directories
*
*/


#include "rsfwfetchandcachestatemachine.h"
#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwinterface.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwrfeserver.h"
#include "mdebug.h"
#include "rsfwfileengine.h"
#include "rsfwdirent.h"

_LIT8(KMimeTypeJpeg, "image/jpeg");
_LIT8(KMimeTypeMpeg, "audio/mpeg");


// ----------------------------------------------------------------------------
// CRsfwFetchAndCacheStateMachine::CRsfwFetchAndCacheStateMachine
// ----------------------------------------------------------------------------
//
CRsfwFetchAndCacheStateMachine::CRsfwFetchAndCacheStateMachine()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwFetchAndCacheStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState*
CRsfwFetchAndCacheStateMachine::CompleteRequestL(TInt aError) 
    {    
    if (aError == KUpdateNotRequired) 
        {   // discard 
        aError = KErrNone;
        }
    
    iDirEnts.ResetAndDestroy();
    // last byte that was actually fetched, may be more than was requested
    TRfeFetchOutArgs* outArgs = static_cast<TRfeFetchOutArgs*>(iOutArgs);
    outArgs->iLastByte = iLastByte;
    CompleteAndDestroyState()->SetErrorCode(aError);  
    
    // remove the fetching directory wait note
    if (Node()->Type() == KNodeTypeDir) 
        {
        DeleteWaitNoteL(ETrue);   
        }

    return CompleteAndDestroyState();
    } 

// ----------------------------------------------------------------------------
// CRsfwFetchAndCacheStateMachine::TFetchDataState::TFetchDataState
// ----------------------------------------------------------------------------
//
CRsfwFetchAndCacheStateMachine::TFetchDataState::TFetchDataState(
    CRsfwFetchAndCacheStateMachine* aParent)
    : iOperation(aParent)
    {
    
    }

// ----------------------------------------------------------------------------
// CRsfwFetchAndCacheStateMachine::TFetchDataState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwFetchAndCacheStateMachine::TFetchDataState::EnterL() 
    {
    TRfeFetchInArgs* inArgs =
        static_cast<TRfeFetchInArgs*>(iOperation->iInArgs);
    iOperation->iFirstByte = inArgs->iFirstByte;
    iOperation->iLastByte = inArgs->iLastByte;
    
    TInt recognizerLimit;
    TInt metadataLimit = 0;
    TCachingMode cachingMode;

    if (!(iOperation->Node()))
        {
        User::Leave(KErrNotFound);
        }

    // the cache file should be continuos
    // i.e. we always add to the end of the cache
    __ASSERT_DEBUG(iOperation->iFirstByte <= iOperation->Node()->iCachedSize, 
                   User::Panic(KRfeServer, ECacheInconsistency));

    cachingMode =
        iOperation->Node()->iFileTable->Volume()->iVolumeTable->iCachingMode;
    recognizerLimit =
        iOperation->
        Node()->iFileTable->Volume()->iVolumeTable->iRecognizerLimit;
   
        
    // for files, adjust lastByte based on the caching mode...
    if (iOperation->Node()->Type() == KNodeTypeFile) 
        {
        switch (cachingMode)
            {
        case EWholeFileCaching:
            if (iOperation->iLastByte < recognizerLimit) 
                {
                // iLastByte = 127
                iOperation->iLastByte = recognizerLimit-1;
                } 
            else 
                {
                // fetch the whole file
                iOperation->iLastByte = iOperation->Node()->Size() -1;
                }
        
            break;
        case EFullIfa:
            if (iOperation->iLastByte < recognizerLimit) 
                {
                // iLastByte = 127
                iOperation->iLastByte = recognizerLimit-1;
                } 
            // othewise no change
            break;
        case EMetadataIfa:
            // set metadataLimit based on the MIME-type
            if (iOperation->Node()->MimeType())
                {
                if ((*iOperation->Node()->MimeType()).Compare(
                        KMimeTypeJpeg) == 0)
                    {
                    metadataLimit =
                        iOperation->Node()->iFileTable->
                        Volume()->iVolumeTable->iImageJpegLimit;
                    }

                if ((*iOperation->Node()->MimeType()).Compare(
                        KMimeTypeMpeg) == 0)
                    {
                    metadataLimit =
                        iOperation->Node()->iFileTable->
                        Volume()->iVolumeTable->iAudioMpegLimit;
                    }
                
                // set the lastbyte
                if (iOperation->iLastByte < recognizerLimit) 
                    {
                    // iLastByte = 127
                    iOperation->iLastByte = recognizerLimit-1;
                    }
                
                else if (iOperation->iLastByte < metadataLimit)
                    {
                    // Fetch "enough" metadata to avoid
                    // unnecessary many round-trips...
                    iOperation->iLastByte = metadataLimit - 1;
                    }
                else if (iOperation->iLastByte >= metadataLimit)
                    {
                    iOperation->iLastByte = iOperation->Node()->Size() - 1;
                    }
                }
            else 
            	{
            	// MIME-type not recognized
            	if (iOperation->iLastByte < recognizerLimit) 
                	{
                	// iLastByte = 127
                	iOperation->iLastByte = recognizerLimit-1;
                	} 
            	else 
                	{
                	// fetch the whole file
                	iOperation->iLastByte = iOperation->Node()->Size() -1;
                	}
            	}
            }
        }
        
    // Now we know what actually will be fetched, write to debug...
    // and put up wait notes. 
    if (iOperation->Node()->Type() == KNodeTypeFile) 
        {
        
        DEBUGSTRING(("FETCH for a file with fid %d, bytes %d - %d",
                     iOperation->Node()->Fid().iNodeId,
                     iOperation->iFirstByte,
                     iOperation->iLastByte));
                             
        DEBUGSTRING16(("name is '%S",
                        iOperation->Node()->Name()));               
                     
        DEBUGSTRING(("full size is %d, cached size is %d",
                     iOperation->Node()->Size(),
                     iOperation->Node()->iCachedSize));   
        
      
        }
    else if (iOperation->Node()->Type() == KNodeTypeDir) 
        {
        
        DEBUGSTRING(("FETCH for a directory with fid %d, bytes %d - %d",
                     iOperation->Node()->Fid().iNodeId,
                     iOperation->iFirstByte,
                     iOperation->iLastByte));
                     
        DEBUGSTRING16(("name is '%S",
                        iOperation->Node()->Name()));             
        DEBUGSTRING(("full size is %d, cached size is %d",
                     iOperation->Node()->Size(),
                     iOperation->Node()->iCachedSize));   
                     
        }

    // whether cached data is used...
    // for files:
    if (((iOperation->Node()->Type() == KNodeTypeFile) &&
         (iOperation->FileEngine()->UseCachedData(*iOperation->Node())) &&
         ((iOperation->iLastByte <= iOperation->Node()->iCachedSize) ||
          iOperation->Node()->IsFullyCached())) ||
          
        // for directories:
        ((iOperation->Node()->Type() == KNodeTypeDir) &&
         (iOperation->FileEngine()->UseCachedAttributes(*iOperation->Node())) &&
         (iOperation->FileEngine()->UseCachedData(*iOperation->Node()))))
        {
        DEBUGSTRING(("using cached data"));

        if (iOperation->Node()->IsLocallyDirty())
            {
            DEBUGSTRING16(("directory is locally dirty"));
                            
            // This is a directory which has at least one kid
            // that has been cached or flushed since the last opening
            // of the directory.
            iOperation->FileEngine()->UpdateDirectoryContainerL(
                *iOperation->Node());
            }
        // if the directory appeared to be childless add it to metadata LRU list
        if ( iOperation->Node()->Type() == KNodeTypeDir && 
             iOperation->Node()->Kids()->Count() == 0 )
            {
            iOperation->Volumes()->AddToMetadataLRUPriorityListL(iOperation->Node(), ECachePriorityNormal);
            }
            
        iOperation->iLastByte = iOperation->Node()->Size();
        iOperation->HandleRemoteAccessResponse(0, KUpdateNotRequired);
        }
    else
        {
        DEBUGSTRING(("fetching data from server"));
        // put up a wait note if getting a directory
        // (for files no global wait notes, as that would take a too long time)
         if (iOperation->Node()->Type() == KNodeTypeDir) 
        	{
        	// directory - pu up a 'Retrieving...' global wait note
        	iOperation->ShowWaitNoteL( ERemoteOpDirDownloading ); 
        	}

            
        if (iOperation->iLastByte > iOperation->Node()->Size())
            {   // Don't try to read beyond the end of the file...
            // Don't try to read beyond the end of the file...
            iOperation->iLastByte = iOperation->Node()->Size();    
            }  
            
        if (iOperation->iLastByte == 0)
            {
            iOperation->iLength = 0;
            // aLastByte == 0 indicates "no partial caching..."
            // i.e. range 0 - 0
            TUint transactionId = 
                iOperation->FileEngine()->
                FetchAndCacheL(*iOperation->Node(),
                               0 ,
                               &(iOperation->iLength),
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
            iOperation->iLength =
                iOperation->iLastByte - iOperation->Node()->iCachedSize + 1;
            // Continue filling the cache-file sequentially
            TUint transactionId = 
                iOperation->FileEngine()->
                FetchAndCacheL(*iOperation->Node(),
                               iOperation->Node()->iCachedSize,
                               &(iOperation->iLength),
                               &(iOperation->iDirEnts),
                               iOperation);
            // transactionId = 0 means syncronous non-cancellable operation    
    		if (transactionId > 0) 
   				{
    			iOperation->iTransactionId = transactionId;
    			}
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFetchAndCacheStateMachine::TFetchDataState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwFetchAndCacheStateMachine::TState*
CRsfwFetchAndCacheStateMachine::TFetchDataState::CompleteL()
    {   
    iOperation->iLastByte = iOperation->FileEngine()->AddToCacheL(
        *iOperation->Node(), 
        &iOperation->iDirEnts, 
        iOperation->FileEngine(),
        iOperation->Node()->iCachedSize +
        iOperation->iLength); 
    
    return iOperation->CompleteRequestL(KErrNone);
    }
  
// ----------------------------------------------------------------------------
// CRsfwFetchAndCacheStateMachine::TFetchDataState::ErrorL
// ----------------------------------------------------------------------------
//    
CRsfwFetchAndCacheStateMachine::TState*
CRsfwFetchAndCacheStateMachine::TFetchDataState::ErrorL(TInt aCode)
    {   
    // *********** from CRsfwFileEngine::GetDirectoryL()
    if (iOperation->Node()->Type() == KNodeTypeDir)
        {
        TInt err = aCode;
        if (aCode == KUpdateNotRequired)
            {
            err = KErrNone;
            }
        return iOperation->CompleteRequestL(err);
        }
        
    // file
    return iOperation->CompleteRequestL(aCode);   
    }

// ----------------------------------------------------------------------------
// CRsfwWaitNoteStateMachine::ErrorOnStateExit
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState* CRsfwFetchAndCacheStateMachine::ErrorOnStateExit(TInt aError)
    {
    iDirEnts.ResetAndDestroy();
        // remove the fetching directory wait note
    if (Node()->Type() == KNodeTypeDir) 
        {
        TRAP_IGNORE(DeleteWaitNoteL(ETrue));   
        }

        
    return CRsfwRfeStateMachine::ErrorOnStateExit(aError);
    }    
