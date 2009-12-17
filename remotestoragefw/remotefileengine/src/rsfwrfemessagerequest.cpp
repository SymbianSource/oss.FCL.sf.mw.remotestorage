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
* Description:  A request sent by a client (i.e. non-internal request)
*
*/


#include "rsfwrfemessagerequest.h"
#include "rsfwrfeoperation.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwcommon.h"
#include "rsfwinterface.h"
#include "mdebug.h"
#include "rsfwrfesession.h"



// ----------------------------------------------------------------------------
// CRsfwRfeMessageRequest::CRsfwRfeMessageRequest
// ----------------------------------------------------------------------------
//     
CRsfwRfeMessageRequest::CRsfwRfeMessageRequest() 
    {
    SetRequestType(EMessageRequest);
    iMessageCompleted = EFalse;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeMessageRequest::SetL
// ----------------------------------------------------------------------------
//     
void CRsfwRfeMessageRequest::SetL(const RMessage2& aMessage,
                             CRsfwRfeSession* aSession)
    {    
    DEBUGSTRING(("CRsfwRfeMessageRequest::Set"));
    
    // common for all operations
    iMessage = aMessage;
    iSession = aSession;
    iVolumeTable = aSession->Volume(); 
    
    // if a file system access operation
    if (aMessage.Function() >=  ERenameReplace) 
        {
        switch (aMessage.Function())
            {
            // read input parameters
        case ERenameReplace:
            {
            TRfeRenameInArgs* inArgs = new (ELeave) TRfeRenameInArgs();
            CleanupStack::PushL(inArgs);
            TPckg<TRfeRenameInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(inArgs);
            iInArgs = inArgs;
            iOutArgs = NULL;
            break;
            }
        case ESetAttr:
            {
            TRfeSetAttrInArgs* inArgs = new (ELeave) TRfeSetAttrInArgs();
            CleanupStack::PushL(inArgs);
            TPckg<TRfeSetAttrInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(inArgs);
            iInArgs = inArgs;
            iOutArgs = NULL;
            break;
            }
        case EFsIoctl:
            {
            TRfeIoctlInArgs* inArgs = new (ELeave) TRfeIoctlInArgs();
            CleanupStack::PushL(inArgs);
            TPckg<TRfeIoctlInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(inArgs);
            iInArgs = inArgs;
            iOutArgs = NULL;
            break;
            }
        
        case EGetAttr:
            {
            TRfeGetAttrInArgs* inArgs = new (ELeave) TRfeGetAttrInArgs();
            CleanupStack::PushL(inArgs);
            TRfeGetAttrOutArgs* outArgs = new (ELeave )TRfeGetAttrOutArgs();
            CleanupStack::PushL(outArgs);
            TPckg<TRfeGetAttrInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(2, inArgs); // inArgs, outArgs
            iInArgs = inArgs;
            iOutArgs = outArgs;
            break;
            }

        case EOpenByPath:
            {
            TRfeOpenByPathInArgs* inArgs = new (ELeave) TRfeOpenByPathInArgs();
            CleanupStack::PushL(inArgs);
            TRfeOpenByPathOutArgs* outArgs = new (ELeave) TRfeOpenByPathOutArgs();
            CleanupStack::PushL(outArgs);
            TPckg<TRfeOpenByPathInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(2, inArgs); // inArgs, outArgs
            iInArgs = inArgs;
            iOutArgs = outArgs;
            break;      
            }   

        case EFsRoot:
            {
            TRfeRootInArgs* inArgs = new (ELeave) TRfeRootInArgs();
            CleanupStack::PushL(inArgs);
            TRfeRootOutArgs* outArgs = new (ELeave) TRfeRootOutArgs();
            CleanupStack::PushL(outArgs);
            TPckg<TRfeRootInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(2, inArgs); // inArgs, outArgs
            iInArgs = inArgs;
            iOutArgs = outArgs;
            break;      
            }   

        case EMkDir:
            {
            TRfeMkdirInArgs* inArgs = new (ELeave) TRfeMkdirInArgs();
            CleanupStack::PushL(inArgs);
            TPckg<TRfeMkdirInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(inArgs);
            iInArgs = inArgs;
            iOutArgs = NULL;
            break;
            }

        case ERemoveDir:
            {
            TRfeRmdirInArgs* inArgs = new (ELeave) TRfeRmdirInArgs();
            CleanupStack::PushL(inArgs);
            TPckg<TRfeRmdirInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(inArgs);
            iInArgs = inArgs;
            iOutArgs = NULL;
            break;
            }

        case ECreateFile:
            {
            TRfeCreateInArgs* inArgs = new (ELeave) TRfeCreateInArgs();
            CleanupStack::PushL(inArgs);
            TRfeCreateOutArgs* outArgs = new (ELeave) TRfeCreateOutArgs();
            CleanupStack::PushL(outArgs);
            TPckg<TRfeCreateInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(2, inArgs); // inArgs, outArgs
            iInArgs = inArgs;
            iOutArgs = outArgs;
            break;      
            }   

        case ERemove:
            {
            TRfeRemoveInArgs* inArgs = new (ELeave) TRfeRemoveInArgs();
            CleanupStack::PushL(inArgs);
            TPckg<TRfeRemoveInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(inArgs);
            iInArgs = inArgs;
            iOutArgs = NULL;
            break;
            }

        case ELookUp:
            {
            TRfeLookupInArgs* inArgs = new (ELeave) TRfeLookupInArgs();
            CleanupStack::PushL(inArgs);
            TRfeLookupOutArgs* outArgs = new (ELeave) TRfeLookupOutArgs();
            CleanupStack::PushL(outArgs);
            TPckg<TRfeLookupInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(2, inArgs);
            iInArgs = inArgs;
            iOutArgs = outArgs;
            break;      
            }   

        case EClose:
            {
            TRfeCloseInArgs* inArgs = new (ELeave) TRfeCloseInArgs();
            inArgs->iFid.iVolumeId = aMessage.Int0();
            inArgs->iFid.iNodeId = aMessage.Int1();
            inArgs->iOpCode = EClose;
            inArgs->iFlags = aMessage.Int2();
            iInArgs = inArgs;
            iOutArgs = NULL;
            break;
            }

       case EFlush:
            {
            TRfeFlushInArgs* inArgs = new (ELeave) TRfeFlushInArgs();
            CleanupStack::PushL(inArgs);
            TPckg<TRfeFlushInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(inArgs);
            iInArgs = inArgs;
            break;
            }

        case EFetch:
            {
            TRfeFetchInArgs* inArgs = new (ELeave) TRfeFetchInArgs();
            CleanupStack::PushL(inArgs);
            TRfeFetchOutArgs* outArgs = new (ELeave) TRfeFetchOutArgs();
            CleanupStack::PushL(outArgs);
            TPckg<TRfeFetchInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(2, inArgs);
            iInArgs = inArgs;
            iOutArgs = outArgs;
            break;  
            }

        case EFetchData:
            {
            TRfeFetchDataInArgs* inArgs = new (ELeave) TRfeFetchDataInArgs();
            CleanupStack::PushL(inArgs);
            TRfeFetchDataOutArgs* outArgs = new (ELeave) TRfeFetchDataOutArgs();
            CleanupStack::PushL(outArgs);
            TPckg<TRfeFetchDataInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(2, inArgs);
            iInArgs = inArgs;
            iOutArgs = outArgs;
            break;
            }

        case EOkToWrite:
            {
            TRfeWriteDataInArgs* inArgs = new (ELeave) TRfeWriteDataInArgs();
            CleanupStack::PushL(inArgs);
            TRfeWriteDataOutArgs* outArgs = new (ELeave) TRfeWriteDataOutArgs();
            CleanupStack::PushL(outArgs);
            TPckg<TRfeWriteDataInArgs> pkgInArgs(*inArgs);
            aMessage.ReadL(0, pkgInArgs);
            CleanupStack::Pop(2, inArgs);
            iInArgs = inArgs;
            iOutArgs = outArgs;
            break;
            }   
            }
    
        DEBUGSTRING(("volume id %d", iInArgs->iFid.iVolumeId));
        // Here volumeId may be 0 to denote "no volume id"
        // This is OK - we assume that drive letter A is never allocated
        iVolume = iVolumeTable->VolumeByVolumeId(iInArgs->iFid.iVolumeId);
        
        // As this is a file system access operation - not mount operation,
        // if volume is null we are not ready for file operations
        // - indicate this to the client who needs to call mount
        TBool ready =
            iVolume &&
            (iVolume->iMountInfo.iMountStatus.iMountState !=
             KMountStateDormant);
        if (!ready && aMessage.Function() != EFsRoot && aMessage.Function() != EClose) 
            {
            aMessage.Complete(KErrNotReady);
            iMessageCompleted = ETrue;
            }
        }
    }
    
// ----------------------------------------------------------------------------
// CRsfwRfeMessageRequest::Message
// ----------------------------------------------------------------------------
//     
const RMessage2& CRsfwRfeMessageRequest::Message()
    {
    return iMessage;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeMessageRequest::Session
// ----------------------------------------------------------------------------
//         
CRsfwRfeSession* CRsfwRfeMessageRequest::Session()
    {
    return iSession;
    }


// ----------------------------------------------------------------------------
// CRsfwRfeMessageRequest::Session
// ----------------------------------------------------------------------------
//         
void CRsfwRfeMessageRequest::SetSession(CRsfwRfeSession* aSession)
    {
    iSession = aSession;
    }


// ----------------------------------------------------------------------------
// CRsfwRfeMessageRequest::Complete
// ----------------------------------------------------------------------------
//         
void CRsfwRfeMessageRequest::Complete(TInt aError)
    {
   	DEBUGSTRING(("CRsfwRfeMessageRequest::Complete"));
    if (iSession) 
        {
        DEBUGSTRING((">>> Dispatch exit (operation %d, error %d, return %d)",
                     Operation()->iFunction,
                     aError,
                     aError));
        DEBUGSTRING(("completing request"));  
        if (!Message().IsNull()) 
            {
            Message().Complete(aError);
            }
        DEBUGSTRING(("request completed")); 
        iMessageCompleted = ETrue;  
        }
    }   

// ----------------------------------------------------------------------------
// CRsfwRfeMessageRequest::CompleteAndDestroy
// ----------------------------------------------------------------------------
//     
void CRsfwRfeMessageRequest::CompleteAndDestroy(TInt aError)
    {
  	DEBUGSTRING(("CRsfwRfeMessageRequest::CompleteAndDestroy"));
  	
  	TInt err;
    err = KErrNone;
    if (iSession) 
        {
        DEBUGSTRING((">>> Dispatch exit (operation %d, error %d, return %d)",
                     Operation()->iFunction, aError, aError));
        switch (Operation()->iFunction)
            {
            // write output parameters
        case EFetch:
            {
            DEBUGSTRING(("writing parameters: EFetch"));	
            TRfeFetchOutArgs* outArgs = (TRfeFetchOutArgs *)(iOutArgs);
            TPckg<TRfeFetchOutArgs> pkgOutArgs(*outArgs);
            err = Message().Write(1, pkgOutArgs);
            break;                  
            }   

        case EFetchData:
            {
           DEBUGSTRING(("writing parameters: EFetchData"));	
            TRfeFetchDataOutArgs* outArgs = (TRfeFetchDataOutArgs *)(iOutArgs);
            TPckg<TRfeFetchDataOutArgs> pkgOutArgs(*outArgs);
            err = Message().Write(1, pkgOutArgs);
            break;                  
            }   

        case EOkToWrite:
            {
           DEBUGSTRING(("writing parameters: EOkToWrite"));	 	
            TRfeWriteDataOutArgs* outArgs = (TRfeWriteDataOutArgs *)(iOutArgs);
            TPckg<TRfeWriteDataOutArgs> pkgOutArgs(*outArgs);
            err = Message().Write(1, pkgOutArgs);
            break;                  
            }           

        case ELookUp:
            {
            DEBUGSTRING(("writing parameters: ELookup"));		
            TRfeLookupOutArgs* outArgs = (TRfeLookupOutArgs *)(iOutArgs);
            TPckg<TRfeLookupOutArgs> pkgOutArgs(*outArgs);
            err = Message().Write(1, pkgOutArgs);
            break;                  
            }   

        case ECreateFile:
            {
           DEBUGSTRING(("writing parameters: ECreateFile"));		 	
            TRfeCreateOutArgs* outArgs = (TRfeCreateOutArgs *)(iOutArgs);
            TPckg<TRfeCreateOutArgs> pkgOutArgs(*outArgs);
            err = Message().Write(1, pkgOutArgs);
            break;  
            }           

        case EOpenByPath:
            {
           DEBUGSTRING(("writing parameters: EOpenByPath"));		 	
            TRfeOpenByPathOutArgs* outArgs =
                (TRfeOpenByPathOutArgs *)(iOutArgs);
            TPckg<TRfeOpenByPathOutArgs> pkgOutArgs(*outArgs);
            err = Message().Write(1, pkgOutArgs);
            break;  
            }
            
        case EGetAttr:
            {
            DEBUGSTRING(("writing parameters: EGetAttr"));		
            TRfeGetAttrOutArgs* outArgs = (TRfeGetAttrOutArgs *)(iOutArgs);
            TPckg<TRfeGetAttrOutArgs> pkgOutArgs(*outArgs);
            err = Message().Write(1, pkgOutArgs);
            break;      
            }

        case EFsRoot:
            {
            DEBUGSTRING(("writing parameters: EFsRoot"));			
            TRfeRootOutArgs* outArgs = (TRfeRootOutArgs *)(iOutArgs);
            TPckg<TRfeRootOutArgs> pkgOutArgs(*outArgs);
            err = Message().Write(1, pkgOutArgs);
            break;      
            }
          }
          
  		if (err)
  		    {
  		    aError = err;
  			}
  
        DEBUGSTRING(("completing request"));
        if (!Message().IsNull()) 
            {
            Message().Complete(aError);
            }
        DEBUGSTRING(("request completed")); 
        iMessageCompleted = ETrue;  
        }
  
    Destroy();
    }
    
    
 // ----------------------------------------------------------------------------
// CRsfwRfeRequest::Destroy
// ----------------------------------------------------------------------------
// 
void CRsfwRfeMessageRequest::Destroy()
    {
    if (iSession) 
        {
        iSession->RemoveFromMessageRequestArray(this);
        }
        
    CRsfwRfeRequest::Destroy();
    }
       

