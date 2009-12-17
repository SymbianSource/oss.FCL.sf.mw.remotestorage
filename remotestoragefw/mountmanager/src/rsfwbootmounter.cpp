/*
* Copyright (c) 2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Mounts during boot remote drives configured in CenRep
 *
*/


#include <f32file.h>	// link against efsrv.lib
#include <centralrepository.h> // link against centralrepository.lib 
#include <f32fsys.h>

// for the file server client side API   
_LIT(KRemoteFSName, "RemoteFS");
_LIT(KRemoteFs, "eremotefs");
   
   
// for the central repository API   
enum TMountEntryItemIndex
    {
    EMountEntryItemIndex, 
    EMountEntryItemName,
    EMountEntryItemDrive,
    EMountEntryItemUri,
    EMountEntryItemUserName,
    EMountEntryItemPassword,
    EMountEntryItemIap,
    EMountEntryItemInactivityTimeout,
    EMountEntryItemReserved,
    EMountEntryItemCount
    };
const TUid  KCRUidRsfwCtrl = { 0x101F9775 }; // RSFW cenrep table id
const TUint KColumnName       = EMountEntryItemName;
const TUint KColumnMask       = 0x000000ff; 
const TUint KRowMask          = 0xffffff00;
const TInt KMaxFileSystemName = 256;
const TInt KMaxDriveLetterLength = 5;
const TInt KMaxFriendlyNameLength = 20;
 

//  isRsfwFileSystem
//
//  Checks whether certain drive has rsfw file system mounted on it
//
//  
LOCAL_C TBool isRsfwFileSystemL(TInt aDrive,
                              TDriveList aDriveList,
                              RFs aFsSession) 
    {
    TBool rsfwFileSystem = EFalse;
    // check first KDriveAttRemote bit as if that is missing it is a quick way 
    // to conclude that this is not a rsfw mount
    if (aDriveList[aDrive] & KDriveAttRemote) 
        {
        TInt err;
        HBufC* filesystemName = HBufC::NewL(KMaxFileSystemName);
        TPtr itemPtr = filesystemName->Des();
        err = aFsSession.FileSystemName(itemPtr, aDrive);
        if (!err && (itemPtr.Compare(KRemoteFSName) == 0)) 
            {
            rsfwFileSystem = ETrue;
            }
        delete filesystemName;
        }
    return rsfwFileSystem;
    }
 
 
 
//  DoMountL
//
//  Do either simple mount or replace (our) existing mount
//  by doing dismount and mount
//
//  
LOCAL_C TInt DoMount(TInt aDrive, RFs aFsSession, TBool aReplaceExisting)

    {
    TInt err;
    if (aReplaceExisting) 
        {
        // ignore dismount error code
        aFsSession.DismountFileSystem(KRemoteFSName, aDrive);
        err = aFsSession.MountFileSystem(KRemoteFSName, aDrive);
        }
    else 
        {
        err = aFsSession.MountFileSystem(KRemoteFSName, aDrive);
        }
    return err;
    }
 

//  ExecuteFileServerMountL
//
//  Attempts to mount our file system to the file server
//
// 
LOCAL_C TInt ExecuteFileServerMountL(TInt aDrive,
                                    TDriveList aDriveList,
                                    RFs aFsSession)
    {
    TInt err;
    if (aDriveList[aDrive]) 
        {
        if (isRsfwFileSystemL(aDrive, aDriveList, aFsSession)) 
            {
            err = DoMount(aDrive, aFsSession, ETrue);
            }
        else 
            {
            // the drive we attempt to mount contains some other file system
            return KErrInUse;
            }
        }
    else 
        {
        // the drive we attempt to mount does not contain an existing mount
        err = DoMount(aDrive, aFsSession, EFalse);
        }
    return err;
    }


//  SetFriendlyNameL
//  
//  Sets the friendly name of the remote drive to 
//  mounted fs drivename and volumename fields
//  (to the volume name field first 11 chars)
//
//  This function assumes that we are allowed to manipulate
//  drive aDrive, in practise that ExecuteFileServerMount()
//  has been called succesfully
// 
LOCAL_C TInt SetFriendlyName(TInt aDrive,
                             TDesC& aFriendlyName,
                             RFs aFsSession)
    {
    TInt err;
    err = aFsSession.SetDriveName(aDrive, aFriendlyName);
    if (!err) 
        {
        TPtrC volumeNamePtr = aFriendlyName.Left(KMaxVolumeNameLength);
        err = aFsSession.SetVolumeLabel(volumeNamePtr, aDrive);
        }
    return err;
    }

// ----------------------------------------------------------------------------
// SyncConfiguredRemoteDrivesL
// adds RSFW File Server plug-in 
// and synchronizes Central Repository's view of configured remote drives 
// with File Server's view (mounts the remote drives configured in CR and
// unmounts those remote drives that are not anymore configured)   
// ----------------------------------------------------------------------------
//  
LOCAL_C void SyncConfiguredRemoteDrivesL()
    {  
    TInt err = 0;
    TInt row = 0;
    TInt driveNumber = 0;
    RFs fs;
    TChar paramLetter(90);
    TBool paramSet = EFalse;
    
    User::LeaveIfError(fs.Connect());
    CleanupClosePushL(fs);
    
    // it is possible to manipulate only one drive letter, and give that as an argument
    TInt clinelength = User::CommandLineLength();
    if (clinelength > 0) 
        {
        HBufC* cl = HBufC::NewL(clinelength);
        TPtr linePtr = cl->Des();
        User::CommandLine(linePtr);
        TLex lex(linePtr);
        paramLetter = lex.Get();
        paramSet = ETrue;
        delete cl;
        }
    
 	// add our file system plugin to the file server
    err = fs.AddFileSystem(KRemoteFs);
    if ((err != KErrNone) && (err != KErrAlreadyExists)) 
    	{
    	User::Leave(err);
    	}
    	
    // Get a list of drives in the File Server
    TDriveList drives;
    User::LeaveIfError(fs.DriveList(drives, KDriveAttAll));  
    // 	(drives[i] & KDriveAttRemote) now tells whether i:th drive is remote
    
    // Get a list of remote drives in the central repository table
    
    // connect
   	CRepository* cenrep;
    cenrep = CRepository::NewL(KCRUidRsfwCtrl);
    CleanupStack::PushL(cenrep);
    
     // find all entries by name
    RArray<TUint32> nameIds;
    CleanupClosePushL(nameIds);
    err = cenrep->FindL(KColumnName, KColumnMask, nameIds);
    if (!err)
    	{
    	// for each remote drive entry, represented by a record in central repository, do the following:
    	// 1) get drive letter from central repository
    	// 2) based on drive letter acquire corresponding drive number from the File Server
    	// 3) check whether in the File Server there is already mounted a drive with given number
    	//    3.1) if there is NOT, then mount the drive
    	//    3.2) if there is and it appears to be remote, then do re-mounting
    	//    3.3) if there is and it appears NOT to be remote, then this means error
    	// 4) still for the same record from central repository, get the name of the drive
    	// 5) use this name as the volume label in the File Server
    	for (row = 0; row < nameIds.Count(); row++)
        	{
        	TUint rowId = (nameIds[row] & KRowMask);
        	// don't touch zero row as it DOES NOT contain info about mounted drives
        	if (rowId > 0) 
        	    {
        	    TInt i;
            	// mount it to the file server
            	
            	// get drive number
            	TBuf<KMaxDriveLetterLength> driveLetter;
            	i = EMountEntryItemDrive;
            	User::LeaveIfError(cenrep->Get(rowId | i, driveLetter));
            	
            	// driveNumber needed in any case, so that we can mark this as existing drive
            	if (driveLetter.Length() > 0) 
            	    {
            	    User::LeaveIfError(fs.CharToDrive((driveLetter)[0], driveNumber));
            	    }
            	 else 
            	    {
            	    User::Leave(KErrBadName);
            	    }
            	
            	// proceed this drive if we didn't get any drive
            	// letter as a parameter or we got this one
                if ((!paramSet) || (paramLetter ==  driveLetter[0]))
                    {
                    // get friendly name
            	    TBuf<KMaxFriendlyNameLength> friendlyName;
            	    i = EMountEntryItemName;
            	    User::LeaveIfError(cenrep->Get(rowId | i, friendlyName));
            	
            	    if (friendlyName.Length() > 0) 
            	        {
            		    User::LeaveIfError(ExecuteFileServerMountL(driveNumber, drives, fs));
            		    User::LeaveIfError(SetFriendlyName(driveNumber, friendlyName,fs));		
            		    }
            	
            	    else
            		    {
            		    User::Leave(KErrBadName);
            		    }
                    }
                    
            	// do not unmount this drive as it is still in the CenRep table
            	// after this loop non-zero drives are used to see 
            	// which remote drives should be unmounted	
            	drives[driveNumber] = 0; 
        	    }
            } // loop
    	}
    	
    	// If drives now contain some remote drives, this means that they are not 
    	// anymore configured in CenRep and should be unmounted. 
       	for (int i = 0; i < EDriveZ; i++) 
       		{
       		if (isRsfwFileSystemL(i, drives, fs))
       			{
       			fs.DismountFileSystem(KRemoteFSName, i);
       			}
       		}
    
    CleanupStack::PopAndDestroy(3, &fs);   // fs, cenrep, nameIds
    
    }
    
// ----------------------------------------------------------------------------
// E32Main
// 
// ----------------------------------------------------------------------------
//   
GLDEF_C TInt E32Main() 
    {
    CTrapCleanup* cleanupStack = CTrapCleanup::New();
    TRAPD(err, SyncConfiguredRemoteDrivesL());
    delete cleanupStack;
    return err;
    }


// End of file
