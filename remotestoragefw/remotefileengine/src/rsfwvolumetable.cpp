/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:   data struct for all volumes
*
*/


#include <sysutil.h>
#include <bautils.h>
#include <rsfwmountman.h>
#include <rsfwmountentry.h>

#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwinterface.h"
#include "rsfwmountstatemachine.h"
#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwconfig.h"
#include "rsfwfileengine.h"
#include "rsfwrfeserver.h"

#include "rsfwmountstore.h"
#include "rsfwwaitnotemanager.h"
#include "rsfwdormantmountloader.h"
#include "mdebug.h"

//CONSTANTS
_LIT(KRsfwLruFileName, "lru.dat");
_LIT(KRsfwRestorePendingMark, "rsfwmetadatarestorepending.dat");

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::NewL
//
// ----------------------------------------------------------------------------
//
CRsfwVolumeTable* CRsfwVolumeTable::NewL(CRsfwRfeServer* aRfeServer,
                                 CRsfwConfig* aRsfwConfig)
    {
    CRsfwVolumeTable* self = new (ELeave) CRsfwVolumeTable;
    DEBUGSTRING(("CRsfwVolumeTable: in NewL 0x%x", self));
    CleanupStack::PushL(self);
    self->ConstructL(aRfeServer, aRsfwConfig);
    CleanupStack::Pop(self);
    return self;
    }


// ----------------------------------------------------------------------------
// CRsfwVolumeTable::RestoreDormantMountsL
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::RestoreDormantMountsL() 
    {
    DEBUGSTRING(("CRsfwVolumeTable::RestoreDormantMountsL enter"));
    // Currently, we always enable persistence (used to be configurrable)
    iPermanence = ETrue;
    if (iPermanence)
        {
        // if restoring permanent state causes RFE panic, we must delete 
        // the old permanent metadata and skip restoring it
        // otherwise RFE will just crash every time
        // Strategy is to add a file that tells that the process is ongoing.
        // If it is not able to finish successfully, we can conclude that there was a crash
        // marker is deleted from CRsfwDormantMountLoader, when we have succesfully also checked for dirty files
        TBool internalize = CheckAndAddProcessStartMarker();
        if (internalize) 
            {
            WillLRUPriorityListBeInternalized();
            RestoreVolumesL();
            InternalizeLRUPriorityListL();
            }
        else 
            {
            CleanupCorruptedCacheL();
            }
        }
    iDormantMountRestorePending = EFalse;   
    DEBUGSTRING(("CRsfwVolumeTable::RestoreDormantMountsL exit"));
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::ConstructL
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::ConstructL(CRsfwRfeServer* aRfeServer, CRsfwConfig* aRsfwConfig)
    {
    iRfeServer = aRfeServer;
    iRsfwConfig = aRsfwConfig;

    iAllEnginesIdle = ETrue;
    
    // set some local pointers for commonly used variable from CRsfwRfeServer::Env()
    iFs = CRsfwRfeServer::Env()->iFs;
    iCacheRoot = &(CRsfwRfeServer::Env()->iCacheRoot);
    
    TInt err = iRsfwConfig->Get(RsfwConfigKeys::KMaxCacheSize,
                                iMaxCacheSize);
    if (err != KErrNone)
        {
        iMaxCacheSize = KDefaultMaxCacheSize;
        }

    err = iRsfwConfig->Get(RsfwConfigKeys::KMaxEntryCount,
                           iMaxEntryCount);
    if (err != KErrNone)
        {
        iMaxEntryCount = KDefaultMaxEntryCount;
        }

    HBufC* confItem = HBufC::NewLC(KMaxRsfwConfItemLength);
    TPtr confItemPtr = confItem->Des();

    // global wait notes manager class
    // must be created before restoring  the volumes
    iWaitNoteManager = CRsfwWaitNoteManager::NewL();
    
    RestoreDormantMountsL();
    
    err = iRsfwConfig->Get(RsfwConfigKeys::KFileCacheValidity,
                           iFileCacheTimeout);
    if (err)
        {
        iFileCacheTimeout = KDefaultCacheValidity;
        }
    err = iRsfwConfig->Get(RsfwConfigKeys::KDirCacheValidity,
                           iDirCacheTimeout);
    if (err)
        {
        iDirCacheTimeout = KDefaultDirCacheValidity;
        }

    err = iRsfwConfig->Get(RsfwConfigKeys::KRecognizerLimit,
                           iRecognizerLimit);
    if (err)
        {
        iRecognizerLimit = KDefaultRecognizerLimit;
        }

    err = iRsfwConfig->Get(RsfwConfigKeys::KCachingMode, confItemPtr);
    if (err == KErrNone)
        {
        TLex lex(confItemPtr);
        TChar firstChar = lex.Get();
        firstChar.UpperCase();
        switch (firstChar)
            {
        case 'W':
            iCachingMode = EWholeFileCaching;
            break;

        case 'F':
            iCachingMode = EFullIfa;
            break;

        case 'M':
        default:
            iCachingMode = EMetadataIfa;
            break;
            }
        }
    else
        {
        // caching mode configuration entry not found
        iCachingMode = EMetadataIfa;
        }
    if (iCachingMode != EWholeFileCaching)
        {
        GetMimeTypeSpecificLimits();
        }

    err = iRsfwConfig->Get(RsfwConfigKeys::KInactivityTimeout,
                           iInactivityTimeout);
    if (err)
        {
        iInactivityTimeout = KDefaultInactivityTimeout;
        }

    CleanupStack::PopAndDestroy(confItem);

    iMountStore = CRsfwMountStore::NewL(NULL);

    RProperty::Define(KRfeServerSecureUid,
                      ERsfwPSKeyConnect,
                      RProperty::EByteArray,
                      KMaxDrives);

    iMountStateProperty.Attach(KRfeServerSecureUid,
                               ERsfwPSKeyConnect);


   // this restores the dormant mounts asynchrously shortly after the server has started
    iDormantMountLoader = CRsfwDormantMountLoader::NewL(this);
    iDormantMountRestorePending = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::~CRsfwVolumeTable
//
// ----------------------------------------------------------------------------
//
CRsfwVolumeTable::~CRsfwVolumeTable()
    {
    // save LRU list to the file
    ExternalizeLRUPriorityList();
    
    TInt i;
    DEBUGSTRING(("Deleting all volumes"));
    for (i = 0; i < KMaxVolumes; i++)
        {
        if (iVolumes[i])
            {
            delete iVolumes[i];
            }
        }
    delete iMountStore;
    iMountStateProperty.Close();
    if (iWaitNoteManager)
    	{
    	delete iWaitNoteManager;
    	}
    if (iDormantMountLoader)
    	{
    	delete iDormantMountLoader;
    	}    	
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::DispatchL
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::DispatchL(TAny* aIp, TAny* aOp)
    {
    TRfeInArgs* ip = reinterpret_cast<TRfeInArgs*>(aIp);
    TRfeOutArgs* op = reinterpret_cast<TRfeOutArgs*>(aOp);
    CRsfwFileEngine* fileEngine = NULL;
    TInt volumeId;

    if(ip->iOpCode == EFsRoot)
        {
        volumeId = iLastVolumeId;
        // boot  - assume that ROOT
        // immediately follows after MountL()
        if (volumeId)
            {
            DEBUGSTRING(("Volume: acquired engine %d", volumeId));
            CRsfwVolume* volume = iVolumes[volumeId];
            if (volume)
                {
                fileEngine = volume->iFileEngine;
                }
            }
        }
    else
        {
        // iFid is always in the same position in the messages
        volumeId = static_cast<TRfeLookupInArgs*>(ip)->iFid.iVolumeId;
        CRsfwVolume* volume = VolumeByVolumeId(volumeId);
        if (volume)
            {
            fileEngine = volume->iFileEngine;
            }
        }
    if (fileEngine)
        {
        fileEngine->DispatchL(*ip, *op);
        }
    else
        {
        DEBUGSTRING(("Volume: no engine for %d", volumeId));
        User::Leave(KErrNotReady);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::VolumeIdByDriveLetter
//
// ----------------------------------------------------------------------------
//
TInt CRsfwVolumeTable::VolumeIdByDriveLetter(TChar aDriveLetter)
    {
    TInt driveNumber;
    iFs.CharToDrive(aDriveLetter, driveNumber);
    if (driveNumber < 0) // note that 0 = EDriveA is allowed
        {
        driveNumber = KErrNotFound;
        }
    return driveNumber;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::VolumeByVolumeId
//
// ----------------------------------------------------------------------------
//
CRsfwVolume* CRsfwVolumeTable::VolumeByVolumeId(TInt aVolumeId)
    {
    if ((aVolumeId >= 0) && (aVolumeId < KMaxVolumes))
        {
        return iVolumes[aVolumeId];
        }
    return NULL;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::VolumeByDriveLetter
//
// ----------------------------------------------------------------------------
//
CRsfwVolume* CRsfwVolumeTable::VolumeByDriveLetter(TChar aDriveLetter)
    {
    return VolumeByVolumeId(VolumeIdByDriveLetter(aDriveLetter));
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::RecoverVolumeL
//
// ----------------------------------------------------------------------------
//
TUint CRsfwVolumeTable::RecoverVolumeL(const TRsfwMountConfig& aMountConfig,
                                   CRsfwMountStateMachine* aCaller)
    {
    DEBUGSTRING(("Recovering volume"));
    TUint transactionId = 0;
    CRsfwVolume* volume = VolumeByVolumeId(aCaller->iVolumeId);
    if (volume)
        {
        aCaller->iVolume = volume;
        // set also fileengine pointer in the state machine,
        // so that the operation can be cancelled
        aCaller->SetFileEngine(volume->iFileEngine);
        if (User::UpperCase(
                aCaller->iVolume->iMountInfo.iMountConfig.iDriveLetter) ==
            User::UpperCase(aMountConfig.iDriveLetter))
            {
            if (aCaller->iVolume->iMountInfo.iMountConfig.iFlags &
                KMountFlagOffLine)
                {
                // We are working offline
                aCaller->iVolume->iMountInfo.iMountStatus.iConnectionState =
                    KMountNotConnected;
                }
            else
                {
                aCaller->iVolume->iMountInfo.iMountStatus.iConnectionState =
                    KMountStronglyConnected;
                }

            transactionId =
                aCaller->iVolume->iFileEngine->RequestConnectionStateL(
                    aCaller->iVolume->iMountInfo.iMountStatus.iConnectionState,
                    aCaller);
            }
        else
            {
            User::Leave(KErrArgument);
            }
        }
    else
        {
        User::Leave(KErrNotFound);
        }

    return transactionId;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::MountState
//
// ----------------------------------------------------------------------------
//
TInt CRsfwVolumeTable::MountState(TChar aDriveLetter)
    {
    DEBUGSTRING(("Getting mount state"));
    TInt state;
    // Find the volume
    CRsfwVolume* volume = VolumeByDriveLetter(aDriveLetter);
    if (volume)
        {
        TRsfwMountInfo& mountInfo = volume->iMountInfo;
        DEBUGSTRING16(("mount '%S'in state %d",
                       &mountInfo.iMountConfig.iName,
                       mountInfo.iMountStatus.iMountState));
        state = mountInfo.iMountStatus.iMountState;
        }
    else
        {
        DEBUGSTRING(("mount not found"));
        state = 0; // define KMountStateNone=0 in RsfwControl.h
        }
    return state;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::GetMountConfigL
//
// ----------------------------------------------------------------------------
//
TInt CRsfwVolumeTable::GetMountConfigL(TRsfwMountConfig& aMountConfig)
    {
    DEBUGSTRING16(("Getting mount config for name '%S', drive %c",
                   &aMountConfig.iName,
                   TUint(aMountConfig.iDriveLetter)));
    TInt err = KErrNone;
    const CRsfwMountEntry* entry =
        iMountStore->LookupEntryByDriveL(aMountConfig.iDriveLetter);
    if (!entry)
        {
        // Could not find by drive letter - retry with name
        entry = iMountStore->LookupEntryByNameL(aMountConfig.iName);
        }
    if (entry &&
        entry->Item(EMountEntryItemDrive) &&
        entry->Item(EMountEntryItemUri))
        {
        if (entry->Item(EMountEntryItemDrive)->Length())
            {
            TLex parse(*entry->Item(EMountEntryItemDrive));
            aMountConfig.iDriveLetter = parse.Get();
            if (entry->Item(EMountEntryItemName))
                {
                aMountConfig.iName.Copy(*entry->Item(EMountEntryItemName));
                }
            else
                {
                aMountConfig.iName.SetLength(0);
                }
            aMountConfig.iUri.Copy(*entry->Item(EMountEntryItemUri));
            if (entry->Item(EMountEntryItemUserName))
                {
                aMountConfig.iUserName.Copy(
                    *entry->Item(EMountEntryItemUserName));
                }
            else
                {
                aMountConfig.iUserName.SetLength(0);
                }

            if (entry->Item(EMountEntryItemPassword))
                {
                aMountConfig.iPassword.Copy(
                    *entry->Item(EMountEntryItemPassword));
                }
            else
                {
                aMountConfig.iPassword.SetLength(0);
                }

            if (entry->Item(EMountEntryItemIap))
                {
                aMountConfig.iAuxData.Copy(
                    *entry->Item(EMountEntryItemIap));
                }
            else
                {
                aMountConfig.iAuxData.SetLength(0);
                }

            TInt inactivityTimeout = iInactivityTimeout;
            if (entry->Item(EMountEntryItemInactivityTimeout))
                {
                TLex timeout(*entry->Item(EMountEntryItemInactivityTimeout));
                timeout.Val(inactivityTimeout);
                DEBUGSTRING(("Set inactivity timeout = %d",
                             inactivityTimeout));
                }
            aMountConfig.iInactivityTimeout = inactivityTimeout;

            aMountConfig.iFlags = 0;
            }
        else
            {
            err = KErrArgument;
            }
        }
    else
        {
        DEBUGSTRING(("mount configuration not found"));
        err = KErrNotFound;
        }

    return err;
    }


// ----------------------------------------------------------------------------
// CRsfwVolumeTable::RestoreVolumesL
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::RestoreVolumesL()
    {
    DEBUGSTRING(("CRsfwVolumeTable::RestoreVolumesL"));
    // Load persistently stored volumes
    CDir* dirList;
    TInt err = iFs.GetDir(*iCacheRoot,
                         KEntryAttMaskSupported,
                         ESortByName,
                         dirList);
    CleanupStack::PushL(dirList);
    DEBUGSTRING(("GetDir for cacheRoot returned %d", err));
    if (err == KErrNone)
        {
        TInt i;
        for (i = 0; i < dirList->Count(); i++)
            {
            const TEntry& entry = (*dirList)[i];
            if (entry.iAtt & KEntryAttDir)
                {
                // The name of cache directories are C<volume_id>
                _LIT(KCacheMatch, "C*");
                if (entry.iName.Match(KCacheMatch) == 0)
                    {
                    TLex volumeAlpha(entry.iName.Mid(1));
                    TInt volumeId;
                    err = volumeAlpha.Val(volumeId);
                    if ((err == KErrNone) &&
                        (volumeId >= 0) &&
                        (volumeId < KMaxVolumes))
                        {
                        TRsfwMountConfig* mountConfig = new (ELeave) TRsfwMountConfig;
                        CleanupStack::PushL(mountConfig);
                        HBufC* metaDataPath = HBufC::NewLC(KMaxPath);
                        TPtr metaDataPathPtr = metaDataPath->Des();
                        metaDataPathPtr.Copy(*iCacheRoot);
                        metaDataPathPtr.Append(entry.iName);
                        metaDataPathPtr.Append('\\');
                        metaDataPathPtr.Append(KMetaDataFileName);
                        CRsfwMetaDataStore* metaDataStore =
                            CRsfwMetaDataStore::NewLC(metaDataPathPtr);
                        // Get the mount configuration info from the
                        // persistent store
                        TRAP(err,
                             metaDataStore->GetMountConfigL(*mountConfig));
                        // metaDataStore, metaDataPath
                        CleanupStack::PopAndDestroy(2);
                        if (err == KErrNone)
                            {
                            DEBUGSTRING16(("Restoring '%S' as volume %d'",
                                           &mountConfig->iUri,
                                           volumeId));
                            TRAP_IGNORE(MountDormantL(*mountConfig,
                                                      volumeId));
                            // In case of error, we should clear the cache
                            }
                        CleanupStack::PopAndDestroy(mountConfig);
                        }
                    }
                }
            }
        }
    CleanupStack::PopAndDestroy(dirList); // dirList
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::DismountByVolumeId
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::DismountByVolumeIdL(TInt aVolumeId,
                                       TBool aDiscardPermanentData)
    {
    DEBUGSTRING(("Dismounting volume %d", aVolumeId));
    CRsfwVolume* volume = VolumeByVolumeId(aVolumeId);
    if (volume)
        {
        if (aDiscardPermanentData)
            {
            // Delete also the meta data
            CRsfwFileEngine* fileEngine = volume->iFileEngine;
            if (fileEngine)
                {
                // Clear cache of files and meta data
                fileEngine->SetPermanenceL(EFalse);
                if (!volume->iMountInfo.iMountStatus.iPermanence)
                    {
                    // There was no change in the above -
                    // so, we have to clear the cache explicitly
                    fileEngine->iFileTable->SetupCacheL();
                    }
                }
            }
        delete volume;
        iVolumes[aVolumeId] = NULL;
        }
    else
        {
        User::Leave(KErrNotFound);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::DismountByDriveLetter
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::DismountByDriveLetterL(TChar aDriveLetter,
                                          TBool aDiscardPermanentData)
    {
    DEBUGSTRING(("Dismounting drive %c", TUint(aDriveLetter)));
    CRsfwVolume* volume = VolumeByDriveLetter(aDriveLetter);
    if (volume)
        {
        DismountByVolumeIdL(volume->iMountInfo.iMountStatus.iVolumeId,
                            aDiscardPermanentData);
        }
    else
        {
        User::Leave(KErrNotFound);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::GetMountList
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::GetMountList(TDriveList& aMountList)
    {
    aMountList.Zero();
    TInt i;
    for (i = 0; i < KMaxVolumes; i++)
        {
        if (iVolumes[i])
            {
            aMountList.Append(iVolumes[i]->
                              iMountInfo.iMountConfig.iDriveLetter);
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::GetMountInfo
//
// ----------------------------------------------------------------------------
//
TInt CRsfwVolumeTable::GetMountInfo(TRsfwMountInfo& aMountInfo)
    {
    CRsfwVolume* volume =
        VolumeByDriveLetter(aMountInfo.iMountConfig.iDriveLetter);
    if (volume)
        {
        volume->GetMountInfo(aMountInfo);
        return KErrNone;
        }
    return KErrNotFound;
    }


// ----------------------------------------------------------------------------
// CRsfwVolumeTable::GetMimeTypeSpecificLimits
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::GetMimeTypeSpecificLimits()
    {
    TInt err;
    err = iRsfwConfig->Get(RsfwConfigKeys::KImgJpegLimit, iImageJpegLimit);
    if (err != KErrNone)
        {
        iImageJpegLimit = KDefaultJpegLimit;
        }

    err = iRsfwConfig->Get(RsfwConfigKeys::KAudMpegLimit, iAudioMpegLimit);
    if (err)
        {
        iAudioMpegLimit = KDefaultMpegLimit;
        }
    }



// ----------------------------------------------------------------------------
// CRsfwVolumeTable::EnsureCacheCanBeAddedL
//
// ----------------------------------------------------------------------------
//
TBool CRsfwVolumeTable::EnsureCacheCanBeAddedL(TInt aBytes)
    {
    DEBUGSTRING(("CACHE MANAGER: ensure that %d bytes can be added to cache",
                 aBytes));
    DEBUGSTRING(("CACHE MANAGER: total cached size is currently %d",
                 TotalCachedSize()));
    DEBUGSTRING(("CACHE MANAGER: max cache size is %d", iMaxCacheSize));
 

    // FIRST: Is there enough space on the cache drive to store the new data
    TInt cacheDrive = CRsfwRfeServer::Env()->iCacheDrive;
    TBool isDiskFull = SysUtil::DiskSpaceBelowCriticalLevelL(&iFs,
                                                             aBytes,
                                                             cacheDrive);
    if ( isDiskFull )
        {
        // check whether clearing cache may help at all
        if ( aBytes > TotalCachedSize() )
            {
            isDiskFull = SysUtil::DiskSpaceBelowCriticalLevelL(&iFs,
                                                               aBytes - TotalCachedSize(),
                                                               cacheDrive);
            if ( isDiskFull )
                {
                DEBUGSTRING(("CACHE MANAGER: no space on disk"));
                return EFalse;
                }
            }
        // seems that clearing cache may help
        else
            {
            DEBUGSTRING(("CACHE MANAGER: running out of disk space, attempting to purge some bytes from cache"));
            do
                {
                CRsfwFileEntry* victim = iLRUPriorityList.GetAndRemoveFirstEntry();

                if (!victim)
                    {
                    DEBUGSTRING(("CACHE MANAGER: nothing to delete!!!"));
                    return EFalse; // cannot clear enough cache space
                    }

                DEBUGSTRING(("CACHE MANAGER: removing fid %d from the cache ",
                             victim->Fid().iNodeId));

                TDesC* cacheNamep = victim->CacheFileName();
                User::LeaveIfError(iFs.Delete(*cacheNamep));

                victim->SetCached(EFalse);
                victim->iCachedSize = 0;
                victim->Parent()->SetLocallyDirty();


                isDiskFull = SysUtil::DiskSpaceBelowCriticalLevelL(&iFs,
                                                                   aBytes,
                                                                   cacheDrive);
                }
            while ( isDiskFull );
            }
        }

    // SECOND: is there enough space in the cache
    if (TotalCachedSize() + aBytes > iMaxCacheSize)
        {
        TInt bytesDeleted = 0;
        TInt bytesToBeDeleted = TotalCachedSize() + aBytes - iMaxCacheSize;
        DEBUGSTRING(("CACHE MANAGER: attempting to purge %d bytes from cache",
                     bytesToBeDeleted));

        while (bytesDeleted < bytesToBeDeleted)
            {
            CRsfwFileEntry* victim = iLRUPriorityList.GetAndRemoveFirstEntry();

            if (!victim)
                {
                DEBUGSTRING(("CACHE MANAGER: XXXX: nothing to delete!!!"));
                return EFalse; // cannot clear enough cache space
                }

            DEBUGSTRING(("CACHE MANAGER: removing fid %d from the cache ",
                         victim->Fid().iNodeId));

            TDesC* cacheNamep = victim->CacheFileName();
            TInt victimSize = victim->iCachedSize;
            User::LeaveIfError(iFs.Delete(*cacheNamep));

            victim->SetCached(EFalse);
            victim->iCachedSize = 0;
            victim->Parent()->SetLocallyDirty();

            bytesDeleted = bytesDeleted + victimSize;
            }
        }

    return ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::EnsureMetadataCanBeAddedL
// This function is called at any time a new CRsfwFileEntry object is about to 
// be created in memory.
// However we want to prevent the scenario in which parent entry is deleted 
// just before its kid creation.
// That's why function takes as a parameter a parent entry for the entry 
// that is about to be created.
// ----------------------------------------------------------------------------
//
TBool CRsfwVolumeTable::EnsureMetadataCanBeAddedL(CRsfwFileEntry* aParent)
    {
    DEBUGSTRING(("memory cap: number of entries %d, entry limit: %d",
                 TotalEntryCount(), iMaxEntryCount));

    TBool parentFound = EFalse;

    while ( TotalEntryCount() >= iMaxEntryCount )
    {
        CRsfwFileEntry* victim = iMetadataLRUPriorityList.GetAndRemoveFirstEntry();

        // if no entries to delete on metadata LRU list try to remove item from file cache LRU list
        if (!victim)
            {
            victim = iLRUPriorityList.GetAndRemoveFirstEntry();

            if (!victim)
                {
                DEBUGSTRING(("memory cap: nothing to delete!!!"));
                return EFalse; // no posibility to add new entry
                }
            }

        // don't touch the root items
        if ( IsRoot(victim) )
            {
            continue;
            }
        
        // if we've found the parent don't touch it, just find the other victim ...
        if (victim && aParent && victim == aParent)
            {
            DEBUGSTRING(("<<<<< SAVED THE PARENT!!!! >>>>"));
            parentFound = ETrue;
            continue;
            }


        // destroy the item
        DEBUGSTRING(("memory cap: removing fid %d from memory",
                         victim->Fid().iNodeId));
        // victim's parent metadata will become out-of-date if we remove victim from memory
        if ( victim->Parent() )
            {
            victim->Parent()->iUseCachedData = EFalse;    
            }

        victim->DropLD();
    }
    
    // put the parent back to the list if it was removed from the list.
    // the reason is that at this point we are not sure whether child will be in fact created.
    // for now we were only interested in preventing the parent from being deleted, nothing more
    // as soon as the child is created, the parent will be removed from the list anyway
    if (parentFound)
        {
        iMetadataLRUPriorityList.AddNodeL(aParent, ECachePriorityNormal);
        }
    
    return ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::IsRoot
// Checks whether given entry is a root of some file table
// ----------------------------------------------------------------------------
//
TBool CRsfwVolumeTable::IsRoot(const CRsfwFileEntry* aEntry)
    {
    if (!aEntry)
        {
        return EFalse;
        }
        
    TInt i;
    for ( i = 0; i < KMaxVolumes; i++ )
        {
        CRsfwVolume* volume;
        volume = iVolumes[i];
        if ( volume && aEntry == volume->iFileEngine->iFileTable->Root())
            {
            return ETrue;
            }
        }
    return EFalse;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::TotalCachedSize
//
// ----------------------------------------------------------------------------
//
TInt CRsfwVolumeTable::TotalCachedSize()
    {
    TInt totalSize = 0;
    CRsfwVolume* volume;
    TInt i = 0;
    while (i < KMaxVolumes)
        {
        volume = iVolumes[i];
        if (volume)
            {
            TInt newSize = volume->iFileEngine->iFileTable->TotalCachedSize();
            totalSize += newSize;
            }
        i++;
        }
    return totalSize;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::TotalEntryCount
//
// ----------------------------------------------------------------------------
//
TInt CRsfwVolumeTable::TotalEntryCount()
    {
    TInt totalCount = 0;
    CRsfwVolume* volume;
    for ( TInt i = 0; i < KMaxVolumes; i++ )
        {
        volume = iVolumes[i];
        if (volume)
            {
            TInt volumeCount = volume->iFileEngine->iFileTable->TotalEntryCount();
            totalCount += volumeCount;
            }
        }
    return totalCount;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::AddToLRUPriorityListL
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::AddToLRUPriorityListL(CRsfwFileEntry *aFe, TInt aPriority)
    {
    DEBUGSTRING(("CACHE MANAGER: adding fid '%d' to the LRU cache",
                 aFe->Fid().iNodeId));
    iLRUPriorityList.AddNodeL(aFe, aPriority);
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::RemoveFromLRUPriorityList
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::RemoveFromLRUPriorityList(CRsfwFileEntry *aFe)
    {
    DEBUGSTRING(("CACHE MANAGER: removing fid '%d' from the LRU cache",
                  aFe->Fid().iNodeId));
    iLRUPriorityList.RemoveNode(aFe);
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::AddToMetadataLRUPriorityListL
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::AddToMetadataLRUPriorityListL(CRsfwFileEntry *aFe, TInt aPriority)
    {
    DEBUGSTRING(("memory cap: adding fid '%d' to the metadata LRU list",
                 aFe->Fid().iNodeId));
    iMetadataLRUPriorityList.AddNodeL(aFe, aPriority);
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::RemoveFromMetadataLRUPriorityList
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::RemoveFromMetadataLRUPriorityList(CRsfwFileEntry *aFe)
    {
    DEBUGSTRING(("CRsfwVolumeTable::RemoveFromMetadataLRUPriorityList"));
    iMetadataLRUPriorityList.RemoveNode(aFe);
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::MoveToTheBackOfMetadataLRUPriorityList
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::MoveToTheBackOfMetadataLRUPriorityListL(CRsfwFileEntry *aFe)
    {
    // just try to remove the entry from the list and if it was on the list then append it again
    if ( iMetadataLRUPriorityList.RemoveNode(aFe) == KErrNone )
        {
        DEBUGSTRING(("memory cap: moving fid '%d' to the back of metadata LRU list",
                     aFe->Fid().iNodeId));
        iMetadataLRUPriorityList.AddNodeL(aFe, ECachePriorityNormal);
        }
    }



// ----------------------------------------------------------------------------
// CRsfwVolumeTable::CheckAndAddProcessStartMarker
//
// ----------------------------------------------------------------------------
//
TBool CRsfwVolumeTable::CheckAndAddProcessStartMarker()
    {
    DEBUGSTRING(("CRsfwVolumeTable::CheckAndAddProcessStartMarker"));
    TFileName path;
    path.Copy(*iCacheRoot);
    path.Append(KRsfwRestorePendingMark);
    
    if (BaflUtils::FileExists(iFs, path))
        {
        // file already exists, the previous attempt to restore metadata must have failed
        DEBUGSTRING(("returning EFalse; file already exists"));
        return EFalse;
        }
    else 
        {
        // create an empty file
        TInt err;
        RFile markerFile;
        err = markerFile.Create(iFs, path, EFileWrite);
        if (err) 
            {
            return EFalse;
            }
        else 
            {
            DEBUGSTRING(("returning ETrue; file created"));
            markerFile.Close();
            return ETrue;
            }
 
        }
    }
    
// ----------------------------------------------------------------------------
// CRsfwVolumeTable::DeleteTheMarker
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::DeleteTheMarker()
    {    
    TFileName path;
    path.Copy(*iCacheRoot);
    path.Append(KRsfwRestorePendingMark);
    
    // ignore the error
    // if this fails for some reason, lets allow the file to be there
    // as "something" must be wrong
    iFs.Delete(path);
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::CleanupCorrutedCacheL
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::CleanupCorruptedCacheL()
    {    
    // delete everything from the cache
    TFileName cachepath;
    cachepath.Copy(*iCacheRoot);
    CFileMan* fileMan = CFileMan::NewL(iFs);
    fileMan->Delete(cachepath, CFileMan::ERecurse);
    delete fileMan;
    }


// ----------------------------------------------------------------------------
// CRsfwVolumeTable::WillExternalizedLRUPriorityListBeUsed
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::WillLRUPriorityListBeInternalized()
    {
    DEBUGSTRING(("CRsfwVolumeTable::WillLRUPriorityListBeInternalized"));
    iUseExternalizedLRUList = EFalse;
    // check whether the file with externalized data exists
    TFileName path;
    path.Copy(*iCacheRoot);
    path.Append(KRsfwLruFileName);

    if (BaflUtils::FileExists(iFs, path))
        {
        iUseExternalizedLRUList = ETrue;
        }
    DEBUGSTRING(("...set to %d", iUseExternalizedLRUList)); 
    }


// ----------------------------------------------------------------------------
// CRsfwVolumeTable::ExternalizeLRUPriorityList
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::ExternalizeLRUPriorityList()
    {
    TRAPD(err, ExternalizeLRUPriorityListL());
    if (err)
        {
        DEBUGSTRING(("Externalizing LRU priority list failed!"));
        }
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::ExternalizeLRUPriorityList
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::ExternalizeLRUPriorityListL()
    {        
    // prepare temp path
    _LIT(KRsfwLruTempFileName, "lru.temp");
    TFileName tempPath;
    tempPath.Copy(*iCacheRoot);
    tempPath.Append(KRsfwLruTempFileName);
    
    // create temp file
    RFile file;
    CleanupClosePushL(file);
    User::LeaveIfError(file.Replace(iFs, tempPath, EFileShareAny | EFileWrite));

    // associate stream
    RFileWriteStream stream(file);
    CleanupClosePushL(stream);

    // externalize
    iLRUPriorityList.ExternalizeL(stream);
    stream.CommitL();

    // cleanup
    CleanupStack::PopAndDestroy(2); // stream, file        

    // everything went ok -> rename lru.temp into lru.dat
    TFileName path;
    path.Copy(*iCacheRoot);
    path.Append(KRsfwLruFileName);
    CFileMan* fm = CFileMan::NewL(iFs);
    CleanupStack::PushL(fm);
    TInt err = fm->Rename(tempPath, path, CFileMan::EOverWrite);
    if (err)
        {
        fm->Delete(tempPath);
        fm->Delete(path);
        User::Leave(err);
        }
    CleanupStack::PopAndDestroy(fm);
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::InternalizeLRUPriorityList
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::InternalizeLRUPriorityListL()
    {
    if (!iUseExternalizedLRUList)
        {
        // it means LRU has been already populated when loading metadata
        // so nothing to do here
        return;
        }
    // prepare path
    TFileName path;
    path.Copy(*iCacheRoot);
    path.Append(KRsfwLruFileName);
    
    // open file
    RFile file;
    TInt err = file.Open(iFs, path, EFileShareAny | EFileRead);
    if ( err == KErrNone )
        {
        CleanupClosePushL(file);

        // associate stream
        RFileReadStream stream(file);
        CleanupClosePushL(stream);

        // internalize
        TRAP(err, iLRUPriorityList.InternalizeL(stream, this));

        // cleanup
        CleanupStack::PopAndDestroy(2); // stream, file        
        }

    DEBUGSTRING(("InternalizeLRUPriorityListL: status %d", err));
        
    // once internalizing is done, the file is not needed anymore 
    // ignore the result
    iFs.Delete(path);
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::OperationCompleted()
// This function may shut down RFE.
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::OperationCompleted(CRsfwVolume* /* aVolume */)
    {
    DEBUGSTRING(("Volume operation completed"));
    // Shut down the whole server if all remaining mounts are dormant
    // and we are not still restoring dormant mounts
    if ((iAllEnginesIdle) && (!iDormantMountRestorePending))
        {
        iRfeServer->AllEnginesIdling(KRsfwDormantShutdownTimeout);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::VolumeStateChanged()
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::VolumeStateChanged(CRsfwVolume* aVolume)
    {
    DEBUGSTRING(("Volume state changed"));

    TBool allEnginesIdle = ETrue;
    TInt i = 0;
    do
        {
        if (iVolumes[i])
            {
            if (!IsMountIdle(iVolumes[i]->iMountInfo.iMountStatus))
                {
                // Do not shut down if there are connected mounts
                allEnginesIdle = EFalse;
                }
            }
        } while ((++i < KMaxVolumes) && allEnginesIdle);

    // one more thing is to check the current volume since 
    // if the drive was newly mounted, it will not be found from the volume table 
    if (allEnginesIdle)
        {
        TInt driveNumber = VolumeIdByDriveLetter(aVolume->MountInfo()->iMountConfig.iDriveLetter);     
        if ((driveNumber != KErrNotFound) && 
            (!IsMountIdle(aVolume->MountInfo()->iMountStatus)))
            {
            allEnginesIdle = EFalse;
            }         
        }

    DEBUGSTRING(("All engines idle = %d", allEnginesIdle));
    iAllEnginesIdle = allEnginesIdle;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::PublishConnectionStatus()
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::PublishConnectionStatus(CRsfwVolume* aVolume)
    {
    DEBUGSTRING(("Publishing connection status:"));
    TDriveList driveList;
    driveList.FillZ(driveList.MaxLength());
    TInt i;
    // (at least) record the state of the volume received as a parameter
    // (if the drive was newly mounted, it will not be found from the volume table 
    TInt driveNumber = VolumeIdByDriveLetter(aVolume->MountInfo()->iMountConfig.iDriveLetter);     
    if ((driveNumber != KErrNotFound) && 
       (aVolume->MountInfo()->iMountStatus.iMountState != KMountStateDormant))
        {
        DEBUGSTRING(("- connected: %c", TUint(aVolume->MountInfo()->iMountConfig.iDriveLetter)));
        driveList[driveNumber] = 1;
        }    
    
    // for convenience, record the states of other volumes too from the volume table
    for (i = 0; i < KMaxVolumes; i++)
        {
        if (iVolumes[i])
            {
            TRsfwMountInfo& mountInfo = iVolumes[i]->iMountInfo;
            if (mountInfo.iMountStatus.iMountState != KMountStateDormant)
                {
                driveNumber =
                    VolumeIdByDriveLetter(mountInfo.iMountConfig.iDriveLetter);
                if (driveNumber != KErrNotFound)
                    {
                    DEBUGSTRING(("- connected: %c",
                                 TUint(mountInfo.iMountConfig.iDriveLetter)));
                    driveList[driveNumber] = 1;
                    }
                }
            }
        }
          
    iMountStateProperty.Set(driveList);
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::WaitNoteManager()
//
// ----------------------------------------------------------------------------
//
CRsfwWaitNoteManager* CRsfwVolumeTable::WaitNoteManager()
	{
	return iWaitNoteManager;
	}

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::IsCachedDataStillValid()
//
// ----------------------------------------------------------------------------
//
TBool CRsfwVolumeTable::IsCachedDataStillValid(TTime aCachedTime)
	{
	return IsCacheStillValid(aCachedTime,
						TTimeIntervalSeconds(iFileCacheTimeout));
	}

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::IsCachedAttrStillValid()
//
// ----------------------------------------------------------------------------
//
TBool CRsfwVolumeTable::IsCachedAttrStillValid(TTime aCachedTime)
	{
	return IsCacheStillValid(aCachedTime,
						TTimeIntervalSeconds(iDirCacheTimeout));
	}



// ----------------------------------------------------------------------------
// CRsfwVolumeTable::MountDormantL
//
// ----------------------------------------------------------------------------
//
void CRsfwVolumeTable::MountDormantL(const TRsfwMountConfig& aMountConfig,
                                 TInt aVolumeId)
    {
    // Bind a volume id to a file engine
    DEBUGSTRING16(("Restoring drive '%c' with uri '%S' and flags 0x%x",
                   TUint(aMountConfig.iDriveLetter),
                   &aMountConfig.iUri,
                   aMountConfig.iFlags));

    // Create a file engine for the volume
    CRsfwVolume* volume = new (ELeave) CRsfwVolume();
    CleanupStack::PushL(volume);
    volume->iMountInfo.iMountConfig = aMountConfig;
    volume->iMountInfo.iMountStatus.iVolumeId = aVolumeId;
    volume->iVolumeTable = this;
    volume->iMountInfo.iMountStatus.iPermanence = iPermanence;
    // We are working offline
    volume->iMountInfo.iMountStatus.iMountState = KMountStateDormant;
    volume->iMountInfo.iMountStatus.iConnectionState = KMountNotConnected;
    CRsfwFileEngine* fileEngine = CRsfwFileEngine::NewL(volume);
    volume->iFileEngine = fileEngine;
    delete iVolumes[aVolumeId];
    iVolumes[aVolumeId] = volume;
    CleanupStack::Pop(volume);
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::IsCachedAttrStillValid()
//
// ----------------------------------------------------------------------------
//
TBool CRsfwVolumeTable::IsCacheStillValid(TTime aCachedTime,
									  TTimeIntervalSeconds aValidity)
	{
	TTime now;
	TTime comp;

	now.UniversalTime();
	comp = now - aValidity;

    if (comp >= aCachedTime)
        {
        return EFalse;
        }
    else
        {
        return ETrue;
        }
	}
	
// ----------------------------------------------------------------------------
// CRsfwVolumeTable::PurgeFromCache()
//
// ----------------------------------------------------------------------------
//	
TInt CRsfwVolumeTable::PurgeFromCache(TDesC& aCachePath) 
    {
    // get the volume id for this path
    TParse parser;
    parser.Set(aCachePath, NULL, NULL);
    if (!(parser.DrivePresent())) 
        {
        return KErrArgument;
        }
    TPtrC drive = parser.Drive();
    CRsfwVolume* volume = VolumeByDriveLetter(drive[0]);
    if (!volume) 
        {
        return KErrNotFound;
        }
    
    if (!(parser.PathPresent())) 
        {
        return KErrArgument;
        }
            
    if (parser.NamePresent()) 
        {
        // this is a file
        return KErrArgument;
        }
    return volume->iFileEngine->PurgeFromCache(parser.Path());
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::CancelTransfer()
//
// ----------------------------------------------------------------------------
//	
TInt CRsfwVolumeTable::CancelTransferL(TDesC& aFilePath) 
    {
    DEBUGSTRING16(("CRsfwVolumeTable::CancelTransferL for %S", &aFilePath));
      // get the volume id for this path
    TParse parser;
    parser.Set(aFilePath, NULL, NULL);
    if (!(parser.DrivePresent())) 
        {
        return KErrArgument;
        }
    TPtrC drive = parser.Drive();
    CRsfwVolume* volume = VolumeByDriveLetter(drive[0]);
    if (!volume) 
        {
        return KErrNotFound;
        }
              
    if (!(parser.NamePresent())) 
        {
        // this is not a file
        return KErrArgument;
        }
       
    
    // mark the file entry as "cancelled"
    TPtrC pathPtr  = aFilePath.Right(aFilePath.Length() - 2); //drop the drive letter
    CRsfwFileEntry* targetFid = volume->iFileEngine->FetchFep(pathPtr);
    if (targetFid)
        {
        DEBUGSTRING(("setting KNodeWritingCancelled for fid %d", targetFid->Fid().iNodeId));
        targetFid->SetFlags(KNodeWritingCancelled);
        }
        
    volume->iFileEngine->CancelTransactionL(aFilePath);

    return KErrNone;
    }

// ----------------------------------------------------------------------------
// CRsfwVolumeTable::IsMountIdle()
//
// ----------------------------------------------------------------------------
//	
TBool CRsfwVolumeTable::IsMountIdle(TRsfwMountStatus& aMountStatus) 
    {
    if (aMountStatus.iMountState != KMountStateDormant
        || aMountStatus.iConnectionState == KMountConnecting)
        {
        return EFalse;
        }
    else
        {
        return ETrue;
        }
    }
