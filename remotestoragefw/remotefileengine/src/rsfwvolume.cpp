/*
* Copyright (c) 2003-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  data structure for a volume
*
*/


#include "rsfwvolume.h"
#include "rsfwvolumetable.h"
#include "rsfwfiletable.h"
#include "rsfwfileengine.h"

// ----------------------------------------------------------------------------
// CRsfwVolume::~CRsfwVolume
// 
// ----------------------------------------------------------------------------
//
CRsfwVolume::~CRsfwVolume()
    {
    delete iFileEngine;
    }

// ----------------------------------------------------------------------------
// CRsfwVolume::MountInfo
// 
// ----------------------------------------------------------------------------
//
TRsfwMountInfo* CRsfwVolume::MountInfo()
    {
    return &iMountInfo;
    }

// ----------------------------------------------------------------------------
// CRsfwVolume::GetMountInfo
// 
// ----------------------------------------------------------------------------
//
void CRsfwVolume::GetMountInfo(TRsfwMountInfo& aMountInfo)
    {
    aMountInfo = iMountInfo;
    aMountInfo.iMountStatus.iConnectionState = iFileEngine->ConnectionState();
    aMountInfo.iMountStatus.iCachedSize =
    iFileEngine->iFileTable->TotalCachedSize();
    }

// ----------------------------------------------------------------------------
// CRsfwVolume::OperationCompleted()
// ----------------------------------------------------------------------------
//
void CRsfwVolume::OperationCompleted() 
    {
    iVolumeTable->OperationCompleted(this);
    }

// ----------------------------------------------------------------------------
// CRsfwVolume::ConnectionStateChanged
// 
// ----------------------------------------------------------------------------
//
void CRsfwVolume::ConnectionStateChanged(TInt aConnectionState)
    {
    // The mount state and connection state are tightly bound together
    switch (aConnectionState)
        {    
        case KMountNotConnected:
            iMountInfo.iMountStatus.iMountState = KMountStateDormant;
            iMountInfo.iMountStatus.iConnectionState = KMountNotConnected;
            break;
        case KMountStronglyConnected:
            iMountInfo.iMountStatus.iMountState = KMountStateMounted;
            iMountInfo.iMountStatus.iConnectionState = KMountStronglyConnected;
            break;
        case KMountConnecting:
            iMountInfo.iMountStatus.iMountState = KMountStateDormant;
            iMountInfo.iMountStatus.iConnectionState = KMountConnecting;
            break;
        default:
            break;
        }
    iVolumeTable->VolumeStateChanged(this);
    }

