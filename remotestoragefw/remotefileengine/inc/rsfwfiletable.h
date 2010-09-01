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
* Description:  data struct for remote files metadata
*
*/


#ifndef C_RSFWFILETABLE_H
#define C_RSFWFILETABLE_H

#include <e32base.h>
#include <f32file.h>
#include "rsfwmetadatastore.h"

class CRsfwVolume;
class CRsfwFileEntry;
class TFid;
class TMetaDataEvent;

/**  file where permanent metadata is stored */
_LIT(KMetaDataFileName, "M.dat");

/**  an entry has been added */
const TInt KNotifyNodeAdded    = 1;  

/** an entry has been modified  */
const TInt KNotifyNodeModified = 2; 

/**  an entry has been deleted */
const TInt KNotifyNodeRemoved  = 3; 
 
/**  size of the buffer used when writing file entry to permanent metadata */
const TInt KMaxExternalizedFileEntrySize = 1024;

/**  internal state bits */

/** local changes (eg caching bits) */
const TUint KNodeLocallyDirty      = 0x0001; 
/** remote changes */
const TUint KNodeRemotelyDirty     = 0x0002; 
/** temp mark for various purposes */
const TUint KNodeMarked            = 0x0004; 

const TUint KNodeHasValidLock      = 0x0008;
const TUint KNodeOpenedForWriting  = 0x0010;

const TUint KNodeWritingCancelled  = 0x0020;

const TUint KNodePartlyCached      = 0x0100;

/** indicates that a file that has been
opened has been created (instead of opening
an already existing file). This affects
logic when closing the file (PUT + RENAME
trick to protect the existing file in case  of
cancelled PUT is not used)
 **/
const TUint KNodeNewlyCreated      = 0x0200;


class TMetaDataEvent
    {
public:
    TInt        iEvent;    // notified event
    TInt        iNodeId;   // node id
    CRsfwFileEntry* iEntry;    // file entry
    };


class CRsfwFileTable: public CBase
    {
private:
    enum TMetaDataState
        {
        EMetaDataSaveNone = 0,  // meta data saving has not started yet
        EMetaDataSaveStarted,   // meta data saving has been started
        EMetaDataSaveFailed     // meta data saving has failed
        };

    class TMetaDataSlot
        {
    public:
        TInt        iNodeId;       // node id
        TInt        iSlotId;       // slot index
        };

public: 
    static CRsfwFileTable* NewL(CRsfwVolume* aVolume, TFileName& aCachePath);

    inline CRsfwVolume* Volume();
    inline CRsfwFileEntry* Root();

    ~CRsfwFileTable();
    
    // add node to three
    void AddL(CRsfwFileEntry* aFep);
    
    // remove node from thee
    void RemoveL(CRsfwFileEntry* aFep);
    
    // lookup three
    CRsfwFileEntry *Lookup(const TFid& aFid);
    void DumpL(TBool aAll);
    inline const TBool Permanence() const;
    void SetPermanenceL(TBool aPermanence);
    void HandleMetaDataEvent(TInt aEvent, CRsfwFileEntry* aFep);
    CRsfwFileEntry* LoadMetaDataL();
    TInt SaveMetaDataDelta();
    void SetupCacheL();
    TInt TotalCachedSize();
    TInt TotalEntryCount();
    inline TInt OpenFileCount();
    inline void UpdateOpenFileCount(TInt aDelta);
    void ResolveDirtyFilesL();
    void ResolveDirtyFileL(CRsfwFileEntry* aFep);


private:
    void ConstructL(CRsfwVolume* aVolume, TFileName& aCachePath);
    void ClearCacheL();
    void ConstructL(RReadStream& aStream);
    TMetaDataEvent* NodeEvent(TInt aNodeId);
    void AddEvent(TInt aEvent, CRsfwFileEntry* aFep);
    void RemoveEvent(TInt aNodeId);
    void LoadNodeL(CRsfwFileEntry*& aFep, TInt &aSlot);
    void SaveNodeL(CRsfwFileEntry* aFep, TInt &aSlot);
    void SaveMetaDataDeltaL();

private:
    RFs           iFs;                      // inherited from RFE environment
    TInt          iNodeId;                  // next node id to assign
    CRsfwVolume*      iVolume;                  // volume info
    CRsfwFileEntry*   iRootFep;                 // root file entry
    TFileName     iCachePath;               // cache directory path
    TBool         iPermanence;              // permanent meta data
    RArray<TMetaDataEvent> iMetaDataEvents; // dirty entries since last flush
    CRsfwMetaDataStore* iMetaDataStore;         // permanent store for metadata
    RArray<TMetaDataSlot> iMetaDataSlots;   // maps file slot to node ids
    TFileName     iMetaDataFilePath;        // meta data file path
    TInt          iMetaDataSaveState;       // the state of meta data saving
    CRsfwFileEntry*   iCurrentParent;           // parent of last looked up entry
    TInt          iOpenFileCount;           // count of open files
    };

#include "rsfwfiletable.inl"

#endif