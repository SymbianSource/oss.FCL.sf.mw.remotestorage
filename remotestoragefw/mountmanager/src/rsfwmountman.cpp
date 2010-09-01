/*
* Copyright (c) 2004-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  RSFW Mount Manager API
*
*/


// INCLUDE FILES
#include <rsfwmountman.h>
#include "rsfwmountmanimpl.h"

// ============================ MEMBER FUNCTIONS ==============================
// ----------------------------------------------------------------------------
// TRsfwMountConfig::ExternalizeL
// ----------------------------------------------------------------------------
//
EXPORT_C void TRsfwMountConfig::ExternalizeL(RWriteStream& aStream) const
    {
    aStream.WriteUint32L(iDriveLetter);
    aStream << iName;
    aStream << iUri;
    aStream << iUserName;
    // Don't externalize a password that is not part of the mount configuration
    // (this is for security)
    if (iFlags & KMountFlagAskPassword)
        {
        TPtrC emptyPassword;
        aStream << emptyPassword;
        }
    else
        {
        aStream << iPassword;
        }
    aStream << iAuxData;
    aStream.WriteUint32L(iFlags);
    aStream.WriteInt32L(iInactivityTimeout);
    }


// ----------------------------------------------------------------------------
// TRsfwMountConfig::InternalizeL
// ----------------------------------------------------------------------------
//
EXPORT_C void TRsfwMountConfig::InternalizeL(RReadStream& aStream)
    {
    iDriveLetter = aStream.ReadUint32L();
    aStream >> iName;
    aStream >> iUri;
    aStream >> iUserName;
    aStream >> iPassword;
    aStream >> iAuxData;
    iFlags = aStream.ReadUint32L();
    iInactivityTimeout = aStream.ReadInt32L();
    }


// ----------------------------------------------------------------------------
// TRsfwMountInfo::ExternalizeL
// ----------------------------------------------------------------------------
//
void TRsfwMountInfo::ExternalizeL(RWriteStream& aStream) const
    {
    iMountConfig.ExternalizeL(aStream);
    }


// ----------------------------------------------------------------------------
// TRsfwMountInfo::InternalizeL
// ----------------------------------------------------------------------------
//
void TRsfwMountInfo::InternalizeL(RReadStream& aStream)
    {
    Mem::FillZ(this, sizeof(*this));
    iMountConfig.InternalizeL(aStream);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::CRsfwMountMan
// Constructor
// ----------------------------------------------------------------------------
//
CRsfwMountMan::CRsfwMountMan()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwMountMan::ConstructL(TUint aDefaultFlags,
                           MRsfwMountManObserver* aMountManObserver)
    {
    iMountManImpl = CRsfwMountManImpl::NewL(aDefaultFlags, aMountManObserver);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::NewL
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwMountMan* CRsfwMountMan::NewL(TUint aDefaultFlags,
                                    MRsfwMountManObserver* aMountManObserver)
    {
    CRsfwMountMan* self = new (ELeave) CRsfwMountMan();
    CleanupStack::PushL(self);
    self->ConstructL(aDefaultFlags, aMountManObserver);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::~CRsfwMountMan
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwMountMan::~CRsfwMountMan()
    {
    delete iMountManImpl;
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::GetMountNamesL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountMan::GetMountNamesL(CDesC16Array* aNames) const
    {
    iMountManImpl->GetMountNamesL(aNames);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::MountEntry
// ----------------------------------------------------------------------------
//
EXPORT_C const CRsfwMountEntry* CRsfwMountMan::MountEntryL(const TDesC& aName) const
    {
    return iMountManImpl->MountEntryL(aName);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::MountEntry
// ----------------------------------------------------------------------------
//
EXPORT_C const CRsfwMountEntry* CRsfwMountMan::MountEntryL(TChar aDriveLetter) const
    {
    return iMountManImpl->MountEntryL(aDriveLetter);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::AddMountEntryL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountMan::AddMountEntryL(CRsfwMountEntry* aMountEntry)
    {
    iMountManImpl->AddMountEntryL(aMountEntry);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::DeleteMountEntry
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountMan::DeleteMountEntryL(const TDesC& aName)
    {
    iMountManImpl->DeleteMountEntryL(aName);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::DeleteMountEntry
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountMan::DeleteMountEntryL(TChar aDriveLetter)
    {
    iMountManImpl->DeleteMountEntryL(aDriveLetter);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::GetAllDrivesL
// ----------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwMountMan::GetAllDrivesL(TDriveList& aDriveList) const
    {
    return iMountManImpl->GetAllDrivesL(aDriveList);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::GetRemoteMountList
// ----------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwMountMan::GetRemoteMountListL(TDriveList& aDriveList) const
    {
    return iMountManImpl->GetRemoteMountListL(aDriveList);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::GetMountInfo
// ----------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwMountMan::GetMountInfo(TChar aDriveLetter,
                                      TRsfwMountInfo& aMountInfo) const
    {
    return iMountManImpl->GetMountInfo(aDriveLetter, aMountInfo);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::SetMountConnectionState
// ----------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwMountMan::SetMountConnectionState(TChar aDriveLetter,
                                                 TUint aConnectionState)
    {
    if (aConnectionState == KMountStronglyConnected) 
        {
        TRsfwMountConfig mountConfig;
	    mountConfig.iDriveLetter = aDriveLetter;
	    mountConfig.iFlags = KMountFlagMountAtRfeOnly;
	    TRAPD(err, iMountManImpl->MountL(aDriveLetter));
	    return err;
        }
   else 
        {
        return iMountManImpl->SetMountConnectionState(aDriveLetter,
                                                  aConnectionState);     
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::SetMountConnectionStateBlind
// ----------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwMountMan::SetMountConnectionStateBlind(TChar aDriveLetter,
                                                 TUint aConnectionState)
    {
    if (aConnectionState == KMountStronglyConnected) 
        {
        TRsfwMountConfig mountConfig;
	    mountConfig.iDriveLetter = aDriveLetter;
	    mountConfig.iFlags = KMountFlagMountAtRfeOnly;
	    TRAPD(err, iMountManImpl->MountBlindL(aDriveLetter));
	    return err;
        }
   else 
        {
        return iMountManImpl->SetMountConnectionState(aDriveLetter,
                                                  aConnectionState);     
        }
    }


// ----------------------------------------------------------------------------
// CRsfwMountMan::EditMountEntryL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountMan::EditMountEntryL(CRsfwMountEntry* aMountEntry)
    {
    iMountManImpl->EditMountEntryL(aMountEntry);
    }    


// ----------------------------------------------------------------------------
// CRsfwMountMan::RefreshDirectoryL
// ----------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwMountMan::RefreshDirectory(const TDesC& aPath)
    {
    return iMountManImpl->RefreshDirectory(aPath);
    }

// ----------------------------------------------------------------------------
// CRsfwMountMan::IsAppOnBlackList
// ----------------------------------------------------------------------------
//
EXPORT_C TBool CRsfwMountMan::IsAppOnBlackList(TUid aUid) const
    {
    return iMountManImpl->IsAppOnBlackList(aUid);
    }    

// ----------------------------------------------------------------------------
// CRsfwMountMan::CancelAllRemoteTransfers
// ----------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwMountMan::CancelRemoteTransfer(const TDesC& aFile)
    {
    return iMountManImpl->CancelRemoteTransfer(aFile);
    }    


//  End of File
