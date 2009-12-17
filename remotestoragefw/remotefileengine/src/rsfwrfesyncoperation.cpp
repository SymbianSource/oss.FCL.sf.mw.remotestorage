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
* Description:  Encapsulates synchronous operation
*
*/


#include "rsfwrfesyncoperation.h"
#include "rsfwsyncoperations.h"
#include "rsfwvolume.h"
#include "rsfwcommon.h"
#include "rsfwrfeserver.h"
#include "mdebug.h"


// ----------------------------------------------------------------------------
// CRsfwRfeSyncOperation::DoRequestL
// ----------------------------------------------------------------------------
//  
void CRsfwRfeSyncOperation::DoRequestL(CRsfwRfeRequest* aRequest) 
    {
    __ASSERT_ALWAYS(iDoRequestL, User::Panic(KRfeServer, ENullRequestHandler));
    (*iDoRequestL)(aRequest);
    }

// ----------------------------------------------------------------------------
// CRsfwRfeSyncOperation::Set
// ----------------------------------------------------------------------------
//  
void CRsfwRfeSyncOperation::Set(CRsfwRfeRequest* aRequest, TInt aOpCode) 
    {
    if (aRequest->iVolume) 
        {
        DEBUGSTRING(("<<< Dispatch enter (volume=%d)",
                     aRequest->iVolume->iMountInfo.iMountStatus.iVolumeId)); 
        }
    else 
        {
        DEBUGSTRING(("<<< Dispatch enter"));  
        }
          
    switch (aOpCode)
        {
    case EDismountByVolumeId:
        DEBUGSTRING(("DISMOUNTBYVOLUMEID (operation %d)", aOpCode));
        iDoRequestL = &TRFeDismountVolumeId::DoRequestL;
        break;

    case EDismountByDriveLetter:
        DEBUGSTRING(("DISMOUNTBYDRIVELETTER (operation %d)", aOpCode));
        iDoRequestL = &TRFeDismountByDriveLetter::DoRequestL;
        break;

    case EGetMountList:
        DEBUGSTRING(("GETMOUNTLIST (operation %d)", aOpCode));
        iDoRequestL = &TRFeGetMountList::DoRequestL;
        break;

    case EGetMountInfo:
        DEBUGSTRING(("GETMOUNTINFO (operation %d)", aOpCode));
        iDoRequestL = &TRFeGetMountInfo::DoRequestL;
        break;

    case EFsRoot:
    case ESetAttr:
    case EFsIoctl:
        DEBUGSTRING(("ROOT (operation %d)", aOpCode));
        iDoRequestL = &TRFeSynCRsfwRfeRequest::DoRequestL;
        break;

    case EOkToWrite:
        DEBUGSTRING(("WRITEDATA (operation %d)", aOpCode));
        iDoRequestL = &TRFeWriteData::DoRequestL;
        break;
   case EDirRefresh:
        DEBUGSTRING(("REFRESHDIR (operation %d)", aOpCode));
        iDoRequestL = &TRFeDirectoryRefresh::DoRequestL;
        break;
   case ECancelAll:
        DEBUGSTRING(("CANCEL TRANSFER (operation %d)", aOpCode));
        iDoRequestL = &TRFeCancelAll::DoRequestL;
        break;     
        }   

    iIsSync = ETrue;
    CRsfwRfeOperation::Set(aOpCode); 
    }

