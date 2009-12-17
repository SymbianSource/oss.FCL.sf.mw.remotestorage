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
* Description:  Encapsulated all sync operations
*
*/


#include "rsfwsyncoperations.h"
#include "rsfwrfemessagerequest.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwfileengine.h"
#include "rsfwfiletable.h"
#include "rsfwfileentry.h"
#include "rsfwrfesession.h"
#include "rsfwinterface.h"
#include "rsfwcontrol.h"
#include "mdebug.h"

   
// ----------------------------------------------------------------------------
// TRFeSynCRsfwRfeRequest::DoRequestL
// wrapper for all sync requests
// ----------------------------------------------------------------------------
// 
void TRFeSynCRsfwRfeRequest::DoRequestL(CRsfwRfeRequest* aRequest) 
    {
    CRsfwRfeMessageRequest* request = (CRsfwRfeMessageRequest*) aRequest;
    request->Session()->Volume()->DispatchL(request->iInArgs,
                                            request->iOutArgs);
    }

// ----------------------------------------------------------------------------
// TRFeDismountVolumeId::DoRequestL
// dismount a previously mounted volume by volume ID
// ----------------------------------------------------------------------------
//  
void TRFeDismountVolumeId::DoRequestL(CRsfwRfeRequest* aRequest)
    {
    DEBUGSTRING(("TRFeDismountVolumeId::DoRequestL"));
    CRsfwRfeMessageRequest* request = (CRsfwRfeMessageRequest*) aRequest;
    TInt volumeId = reinterpret_cast<TInt>(request->Message().Ptr0());
    DEBUGSTRING(("EDismount: volume %d", volumeId));
    request->Session()->Volume()->DismountByVolumeIdL(volumeId, ETrue);
    // This is called from the fileserver main thread.
    // Therefore, we should return quickly.
    }
    
// ----------------------------------------------------------------------------
// TRFeDismountByDriveLetter::DoRequestL
// dismount a previously mounted volume by drive letter
// ----------------------------------------------------------------------------
//   
void TRFeDismountByDriveLetter::DoRequestL(CRsfwRfeRequest* aRequest)
    {
    DEBUGSTRING(("TRFeDismountByDriveLetter::DoRequestL"));
    // Dismount a volume by drive letter
    // synchronous request
    CRsfwRfeMessageRequest* request = (CRsfwRfeMessageRequest*) aRequest;
    TChar driveLetter = reinterpret_cast<TInt>(request->Message().Ptr0());
    DEBUGSTRING(("EDismountByDriveLetter: '%c'", TUint(driveLetter)));
    request->Session()->Volume()->DismountByDriveLetterL(driveLetter, ETrue); 
    }


// ----------------------------------------------------------------------------
// TRFeGetMountList::DoRequestL
// get a list of currently active mounts
// ----------------------------------------------------------------------------
//      
void TRFeGetMountList::DoRequestL(CRsfwRfeRequest* aRequest)
    {
    DEBUGSTRING(("TRFeGetMountList::DoRequestL"));
    // synchronous request
    CRsfwRfeMessageRequest* request = (CRsfwRfeMessageRequest*) aRequest;
    TDriveList mountList;
    request->Session()->Volume()->GetMountList(mountList);
    TPckg<TDriveList> p(mountList);
    request->Message().WriteL(0, p);
    DEBUGSTRING8(("EGetMountList: '%S'", &mountList));
    }


// ----------------------------------------------------------------------------
// TRFeGetMountInfo::DoRequestL
// get information about a specific mount
// ----------------------------------------------------------------------------
//        
void TRFeGetMountInfo::DoRequestL(CRsfwRfeRequest* aRequest)
    {
    DEBUGSTRING(("TRFeGetMountInfo::DoRequestL"));
    // synchronous request
    CRsfwRfeMessageRequest* request = (CRsfwRfeMessageRequest*) aRequest;
    TRsfwMountInfo* mountInfo = new (ELeave) TRsfwMountInfo;
    CleanupStack::PushL(mountInfo);
    mountInfo->iMountConfig.iDriveLetter = 
        reinterpret_cast<TInt>(request->Message().Ptr0());
    TInt err = request->Session()->Volume()->GetMountInfo(*mountInfo);
    DEBUGSTRING(("EGetMountInfo for '%c' (err=%d)",
                 TUint(mountInfo->iMountConfig.iDriveLetter),
                 err));
    if (err != KErrNone)
        {
        mountInfo->iMountConfig.iUri.Zero();
        }
    TPckg<TRsfwMountInfo> mountInfoPackage(*mountInfo);
    request->Message().WriteL(1, mountInfoPackage);
    CleanupStack::PopAndDestroy(mountInfo);
    }  
    
    
// ----------------------------------------------------------------------------
// TRFeWriteData::DoRequestL
// get permission to write certain amount of data
// ----------------------------------------------------------------------------
//    
void TRFeWriteData::DoRequestL(CRsfwRfeRequest* aRequest) 
    {
    DEBUGSTRING(("TRFeWriteData::DoRequestL"));
    CRsfwRfeMessageRequest* request = (CRsfwRfeMessageRequest*) aRequest;
    TRfeWriteDataInArgs* inArgs =
        static_cast<TRfeWriteDataInArgs*>(request->iInArgs);
    TRfeWriteDataOutArgs* outArgs =
        static_cast<TRfeWriteDataOutArgs*>(request->iOutArgs);
    outArgs->iOkToWrite =
        request->iVolumeTable->EnsureCacheCanBeAddedL(inArgs->iBytes); 
        
    if (outArgs->iOkToWrite)  
        {
        // make sure "dirty bit" is on (file has uncommited modifications)
        CRsfwFileEntry* entry= 
            request->iVolume->iFileEngine->iFileTable->Lookup(inArgs->iFid);
        if (entry) 
            {
            entry->SetOpenedForWriting(ETrue);
            request->iVolume->iFileEngine->iFileTable->SaveMetaDataDelta();
            }
        // if writing has been cancelled, we have to inform the file server plugin
        if (entry->IsCancelled()) 
            {
            User::Leave(KErrCancel);
            }
        
        }
    }

void TRFeDirectoryRefresh::DoRequestL(CRsfwRfeRequest* aRequest) 
    {
    DEBUGSTRING(("TRFeDirectoryRefresh::DoRequestL"));
    CRsfwRfeMessageRequest* request = (CRsfwRfeMessageRequest*) aRequest;
    HBufC* refreshBuf = HBufC::NewLC(KMaxPath);
    TPtr refPtr = refreshBuf->Des();
    request->Message().ReadL(0, refPtr);
    User::LeaveIfError(request->Session()->Volume()->PurgeFromCache(refPtr));
    CleanupStack::PopAndDestroy(refreshBuf);
    }


void TRFeCancelAll::DoRequestL(CRsfwRfeRequest* aRequest)
    {
    DEBUGSTRING(("TRFeCancelAll::DoRequestL"));
    CRsfwRfeMessageRequest* request = (CRsfwRfeMessageRequest*) aRequest;
    HBufC* fileBuf = HBufC::NewLC(KMaxPath);
    TPtr filePtr = fileBuf->Des();
    request->Message().ReadL(0, filePtr);
    User::LeaveIfError(request->Session()->Volume()->CancelTransferL(filePtr));
    CleanupStack::PopAndDestroy(fileBuf);
    }

