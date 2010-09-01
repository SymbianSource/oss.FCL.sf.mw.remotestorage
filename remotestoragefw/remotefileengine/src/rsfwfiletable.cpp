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
* Description:  metadata struct for remote files
*
*/


#include <s32mem.h>

#include "rsfwfiletable.h"
#include "rsfwfileentry.h"
#include "rsfwfileengine.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwrfeserver.h"
#include "rsfwwaitnotemanager.h"
#include "mdebug.h"


// ----------------------------------------------------------------------------
// CRsfwFileTable::NewL
// ----------------------------------------------------------------------------
//   
CRsfwFileTable* CRsfwFileTable::NewL(CRsfwVolume* aVolume, TFileName& aCachePath)
    {
    CRsfwFileTable* self = new (ELeave) CRsfwFileTable();
    DEBUGSTRING(("CRsfwFileTable: in NewL 0x%x", self));
    CleanupStack::PushL(self);
    self->ConstructL(aVolume, aCachePath);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::ConstructL
// ----------------------------------------------------------------------------
//   
void CRsfwFileTable::ConstructL(CRsfwVolume* aVolume, TFileName& aCachePath)
    {
    iFs = CRsfwRfeServer::Env()->iFs;
    // The root will be number 1
    iNodeId = 1;
    iVolume = aVolume;
    iRootFep = NULL;
    iCachePath.Copy(aCachePath);
    iPermanence = iVolume->iMountInfo.iMountStatus.iPermanence;
    iMetaDataFilePath.Copy(aCachePath);
    iMetaDataFilePath.Append(KMetaDataFileName);
    iMetaDataEvents.Reset();
    SetupCacheL();
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::~CRsfwFileTable
// ----------------------------------------------------------------------------
//   
CRsfwFileTable::~CRsfwFileTable()
    {
    if (iRootFep)
        {
        // Delete the whole tree recursively
        delete iRootFep;
        iRootFep = NULL;
        }
    // Discard events
    iMetaDataEvents.Close();
    iMetaDataSlots.Close();
    if (iMetaDataStore) 
        {
        delete iMetaDataStore;
        }
    }
    
    
// ----------------------------------------------------------------------------
// CRsfwFileTable::AddL
// this function associates aFep with this file table but does no
// yet set to any other node's child (or root node), that must be done sepately
// ----------------------------------------------------------------------------
//   
void CRsfwFileTable::AddL(CRsfwFileEntry* aFep)
    {    
    // Just assign a unique id
    TFid newFid;
    newFid.iVolumeId = iVolume->iMountInfo.iMountStatus.iVolumeId;
    newFid.iNodeId = iNodeId;
    aFep->SetFid(newFid);
    aFep->iFileTable = this;
    iNodeId++;
    if (!iRootFep)
        {
        iRootFep = aFep;
        }
    // add item to metadata LRU list, 
    // only add childless directories and non-cached files
    if ( (aFep->Type() == KNodeTypeFile && aFep->iCachedSize == 0) 
         || (aFep->Type() == KNodeTypeDir && aFep->Kids()->Count() == 0)
         || (aFep->Type() == KNodeTypeUnknown) )
        {
        iVolume->iVolumeTable->AddToMetadataLRUPriorityListL(aFep, ECachePriorityNormal);        
        }
    // Note that the first added entry will always be the root
    HandleMetaDataEvent(KNotifyNodeAdded, aFep);
    }
    
// ----------------------------------------------------------------------------
// CRsfwFileTable::Remove
// removed this fileentry and disconnects from the parent
// does not delete the kid file/directory entries
// in practise this means that if deleting a directory, all its entries must be deleted
// recursively first
// ----------------------------------------------------------------------------
//   
void CRsfwFileTable::RemoveL(CRsfwFileEntry* aFep)
    {
    DEBUGSTRING(("CRsfwFileTable::RemoveL"));
    // remove item from metadata LRU list
    iVolume->iVolumeTable->RemoveFromMetadataLRUPriorityList(aFep);        

    if (aFep == iCurrentParent)
        {
        iCurrentParent = NULL;
        }
    if (iPermanence)
        {
        aFep->RemoveCacheFile();
        HandleMetaDataEvent(KNotifyNodeRemoved, aFep);
        }
        
    // remove this file entry from its parent node    
    if (aFep->iParent)  
        {
        aFep->iParent->RemoveKidL(aFep);
        }
    }
    
// ----------------------------------------------------------------------------
// CRsfwFileTable::Lookup
// ----------------------------------------------------------------------------
//   
CRsfwFileEntry* CRsfwFileTable::Lookup(const TFid& aFid)
    {
    if (!iRootFep)
        {
        return NULL;
        }
    if (iRootFep->Fid().iNodeId == aFid.iNodeId)
        {
        return iRootFep;
        }
    // Try to optimize by starting from the latest parent
    CRsfwFileEntry* fep = NULL;
    if (iCurrentParent)
        {
        fep = iCurrentParent->Lookup(aFid);
        }
    if (!fep)
        {
        fep = iRootFep->Lookup(aFid);
        }
    if (fep)
        {
        iCurrentParent = fep->Parent();
        }
    return fep;
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::DumpL
// ----------------------------------------------------------------------------
// 
#ifdef _DEBUG
void CRsfwFileTable::DumpL(TBool aAll)
    {
    if (iRootFep)
        {
        iRootFep->PrintL(0, ETrue, aAll);
        }
    }
#else
void CRsfwFileTable::DumpL(TBool /* aAll */)
    {
    }
#endif //DEBUG
 
 
// ----------------------------------------------------------------------------
// CRsfwFileTable::SetPermanenceL
// ----------------------------------------------------------------------------
// 
void CRsfwFileTable::SetPermanenceL(TBool aPermanence)
    {
    if (iPermanence != aPermanence)
        {
        iPermanence = aPermanence;
        if (!iPermanence)
            {
            delete iMetaDataStore;
            iMetaDataStore = NULL;
            }
        SetupCacheL();
        }
    }
       
// ----------------------------------------------------------------------------
// CRsfwFileTable::HandleMetaDataEvent
// ----------------------------------------------------------------------------
// 
void CRsfwFileTable::HandleMetaDataEvent(TInt aEvent, CRsfwFileEntry* aFep)
    {
    if (iMetaDataSaveState == EMetaDataSaveFailed)
        {
        // No use
        return;
        }

    switch (aEvent)
        {

    case KNotifyNodeAdded:
        {
        // There should not be any previous additions
        AddEvent(aEvent, aFep);
        }
        break;

    case KNotifyNodeModified:
        {
        // There may appear spurious modifications
        // to removed entries (like cache state set to false).
        // We filter them out.
        if (!NodeEvent(aFep->Fid().iNodeId))
            {
            AddEvent(aEvent, aFep);
            }
        }
        break;

    case KNotifyNodeRemoved:
        {
        TMetaDataEvent* oldEvent = NodeEvent(aFep->Fid().iNodeId);
        if (oldEvent)
            {
            if (oldEvent->iEvent == KNotifyNodeAdded)
                {
                // just remove a previous "added"
                RemoveEvent(oldEvent->iNodeId);
                AddEvent(aEvent, aFep);
                }
            else
                {
                // Just replace "modified" (or duplicate "deleted")
                // with "deleted"
                oldEvent->iEvent = KNotifyNodeRemoved;
                }
            }
        else
            {
            AddEvent(aEvent, aFep);
            }
        }
        break;
        
    default:
        break;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::LoadMetaDataL
// ----------------------------------------------------------------------------
// 
CRsfwFileEntry* CRsfwFileTable::LoadMetaDataL()
    {
    // When this function is called the root node
    // must already be created in the file table
    iMetaDataStore->CompactL();
    iMetaDataStore->ResetL(EFalse);

    RPointerArray<CRsfwFileEntry> feps;
    CleanupClosePushL(feps);

    TBool done = EFalse;
    while (!done)
        {
        CRsfwFileEntry* fep;
        TMetaDataSlot slot;
        TRAPD(err, LoadNodeL(fep, slot.iSlotId));
        if (err == KErrNone)
            {
            if (fep != NULL) 
                {
                feps.Append(fep);
                slot.iNodeId = fep->Fid().iNodeId;
                iMetaDataSlots.Append(slot);
                if (!fep->iParentNodeId)
                    {
                    // This must be the root
                    DEBUGSTRING(("Root found at slot %d",
                             iMetaDataSlots.Count() - 1));
                    iRootFep = fep;
                    }
                }
            }
        else
            {
            // All or nothing ...
            DEBUGSTRING(("LoadNode returned with err = %d", err));
            if (err != KErrEof)
                {
                User::Leave(err);
                }
            done = ETrue;
            }
        }

    // Now we have the restored the file entries
    TInt i;
    for (i = 0; i < feps.Count(); i++)
        {
        CRsfwFileEntry* fep = feps[i];
        // Determine the next free node id
        if (fep->Fid().iNodeId >= iNodeId)
            {
            iNodeId = fep->Fid().iNodeId + 1;
            }

        if (fep->iParentNodeId == 0)
            {
            // This is the root node
            fep->SetParent(NULL);
            }
        else if (fep->iParentNodeId == 1)
            {
            // The parent is the root node
            fep->SetParent(iRootFep);
            iRootFep->iKids.Append(fep);
            }
        else
            {
            TInt j;
            // This is O(n**2)
            for (j = 0; j < feps.Count(); j++)
                {
                if (j != i)
                    {
                    // Find the parent for the node
                    CRsfwFileEntry* parent = feps[j];
                    if (fep->iParentNodeId == parent->Fid().iNodeId)
                        {
                        // Set up the two-way linkage
                        fep->SetParent(parent);
                        parent->iKids.Append(fep);
                        break;
                        }
                    }
                }
            }
        }

    // Final fixes
    for (i = 0; i < feps.Count(); i++)
        {
        CRsfwFileEntry* fep = feps[i];
        // Fix volume ids and such ...
        TFid fid;
        fid = fep->Fid();
        fid.iVolumeId = iVolume->iMountInfo.iMountStatus.iVolumeId;
        fep->SetFid(fid);
        fep->iFileTable = this;
        // Add to LRU list (only cached files)
        if ( fep->Type() == KNodeTypeFile && fep->IsCached() 
            && (!iVolume->iVolumeTable->iUseExternalizedLRUList))
            {
            iVolume->iVolumeTable->AddToLRUPriorityListL(fep, ECachePriorityNormal);
            }
        // add item to metadata LRU list, 
        // only add childless directories and non-cached files
        if ( (fep->Type() == KNodeTypeFile && fep->iCachedSize == 0) 
             || (fep->Type() == KNodeTypeDir && fep->Kids()->Count() == 0) )
            {
            iVolume->iVolumeTable->AddToMetadataLRUPriorityListL(fep, ECachePriorityNormal);        
            }
  
        // Check consistency
        if ((fep != iRootFep) && (!fep->Parent()))
            {
            // Should never happen
            DEBUGSTRING16(("LodaMetaDataL() - parent missing for '%S'",
                           fep->Name()));
            }
        }
        
    // Now we don't need the file entry pointer array any more
    CleanupStack::PopAndDestroy(&feps); // feps
    return iRootFep;
    }
        
// ----------------------------------------------------------------------------
// CRsfwFileTable::SaveMetaDataDelta
// ----------------------------------------------------------------------------
// 
TInt CRsfwFileTable::SaveMetaDataDelta()
    {
    DEBUGSTRING16(("CRsfwFileTable::SaveMetaDataDelta"));
    TRAPD(err, SaveMetaDataDeltaL());
    if (err != KErrNone)
        {
        DEBUGSTRING(("SaveMetaDataDeltaL() returns %d", err));
        // Stop recording meta data
        iMetaDataEvents.Reset();
        iMetaDataSaveState = EMetaDataSaveFailed;
        }
    return err;
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::SetupCacheL
// ----------------------------------------------------------------------------
// 
void CRsfwFileTable::SetupCacheL()
    {
    if (iPermanence)
        {
        iMetaDataStore = CRsfwMetaDataStore::NewL(iMetaDataFilePath); 
        DEBUGSTRING(("SetupCacheL()"));
        // The format of label is <drive_letter>:<uri>.
        TRsfwMountConfig mountConfig;
        TRAPD(err, iMetaDataStore->GetMountConfigL(mountConfig));
        if ((err != KErrNone) ||
            mountConfig.iUri.Compare(iVolume->iMountInfo.iMountConfig.iUri) !=
            0)
            {
            // The saved metadata is not current - delete all
            DEBUGSTRING(("Clearing Metadata ..."));
            delete iMetaDataStore;
            iMetaDataStore = NULL;
            ClearCacheL();
            // Start from scratch
            iMetaDataStore = CRsfwMetaDataStore::NewL(iMetaDataFilePath);
            iMetaDataStore->SetMountConfigL(iVolume->iMountInfo.iMountConfig);
            }
        }
    else
        {
        ClearCacheL();
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::TotalCachedSize
// ----------------------------------------------------------------------------
// 
TInt CRsfwFileTable::TotalCachedSize() 
    {
    if (!iRootFep)
        {
        return 0;
        }
    return iRootFep->TotalCachedSize(); 
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::TotalEntryCount
// ----------------------------------------------------------------------------
// 
TInt CRsfwFileTable::TotalEntryCount() 
    {
    if (!iRootFep)
        {
        return 0;
        }
    return iRootFep->TotalEntryCount(); 
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::ClearCacheL
// ----------------------------------------------------------------------------
// 
void CRsfwFileTable::ClearCacheL()
    {
    DEBUGSTRING(("Clearing cache ..."));
    TFileName cachePath = iCachePath;
    _LIT(KWild, "*");
    cachePath.Append(KWild);
    
    CFileMan* fM = CFileMan::NewL(iFs);
    CleanupStack::PushL(fM);
    TInt err = fM->Delete(cachePath, CFileMan::ERecurse);
    CleanupStack::PopAndDestroy(fM); // fM
    if (err != KErrNone)
        {
        DEBUGSTRING16(("Cache cleaning of '%S' failed with err=%d",
                       &cachePath,
                       err));
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::NodeEvent
// ----------------------------------------------------------------------------
//   
TMetaDataEvent* CRsfwFileTable::NodeEvent(TInt aNodeId)
    {
    // Search downwards (for efficiency)
    TInt count = iMetaDataEvents.Count();
    if (count)
        {
        TInt i;
        for (i = count - 1; i >= 0; i--)
            {
            if (iMetaDataEvents[i].iNodeId == aNodeId)
                {
                return &iMetaDataEvents[i];
                }
            }
        }
    return NULL;
    }
    
// ----------------------------------------------------------------------------
// CRsfwFileTable::AddEvent
// ----------------------------------------------------------------------------
//   
void CRsfwFileTable::AddEvent(TInt aEvent, CRsfwFileEntry* aFep)
    {
    TMetaDataEvent event;
    event.iEvent = aEvent;
    event.iEntry = aFep;
    event.iNodeId= aFep->Fid().iNodeId;
    // For searching efficiency insert at the head
    if (iMetaDataEvents.Append(event) != KErrNone)
        {
        iMetaDataEvents.Close();
        iMetaDataSaveState = EMetaDataSaveFailed;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::RemoveEvent
// ----------------------------------------------------------------------------
//   
void CRsfwFileTable::RemoveEvent(TInt aNodeId)
    {
    TInt i;
    for (i = 0; i < iMetaDataEvents.Count(); i++)
        {
        if (iMetaDataEvents[i].iNodeId == aNodeId)
            {
            iMetaDataEvents.Remove(i);
            return;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::LoadNodeL
// ----------------------------------------------------------------------------
// 
void CRsfwFileTable::LoadNodeL(CRsfwFileEntry*& aFep, TInt &aSlot)
    {
    // Internalize a file entry
    // Read data from the file at the specified slot
    HBufC8* buf = HBufC8::NewLC(KMaxExternalizedFileEntrySize);
    TPtr8 ptr = buf->Des();
    TUint8* data = const_cast<TUint8 *>(ptr.Ptr());
    TInt dataLength;
    iMetaDataStore->GetNextDataL(data, dataLength, aSlot);
    RMemReadStream stream(data, dataLength);
    CleanupClosePushL(stream);
    CRsfwFileEntry* fep = CRsfwFileEntry::NewL(stream);
    DEBUGSTRING16(("CRsfwFileTable::LoadNodeL: Loaded node '%S'(id=%d, pid=%d, cn='%S')",
                   fep->Name(),
                   fep->Fid().iNodeId,
                   fep->iParentNodeId,
                   &fep->iCacheName));
    CleanupStack::PopAndDestroy(2); // stream, buf

    aFep = fep;
    }
    
// ----------------------------------------------------------------------------
// CRsfwFileTable::SaveNodeL
// ----------------------------------------------------------------------------
// 
void CRsfwFileTable::SaveNodeL(CRsfwFileEntry* aFep, TInt& aSlot)
    {
    // Externalize the file entry
    HBufC8* buf = HBufC8::NewLC(KMaxExternalizedFileEntrySize);
    TPtr8 ptr = buf->Des();
    TUint8* data = const_cast<TUint8 *>(ptr.Ptr());
    TInt dataLen;

    RMemWriteStream stream(data, KMaxExternalizedFileEntrySize);
    CleanupClosePushL(stream);

    if (aFep)
        {
        // dump the externalized data in the memory buffer
        // stream << *aFep;
        aFep->ExternalizeL(stream);
        MStreamBuf* streamBuf = stream.Sink();
        dataLen = streamBuf->TellL(MStreamBuf::EWrite).Offset();
        stream.CommitL();
        }
    else
        {

        DEBUGSTRING(("Removing slot %d", aSlot));
        // This will clear the slot
        data = NULL;
        dataLen = 0;
        }

    // Write data to the file at the specified slot
    iMetaDataStore->PutDataL(data, dataLen, aSlot);

    CleanupStack::PopAndDestroy(2, buf); // stream, buf
    }
    
// ----------------------------------------------------------------------------
// CRsfwFileTable::SaveMetaDataDeltaL
// ----------------------------------------------------------------------------
// 
void CRsfwFileTable::SaveMetaDataDeltaL()
    {
    DEBUGSTRING(("CRsfwFileTable::SaveMetaDataDeltaL"));
    if (!iPermanence)
        {
        return;
        }

    if (iMetaDataEvents.Count() == 0)

        {
        // Nothing to do
        return;
        }

    switch (iMetaDataSaveState)
        {
    case EMetaDataSaveNone:
        iMetaDataStore->ResetL(ETrue);
        iMetaDataSaveState = EMetaDataSaveStarted;
        break;

    case EMetaDataSaveStarted:
        break;

    case EMetaDataSaveFailed:
        DEBUGSTRING(("EMetaDataSaveFailed!"));
        User::Leave(KErrGeneral);
        break;

    default:
        break;
        }

    TInt i;
    for (i = 0; i < iMetaDataEvents.Count(); i++)
        {
        TInt slotPos;
        TMetaDataEvent *event = &iMetaDataEvents[i];
        
        DEBUGSTRING(("SaveMetaDataDeltaL: id=%d, event=%d",
                     event->iNodeId,
                     event->iEvent));

        switch (event->iEvent)
            {
        case KNotifyNodeModified:
        case KNotifyNodeAdded:
            {
            TMetaDataSlot s; // dummy for finding
            s.iNodeId = event->iNodeId;
            slotPos = iMetaDataSlots.Find(s);
            TInt slotId;
            if (slotPos != KErrNotFound)
                {
                slotId = iMetaDataSlots[slotPos].iSlotId;
                }
            else
                {
                // We don't have a slot yet
                slotId = -1;
                }
            SaveNodeL(event->iEntry, slotId);
            if (slotPos == KErrNotFound)
                {
                TMetaDataSlot slot;
                slot.iNodeId = event->iEntry->Fid().iNodeId;
                slot.iSlotId = slotId;
                iMetaDataSlots.Append(slot);
                }
            else
                {
                // The index may have changed
                iMetaDataSlots[slotPos].iSlotId = slotId;
                }
            }
            break;
            
        case KNotifyNodeRemoved:
            {
            TMetaDataSlot s; // dummy for finding
            s.iNodeId = event->iNodeId;
            slotPos = iMetaDataSlots.Find(s);
            if (slotPos != KErrNotFound)
                {
                TInt slotId = iMetaDataSlots[slotPos].iSlotId;
                iMetaDataSlots.Remove(slotPos);
                // Saving null is the same as removing
                SaveNodeL(NULL, slotId);              
                }
            }
            break;
            
        default:
            break;
            }
        }
    iMetaDataEvents.Reset();

    User::LeaveIfError(iMetaDataStore->Commit());
#if 0
    iMetaDataStore->CompactL();
#endif
    }


void CRsfwFileTable::ResolveDirtyFilesL() 
    {
    DEBUGSTRING(("CRsfwFileTable::ResolveDirtyFilesL"));
    if (iRootFep)
        {
        iRootFep->ResolveDirtyFilesL(); 
        }   
    SaveMetaDataDeltaL();    
    }

void CRsfwFileTable::ResolveDirtyFileL(CRsfwFileEntry *aFileEntry)
    {
    DEBUGSTRING(("CRsfwFileTable::ResolveDirtyFileL"));
	if ((aFileEntry->IsOpenedForWriting() && (aFileEntry->CacheFileName())))
		{
		
		DEBUGSTRING16(("file %S has uncommitted modifications, must be saved locally", aFileEntry->Name()));
		// file with uncommitted modifications
		// (i.e. saving changes to a remote server failed
		// show "save as" dialog for this file
		TRequestStatus status;
		TInt err;
    	TPckgBuf<TRsfwSaveToDlgRequest>   savetoRequest;
		TBuf<KRsfwMaxFileSizeString> fileSizeString;
        TEntry fEntry;
        CRsfwRfeServer::Env()->iFs.Entry((*(aFileEntry->CacheFileName())), fEntry);
        fileSizeString.Num(fEntry.iSize);
		TPtrC cacheDriveLetter = aFileEntry->CacheFileName()->Left(1);
	
	    savetoRequest().iMethod = TRsfwNotPluginRequest::ESaveToDlg;
        savetoRequest().iDriveName = Volume()->MountInfo()->iMountConfig.iName;								
        savetoRequest().iFileName = *(aFileEntry->Name());
        savetoRequest().iCacheDrive = cacheDriveLetter;
        savetoRequest().iFileSize = fileSizeString;

		
		RNotifier notifier;
		User::LeaveIfError(notifier.Connect());
		notifier.StartNotifierAndGetResponse(status, KRsfwNotifierPluginUID,
                    savetoRequest, savetoRequest);
        User::WaitForRequest(status);
		notifier.NotifyCancel();
		notifier.Close();
	
        if (status.Int() != KErrCancel) 
            {
            // move the file from cache to the new location
		    HBufC* newName = HBufC::NewMaxLC(KMaxPath);
            TPtr pathPtr = newName->Des();
            pathPtr = savetoRequest().iFileName;
            CFileMan* fman = CFileMan::NewL(CRsfwRfeServer::Env()->iFs);
            // we assume that this is local-to-local move, and can be synch. call
            err = fman->Move(*(aFileEntry->CacheFileName()), pathPtr, CFileMan::EOverWrite);
            delete fman;
            if (err == KErrNone) 
                {
                Volume()->iVolumeTable->WaitNoteManager()->ShowFileSavedToDialogL(pathPtr);
                }
            else 
                {
                Volume()->iVolumeTable->WaitNoteManager()->ShowFailedSaveNoteL();
                }
   
            CleanupStack::PopAndDestroy(newName);
            }
        
        // in any case, remove the file entry from the file table and cache
        // (has been moved or deleted)
        RemoveL(aFileEntry);
        delete aFileEntry;
        aFileEntry = NULL;
        DEBUGSTRING(("possible uncommitted modifications resolved"));  
		}
    }

