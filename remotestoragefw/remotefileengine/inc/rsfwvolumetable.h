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
* Description:  data struct for all volumes
*
*/

#ifndef C_RSFWVOLUMETABLE_H
#define C_RSFWVOLUMETABLE_H

#include <e32property.h>
#include <f32file.h>

#include "rsfwlruprioritylist.h"

class CRsfwRfeServer;
class CRsfwConfig;
class TRsfwMountInfo;
class TRsfwMountConfig;
class TRsfwMountStatus;
class CRsfwVolume;
class TTime;
class CRsfwMountStateMachine;
class CRsfwWaitNoteManager;
class CRsfwMountStore;
class CRsfwDormantMountLoader;

/** caching mode */
enum TCachingMode 
    {
    EWholeFileCaching,
    EMetadataIfa,
    EFullIfa
    };

/** theoretical maximum number of volumes - A to Z */    
const TInt KMaxVolumes = 26; 


/** Shutdown after all mounts have become dormant/disconnected */
const TInt KRsfwDormantShutdownTimeout = 120;

class CRsfwVolumeTable: public CBase
    {
public:
    static CRsfwVolumeTable* NewL(CRsfwRfeServer* aRfeServer,
                              CRsfwConfig* aRsfwConfig);
    ~CRsfwVolumeTable();

    void DispatchL(TAny* aIp, TAny* aOp);
    TInt VolumeIdByDriveLetter(TChar aDriveLetter);
    CRsfwVolume* VolumeByVolumeId(TInt aVolumeId);
    CRsfwVolume* VolumeByDriveLetter(TChar aDriveLetter);
    void RestoreDormantMountsL();
    TUint RecoverVolumeL(const TRsfwMountConfig& aMountConfig,
                        CRsfwMountStateMachine* aCaller);
    TInt MountState(TChar aDriveLetter);
    TInt GetMountConfigL(TRsfwMountConfig& aMountConfig);
    void RestoreVolumesL();
    void DismountByVolumeIdL(TInt aVolumeId, TBool aDiscardPermanentData);
    void DismountByDriveLetterL(TChar aDriveLetter,
                                TBool aDiscardPermanentData);
    void GetMountList(TDriveList& aMountList);
    TInt GetMountInfo(TRsfwMountInfo& aMountInfo);
    void GetMimeTypeSpecificLimits();

    // Cache management functions
    TBool EnsureCacheCanBeAddedL(TInt aBytes);
    TBool EnsureMetadataCanBeAddedL(CRsfwFileEntry* aParent);
    TBool IsRoot(const CRsfwFileEntry* aEntry);
    TInt TotalCachedSize();
    TInt TotalEntryCount();
    void AddToLRUPriorityListL(CRsfwFileEntry *aFe, TInt aPriority);
    void RemoveFromLRUPriorityList(CRsfwFileEntry *aFe);
    void AddToMetadataLRUPriorityListL(CRsfwFileEntry *aFe, TInt aPriority);
    void RemoveFromMetadataLRUPriorityList(CRsfwFileEntry *aFe);
    void MoveToTheBackOfMetadataLRUPriorityListL(CRsfwFileEntry *aFe);
    void WillLRUPriorityListBeInternalized();
    TBool CheckAndAddProcessStartMarker();
    void DeleteTheMarker();
    void CleanupCorruptedCacheL();
    void OperationCompleted(CRsfwVolume* aVolume);
    void VolumeStateChanged(CRsfwVolume* aVolume);
    CRsfwWaitNoteManager* WaitNoteManager();    
    TBool IsCachedDataStillValid(TTime aCachedTime);
    TBool IsCachedAttrStillValid(TTime aCachedTime);
    
    // removes from cache all the data for certain path
    // this is "refresh", i.e to ensure that next readdir fetches the data 
    // form server
    TInt PurgeFromCache(TDesC& aCachePath);
    
    TInt CancelTransferL(TDesC& aFilePath);
    
    void MountDormantL(const TRsfwMountConfig& aMountConfig, TInt aVolumeId);

    void PublishConnectionStatus(CRsfwVolume* aVolume);

private:
    void ConstructL(CRsfwRfeServer* aRfeServer, CRsfwConfig* aRsfwConfig);
    TBool IsCacheStillValid(TTime aCachedTime, TTimeIntervalSeconds aValidity);
    void ExternalizeLRUPriorityList();
    void ExternalizeLRUPriorityListL();
    void InternalizeLRUPriorityListL();
    TBool IsMountIdle(TRsfwMountStatus& aMountStatus);

public:
    // configuration parameters read from the configuration file
    TInt iMaxCacheSize;       // maximum allowed cache size (global)
    TInt iMaxEntryCount;      // maximum number of cached entries (global)
   
    TCachingMode iCachingMode;
    TInt iRecognizerLimit;
    TInt iImageJpegLimit;
    TInt iAudioMpegLimit;
    TInt iInactivityTimeout;

public:
    CRsfwVolume* iVolumes[KMaxVolumes];  // alphabet
    TInt iLastVolumeId;
    CRsfwConfig* iRsfwConfig;        // RSC configuration

    TBool iPermanence;               // use permanent meta data
    CRsfwMountStore* iMountStore;        // mount configuration repository
    CRsfwLruPriorityList iLRUPriorityList;
    CRsfwLruPriorityList iMetadataLRUPriorityList;
    CRsfwRfeServer* iRfeServer;
    TBool iUseExternalizedLRUList; // whether to use externalized LRU list data or not
    TBool iDormantMountRestorePending; // true when, after server startup, we start to restore mounts

private:
    CRsfwDormantMountLoader* iDormantMountLoader; // loads dormant mounts asynchronously shortly after server start
 	CRsfwWaitNoteManager* iWaitNoteManager; // Implements handling of global wait notes
	// associated with this class because when we come to Connect(), not much else is available
	
    TBool iAllEnginesIdle;          // are all the engines idle
    RProperty iMountStateProperty;   // property for connection state signaling
    TInt iFileCacheTimeout;   // how long cached files are assumed to be valid
    TInt iDirCacheTimeout;    // how long cached dirs are assumed to be valid
    
    // these are also available from TRfeEnv, but they are used so many times
    // by this class that an own pointer is justified
    RFs iFs; // handle to RFs session, owned by RfeServer
    TFileName* iCacheRoot; // pointer to cacheroot, owned by RfeServer
    };


#endif
