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
* Description:  Mount manager
 *
*/


// INCLUDE FILES
#include <badesca.h> 
#include <f32fsys.h> // KMaxVolumeNameLength
#include <rsfwmountman.h>
#include <rsfwmountentry.h>

#include "rsfwinterface.h"
#include "rsfwmountmanimpl.h"


// CONSTANTS
const TUint8 KDefaultDriveLetter = 'J';
 // system default if no other specified; 
 // remote drives should use letters from J: to Y:
 

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::CRsfwMountManImpl
// Constructor
// ----------------------------------------------------------------------------
//
CRsfwMountManImpl::CRsfwMountManImpl()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::NewL
// ----------------------------------------------------------------------------
//
CRsfwMountManImpl* CRsfwMountManImpl::NewL(TUint aDefaultFlags,
                                   MRsfwMountManObserver* aMountManObserver)
    {
    CRsfwMountManImpl* self = new (ELeave) CRsfwMountManImpl();
    CleanupStack::PushL(self);
    self->ConstructL(aDefaultFlags, aMountManObserver);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::~CRsfwMountManImpl
// ----------------------------------------------------------------------------
//
CRsfwMountManImpl::~CRsfwMountManImpl()
    {
    iFs.Close();
    if (iRsfwControlConnected)
        {
        iRsfwControl.Close();
        }
    delete iMountStore;
    
    iBlackList.Close();
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::GetMountNamesL
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::GetMountNamesL(CDesC16Array* aNames)
    {
    iMountStore->GetNamesL(aNames);
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::MountEntry
// ----------------------------------------------------------------------------
//
const CRsfwMountEntry* CRsfwMountManImpl::MountEntryL(const TDesC& aName)
    {
    return iMountStore->LookupEntryByNameL(aName);
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::MountEntry
// ----------------------------------------------------------------------------
//
const CRsfwMountEntry* CRsfwMountManImpl::MountEntryL(TChar aDriveLetter)
    {
    return iMountStore->LookupEntryByDriveL(aDriveLetter);
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::AddMountEntryL
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::AddMountEntryL(CRsfwMountEntry* aMountEntry)
    {
    // Take ownership
    CleanupStack::PushL(aMountEntry);
    
    // Look for existing configuration with the same friendly name
    // overwriting not allowed
    const HBufC* driveName = aMountEntry->Item(EMountEntryItemName);
    const CRsfwMountEntry* entry = MountEntryL(*driveName);
    if (entry)
        {
        User::Leave(KErrInUse);
        }
    // Also look for existing configuration with the same drive letter
    // overwriting not allowed
    TChar driveLetter = DriveLetterFromMountEntry(*aMountEntry);
    if (driveLetter != '?')
        {
        entry = MountEntryL(driveLetter);
        if (entry)
            {
            User::Leave(KErrInUse);
            }
        }
    else 
        {
        driveLetter = FreeDriveLetterL(driveLetter);
        TBuf<1> driveLetterBuf;
        driveLetterBuf.Append(driveLetter);
        // Replace '?' with the allocated drive letter
        aMountEntry->SetItemL(EMountEntryItemDrive, driveLetterBuf);
        }
   
    // find the number of remote drives in use
    // if 9 already new ones are not allowed
    TDriveList fsDriveList;   
    User::LeaveIfError(iFs.DriveList(fsDriveList, KDriveAttRemote));    
    TInt i;
    TInt remoteDrives = 0;
    for (i = EDriveA; i <= EDriveZ; i++)
        { 
        if (fsDriveList[i] & KDriveAttRemote)
            {
            remoteDrives++;
            }
        }
    
    if (remoteDrives == KMaxRemoteDrives)
        {
        User::Leave(KErrInUse);
        }
      
    // Save the friendly name (not guaranteed to survive CMountStore::AddEntryL
    HBufC* name = NULL;
    if (aMountEntry->Item(EMountEntryItemName))
        {
        name = aMountEntry->Item(EMountEntryItemName)->AllocL();
        }

    // Release ownership
    CleanupStack::Pop(aMountEntry);
    // Add mount configuration repository entry
    TRAPD(err, AddEntryL(aMountEntry));
    CleanupStack::PushL(name);
    User::LeaveIfError(err);
    
    // Add mount in File Server
    if (name)
        {
        User::LeaveIfError(MountFileSystem(*name, driveLetter));
        }
    else
        {
        _LIT(KRsfwNoName, "remotedrive");
        User::LeaveIfError(MountFileSystem(KRsfwNoName, driveLetter));
        }
        
    // finally set mountconnection state to disconnected
    // this will make in Remote File Engine what is called "disconnected dormant mount"    
    SetMountConnectionState(driveLetter, KMountNotConnected);
        
    CleanupStack::PopAndDestroy(name);
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::DeleteMountEntryL
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::DeleteMountEntryL(const TDesC& aName)
    {
    const CRsfwMountEntry* mountEntry = iMountStore->LookupEntryByNameL(aName);
    if (mountEntry)
        {
        TChar driveLetter = DriveLetterFromMountEntry(*mountEntry);
        if (driveLetter != '?')
            {
            DeleteMountEntryL(driveLetter);
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::DeleteMountEntryL
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::DeleteMountEntryL(TChar aDriveLetter)
    {
    const CRsfwMountEntry* mountEntry =
        iMountStore->LookupEntryByDriveL(aDriveLetter);
    if (mountEntry)
        {
        // Remove entry from configuration repository
        iMountStore->RemoveEntryL(aDriveLetter);
        // Dismount from file server and Remote File Engine
        ExecuteUnmount(aDriveLetter);
        }
    }
    
// ----------------------------------------------------------------------------
// CRsfwMountManImpl::EditMountEntryL
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::EditMountEntryL(CRsfwMountEntry* aMountEntry)
    {
    // take ownership
    CleanupStack::PushL(aMountEntry);
    
    // look for the drive based on the letter
    TChar driveLetter = DriveLetterFromMountEntry(*aMountEntry);
    const CRsfwMountEntry* entryByDrive = MountEntryL(driveLetter);
    if ( !entryByDrive )
        {
        User::Leave(KErrNotFound);
        }

    // check whether the name has changed
    TBool nameChanged = EFalse;
    const HBufC* newName = aMountEntry->Item(EMountEntryItemName);
    const HBufC* oldName = entryByDrive->Item(EMountEntryItemName);
    if ( newName->Compare(*oldName) != KErrNone )
        {
        nameChanged = ETrue;
        }
        
    // check whether URI has changed
    TBool uriChanged = EFalse;
    const HBufC* newUri = aMountEntry->Item(EMountEntryItemUri);
    const HBufC* oldUri = entryByDrive->Item(EMountEntryItemUri);
    if ( newUri->Compare(*oldUri) != KErrNone )
        {
        uriChanged = ETrue;
        }

    // if the name has changed -> check whether it is not used by the other mount entry
    if ( nameChanged )
        {
        const CRsfwMountEntry* entryByName = MountEntryL(*newName);
        if ( entryByName && entryByDrive != entryByName )
            {
            User::Leave(KErrInUse);
            }        
        }
   
    // release ownership and call MountStore API
    CleanupStack::Pop(aMountEntry);    
    AddEntryL(aMountEntry);

    // if URI has changed we have to unmount the drive from RFE in order 
    // to clear the cache and mount it again
    // note there is no need to make unmounting from File System
    if ( uriChanged )
        {
        TInt err;
        User::LeaveIfError(GetRsfwControlConnection());
        err = iRsfwControl.DismountByDriveLetter(driveLetter);
        if ((err != KErrNone) && (err != KErrNotFound)) 
            {
            User::Leave(err);
            }
        User::LeaveIfError(iRsfwControl.SetMountConnectionState(driveLetter, KMountNotConnected));
        }

    // if the name has changed -> change label in File System
    if ( nameChanged )
        {
        User::LeaveIfError(SetDriveNameToFileSystem(driveLetter, *newName));
        }
    }    

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::MountL
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::MountL(TChar& aDriveLetter)
    {
    TInt driveNumber;
    User::LeaveIfError(iFs.CharToDrive(aDriveLetter, driveNumber));
    User::LeaveIfError(GetRsfwControlConnection());
    User::LeaveIfError(iRsfwControl.Mount(driveNumber));
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::MountL
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::MountBlindL(TChar& aDriveLetter)
    {
    TInt driveNumber;
    User::LeaveIfError(iFs.CharToDrive(aDriveLetter, driveNumber));
    User::LeaveIfError(GetRsfwControlConnection());
    User::LeaveIfError(iRsfwControl.MountBlind(driveNumber));
    }


// ----------------------------------------------------------------------------
// CRsfwMountManImpl::SetMountConnectionState
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::SetMountConnectionState(TChar aDriveLetter,
                                            TUint aConnectionState)
    {
    TInt err = GetRsfwControlConnection();
    if (err != KErrNone)
        {
        return err;
        }

    err = iRsfwControl.SetMountConnectionState(aDriveLetter, aConnectionState);
    return err;
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::IsAppOnBlackList
// ----------------------------------------------------------------------------
//
TBool CRsfwMountManImpl::IsAppOnBlackList(TUid aUid)
    {
    TInt i;
    for ( i = 0; i < iBlackList.Count(); i++ )
        {
        if ( aUid == iBlackList[i] )
            {
            return ETrue;
            }
        }
    return EFalse;
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::GetAllDrivesL
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::GetAllDrivesL(TDriveList& aDriveList)
    {
    GetFsDriveListL(aDriveList, EFalse);
    return aDriveList.Length();
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::GetRemoteMountList
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::GetRemoteMountListL(TDriveList& aDriveList)
    {
    GetFsDriveListL(aDriveList, ETrue);
    return aDriveList.Length();
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::GetMountInfo
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::GetMountInfo(TChar aDriveLetter,
                                 TRsfwMountInfo& aMountInfo)
    {
    TInt err = GetRsfwControlConnection();
    if (err == KErrNone)
        {
        err = iRsfwControl.GetMountInfo(aDriveLetter, aMountInfo);
        }
    return err;
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::GetRsfwControlConnection
// Set a Remote File Engine control connection unless
// we already have a connection
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::GetRsfwControlConnection()
    {
    TInt err = KErrNone;
    if (!iRsfwControlConnected)
        {
        err = iRsfwControl.Connect();
        if (!err) 
            {
            iRsfwControlConnected = ETrue;   
            }
        }
    return err;
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::MountFileSystem
// Install ERemoteFS file system plugin (if not already done) and
// mount a filesystem designated with the given drive letter
// on that file system
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::MountFileSystem(const TDesC& /*aDriveName*/,
                                    TChar aDriveLetter)
    {
    return SyncWithMounterExe(ETrue, aDriveLetter);
    }
        

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::RemoteDriveCount
// Get the number of remote mounts as seen by the File Server
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::RemoteMountCountL()
    {
    // Check if how many mounts there are (also dormant mounts are counted)
    TDriveList driveList;
    return GetRemoteMountListL(driveList);
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::DoUnmountL
// Do conditional unmounting by consulting the user
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::DoUnmountL(TChar aDriveLetter, TUint /* aFlags */)
    {
    TInt err = ExecuteUnmount(aDriveLetter);
    User::LeaveIfError(err);
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::ExecuteUnmount
// Do unconditional unmounting
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::ExecuteUnmount(TChar aDriveLetter)
    {
    TInt err = GetRsfwControlConnection();
    if (err != KErrNone)
        {
        return err;
        }

    // Drop the mount both from the File Server and from Remote File Engine
    TInt errFs = KErrNone;
    TInt driveNumber;
    err = iFs.CharToDrive(aDriveLetter, driveNumber);
    if (err != KErrNone)
        {
        return err;
        }
    TDriveInfo driveInfo;
    err = iFs.Drive(driveInfo, driveNumber);
    if (err != KErrNone)
        {
        return err;
        }    
    if (driveInfo.iDriveAtt & KDriveAttRemote)
        {
        // The mount is known by the File Server
        errFs = iFs.DismountFileSystem(KRemoteFSName, driveNumber);
        if (errFs == KErrPermissionDenied) 
    	    {
    		// Client does not have sufficient capabilities to do the operation
    		// execute mount with the boot mounter application
    		SyncWithMounterExe(ETrue, aDriveLetter);
    	    }
        }
    
    // We also request dismount from the RFE because
    // The File Server (eremotefs) does not pass the
    // dismount request to the RFE.
    err = iRsfwControl.DismountByDriveLetter(aDriveLetter);
    if (errFs != KErrNone)
        {
        // give priority to the File Server error
        err = errFs;
        }
    return err;
    }


// ----------------------------------------------------------------------------
// CRsfwMountManImpl::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::ConstructL(TUint aDefaultFlags,
                                   MRsfwMountManObserver* aMountManObserver)
    {
    iDefaultFlags = aDefaultFlags;

    User::LeaveIfError(iFs.Connect());
    iRsfwControlConnected = EFalse;
    iMountManObserver = aMountManObserver;
    iMountStore = CRsfwMountStore::NewL(this);
    LoadBlackListL();
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::FreeDriveLetterL
// Find a drive letter that has not yet been allocated by the File Server or
// defined in the mount configuration data base
// If the suggested drive letter (given as an input parameter)
// is already occupied, an adjacent letter in the alphabet is returned.
// Letter '?' as an input parameter is equal to KDefaultDriveLetter.
// Leave with KErrInUse if there are no free drive slots
// ----------------------------------------------------------------------------
//
TChar CRsfwMountManImpl::FreeDriveLetterL(TChar aDriveLetter)
    {
    if (aDriveLetter == '?')
        {
        // Use the system default drive letter if none is specified
        aDriveLetter = KDefaultDriveLetter;
        }
    else if ((aDriveLetter < 'A') || (aDriveLetter > 'Z'))
        {
        aDriveLetter.UpperCase();
        }

    TInt driveNumber;
    User::LeaveIfError(iFs.CharToDrive(aDriveLetter, driveNumber));

    // Try to find a free drive around the given drive

    // Get drives that are already mounted in the File Server
    TDriveList fsDriveList;
    User::LeaveIfError(iFs.DriveList(fsDriveList, KDriveAttAll));  
    // scan first upwards, then downwards from the requested drive 
    // (or default drive letter) until a free and legal drive letter is found
    // remote drives should use letters from J: to Y:
    TInt i;
    for (i = driveNumber; i <= EDriveY; i++)
        { 
        if (fsDriveList[i] == 0)
            {
            User::LeaveIfError(iFs.DriveToChar(i, aDriveLetter));
            return aDriveLetter;
            }
        }
    for (i = driveNumber - 1; i >= EDriveJ; i--)
        { 
        if (fsDriveList[i] == 0)
            {
            User::LeaveIfError(iFs.DriveToChar(i, aDriveLetter));
            return aDriveLetter;
            }
        }
    
    //  no free drive letters for remote drives
    User::Leave(KErrInUse);
    return 0;
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::DriveLetterFromMountEntry
// Build a TRsfwMountConfig
// ----------------------------------------------------------------------------
//
TChar CRsfwMountManImpl::DriveLetterFromMountEntry(const CRsfwMountEntry& aMountEntry)
    {
    const HBufC* drive = aMountEntry.Item(EMountEntryItemDrive);
    if (drive && drive->Length())
        {
        return (*drive)[0];
        }
    else
        {
        return '?';
        }
    }
 
// ----------------------------------------------------------------------------
// CRsfwMountManImpl::AddEntryL
// Add an entry in the mount configurarion store
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::AddEntryL(CRsfwMountEntry* aMountEntry)
    {
    iMountStore->AddEntryL(aMountEntry);
    }
 

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::HandleMountStoreEvent
// Handle a mount store change notification
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::HandleMountStoreEvent(TMountStoreEvent aEvent,
                                          TInt aStatus,
                                          TAny* aArg)
    {
    if (iMountManObserver)
        {
        if (aEvent == EMountStoreEventMountConfigurationChanged)
            {
            TRAP_IGNORE(iMountManObserver->HandleMountManEventL(
                EMountManEventMountConfigurationChanged,
                aStatus,
                aArg));
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::SyncWithMounterExe
// Uses boot mounter exe to mount or unmount remote drives in File Server
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::SyncWithMounterExe(TBool aSetDrive, TChar aDrive) 
	{
	TInt err;
	RProcess mounter;
	TRequestStatus status;
	TBuf<5> parameter;
	if (!aSetDrive) 
	    {
	    err = mounter.Create(KRsfwMounterExe, _L(""));
	    }
    else 
        {
        parameter.Append(aDrive);
        err = mounter.Create(KRsfwMounterExe, parameter);
        }

    if (err == KErrNone) 
    	{
    	mounter.Resume();
    	mounter.Logon(status);
    	User::WaitForRequest(status);
    	mounter.Close();
    	err = status.Int();	
    	} 
    return err;
	}
        
// ----------------------------------------------------------------------------
// CRsfwMountManImpl::GetFsDriveList
//
// Returns drive letters. Letters for remote drives in order defined in CenRep
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::GetFsDriveListL(TDriveList& aDriveList, TBool aRemoteOnly)
    {
    TDriveList driveList;
    // get local drives
    User::LeaveIfError(iFs.DriveList(driveList));
    
   // local drives are assumed to be from C:\ to R:\
    // these will go to the front of the list
    if (!aRemoteOnly) 
        {
        for (int i = EDriveC; i <= EDriveZ; i++) 
            {
            if ( (driveList[i]) && (!(driveList[i] & KDriveAttRemote)) )
                {
                TChar driveLetter;
                User::LeaveIfError(iFs.DriveToChar(i, driveLetter));
                aDriveList.Append(driveLetter);
                }
            }
        }
      
    GetRemoteDriveListL(driveList);   
    
    for (int i = 0; i < driveList.Length(); i++) 
        {
        aDriveList.Append(driveList[i]);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::GetRemoteDriveList
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::GetRemoteDriveListL(TDriveList& aDriveList)
    {
    iMountStore->GetDriveLettersL(aDriveList); 
    return aDriveList.Length();
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::SetDriveNameToFileSystem
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::SetDriveNameToFileSystem(TChar aDriveLetter,
                                                 const TDesC& /*aDriveName*/)
    {
    return SyncWithMounterExe(ETrue, aDriveLetter); 
    }
    
// ----------------------------------------------------------------------------
// CRsfwMountManImpl::RefreshDirectoryL
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::RefreshDirectory(const TDesC& aPath)
    {
    TInt err;
    err = GetRsfwControlConnection();
    if (err != KErrNone) 
        {
        return err;
        }
    return iRsfwControl.RefreshDirectory(aPath); 
    }
    
// ----------------------------------------------------------------------------
// CRsfwMountManImpl::LoadBlackListL
// ----------------------------------------------------------------------------
//
void CRsfwMountManImpl::LoadBlackListL()
    {

//    const TUid app1 = TUid::Uid(0x00000000);
//    const TUid app2 = TUid::Uid(0x00000001);
//    const TUid app3 = TUid::Uid(0x00000002);
    
    iBlackList.Reset();
//    iBlackList.AppendL(app1);
//    iBlackList.AppendL(app2);
//    iBlackList.AppendL(app3);
    }

// ----------------------------------------------------------------------------
// CRsfwMountManImpl::LoadBlackListL
// ----------------------------------------------------------------------------
//
TInt CRsfwMountManImpl::CancelRemoteTransfer(const TDesC& aFile)
    {
    TInt err;
    err = GetRsfwControlConnection();
    if (err != KErrNone) 
        {
        return err;
        }
    return iRsfwControl.CancelRemoteTransfer(aFile);  
    }
    
//  End of File
