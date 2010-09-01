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


#ifndef CRSFWMOUNTMANIMPL_H
#define CRSFWMOUNTMANIMPL_H


// INCLUDES
#include "rsfwmountstore.h"
#include "rsfwcontrol.h"

// FORWARD DECLARATIONS
class MRsfwMountManObserver;

// CLASS DECLARATION
/**
 *  Class for implementing mount management of remote file repositories
 *
 *  @lib mountman.dll
 *  @since Series 60 3.1
 */

class CRsfwMountManImpl : public CBase, public MRsfwMountStoreObserver
    {
public: // Constructors and destructor
    /**
     * Two-phased constructor.
     *
     * @param aDefaultFlags must be set to KMountFlagInteractive
     *   if the user is to be prompted during the mount procedure.
     *   Otherwise the parameter can be omitted (defaults to zero)
     * @param mount event observer
     * @return pointer to the created CRsfwMountManImpl object instance
     */
    static CRsfwMountManImpl* NewL(TUint aDefaultFlags,
                               MRsfwMountManObserver* aMountManObserver);

    /**
     * Destructor.
     */
    virtual ~CRsfwMountManImpl();
    
public: // New functions
    // Methods that implement the mount management functions
    // See MountMan.h for documentation
    void GetMountNamesL(CDesC16Array* aNames);

    const CRsfwMountEntry* MountEntryL(const TDesC& aName);

    const CRsfwMountEntry* MountEntryL(TChar aDriveLetter);

    void AddMountEntryL(CRsfwMountEntry* aMountEntry);

    void DeleteMountEntryL(const TDesC& aName);

    void DeleteMountEntryL(TChar aDriveLetter);

    void EditMountEntryL(CRsfwMountEntry* aMountEntry);

    void MountL(TChar& aDriveLetter);
    void MountBlindL(TChar& aDriveLetter);

    TInt GetAllDrivesL(TDriveList& aDriveList);

    TInt GetRemoteMountListL(TDriveList& aDriveList);
    
    TInt GetMountInfo(TChar aDriveLetter, TRsfwMountInfo& aMountInfo);

    TInt SetMountConnectionState(TChar aDriveLetter,
                                 TUint aConnectionState);

    TInt RefreshDirectory(const TDesC& aPath);
    
    TBool IsAppOnBlackList(TUid aUid);
    
    TInt CancelRemoteTransfer(const TDesC& aFile);


    // Methods from MMountStoreObserver
    void HandleMountStoreEvent(TMountStoreEvent aEvent,
                               TInt aStatus,
                               TAny* aArg);

private:
    enum TRsfwMountState
        {
        EMountStateIdle = 0,
        EMountStateWait,
        EMountStateCanceled
        };

private:
    // Default constructor
    CRsfwMountManImpl();

    void ConstructL(TUint aDefaultFlags,
                    MRsfwMountManObserver* aMountManObserver);
    TInt GetRsfwControlConnection();
    TInt MountFileSystem(const TDesC& aDriveName, TChar aDriveLetter);
    TInt RemoteMountCountL();
    
    
    void DoUnmountL(TChar aDriveLetter, TUint aFlags);
    TInt ExecuteUnmount(TChar aDriveLetter);
    TChar FreeDriveLetterL(TChar aDriveLetter);
    TChar DriveLetterFromMountEntry(const CRsfwMountEntry& aMountEntry);
    void AddEntryL(CRsfwMountEntry* aMountEntry);
    TInt SyncWithMounterExe(TBool aSetDrive, TChar aDrive = EDriveQ);
    void GetFsDriveListL(TDriveList& aDriveList, TBool aRemoteOnly);
    TInt GetRemoteDriveListL(TDriveList& aDriveList);
    TInt SetDriveNameToFileSystem(TChar aDriveLetter,
                                 const TDesC& aDriveName);
    void LoadBlackListL();

private: // Data
    RFs                           iFs;
    RRsfwControl                  iRsfwControl;
    TBool                         iRsfwControlConnected;
    TUint                         iDefaultFlags;  // default mount options
    TRsfwMountInfo                iMountInfo;     // current mount info
    TRsfwMountState               iMountState;
    MRsfwMountManObserver*        iMountManObserver;
    CRsfwMountStore*              iMountStore;
    RArray<TUid>                  iBlackList;
    };

#endif // CRSFWMOUNTMANIMPL_H

// End of File
