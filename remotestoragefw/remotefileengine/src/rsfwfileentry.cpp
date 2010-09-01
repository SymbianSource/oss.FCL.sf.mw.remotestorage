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
* Description:  metadata struct for a remote file entry
*
*/


#include <bautils.h>

#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwconfig.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwrfeserver.h"
#include "rsfwlockmanager.h"
#include "mdebug.h"
#include "rsfwdirentattr.h"

// ----------------------------------------------------------------------------
// CRsfwFileEntry::NewLC
// ----------------------------------------------------------------------------
//
CRsfwFileEntry* CRsfwFileEntry::NewLC(const TDesC& aName, CRsfwFileEntry* aParent)
    {
    CRsfwFileEntry* self = new (ELeave) CRsfwFileEntry();
    CleanupStack::PushL(self);
    self->ConstructL(aName, aParent);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::NewL
// ----------------------------------------------------------------------------
//
CRsfwFileEntry* CRsfwFileEntry::NewL(const TDesC& aName, CRsfwFileEntry* aParent)
    {
    CRsfwFileEntry* self = NewLC(aName, aParent);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::NewL
// ----------------------------------------------------------------------------
//
CRsfwFileEntry* CRsfwFileEntry::NewL(RReadStream& aStream)
    {
    CRsfwFileEntry* self = new (ELeave) CRsfwFileEntry();
    CleanupStack::PushL(self);
    self->ConstructL(aStream);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::ConstructL(const TDesC& aName, CRsfwFileEntry* aParent)
    {
    iType = KNodeTypeUnknown;
    iParent = aParent;
    iName = aName.AllocL();
    iAtt = KEntryAttRemote;
    iCachePriority = ECachePriorityNormal;
    
    SetLockTimeout();
    // Note that we don't yet attach the kid to its parent
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::ConstructL(RReadStream& aStream)
    {
    // aStream >> *this;
    this->InternalizeL(aStream);
    SetLockTimeout();
    // Note that we don't yet attach the kid to its parent
    }

void CRsfwFileEntry::SetLockTimeout() 
    {
    // When creating a file entry, the lock timeout is set to default
    // even when internalizing from stream.
    // We do not assume any locks that would survive server restarts
    TInt timeout;
    TInt err = CRsfwRfeServer::Env()->iRsfwConfig->Get(RsfwConfigKeys::KLockTimeout,
                                                       timeout);
    if (!err)
        {
        iLockTimeout = (TUint)timeout;
        }
    else
        {
        iLockTimeout = KDefaultLockTimeout;
        }
    
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::~CRsfwFileEntry
// ----------------------------------------------------------------------------
//
CRsfwFileEntry::~CRsfwFileEntry()
    {
    if (iFlags & KNodeHasValidLock)
        {
        if (iLockManager)
            {
            iLockManager->RemoveLockedEntry(this);
            }
        }
    delete iName;
    delete iMimeType;
    delete iOpaqueFileId;
    delete iLockToken;
    
    if (!iFileTable || !iFileTable->Permanence())
        {
        RemoveCacheFile();
        }
    
    if ( iFileTable )
        {
        iFileTable->Volume()->iVolumeTable->RemoveFromMetadataLRUPriorityList(this);        
        }
    
    delete iLockTimer;

    // delete kids
    TInt i;
    for(i = 0; i < iKids.Count(); i++)
        {
        delete iKids[i];
        }
    iKids.Close();
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::FindKidByName
// ----------------------------------------------------------------------------
//
CRsfwFileEntry* CRsfwFileEntry::FindKidByName(const TDesC& aName)
    {
     DEBUGSTRING(("CRsfwFileEntry::FindKidByName"));
    // finds a kid from a parent directory
    TInt i;
    for (i = 0; i < iKids.Count(); i++)
        {
        CRsfwFileEntry* kid = iKids[i];
        if (kid->iName->Compare(aName) == 0)
            {
            return iKids[i];
            }
        }
    DEBUGSTRING(("...kid not found!"));   
    return NULL;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::RenameL
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::RenameL(const TDesC& aName)
    {
    delete iName;
    iName = NULL;
    iName = aName.AllocL();
    ReportEvent(KNotifyNodeModified);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::AddKid
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::AddKid(CRsfwFileEntry& aFe)
    {
    // if this is the first kid to be added then probably
    // we have to remove the entry from metadata LRU list
    if ( iKids.Count() == 0 )
        {
        iFileTable->Volume()->iVolumeTable->RemoveFromMetadataLRUPriorityList(this);
        }
    iKids.Append(&aFe);
    // (This assignment is sometimes redundant)
    aFe.SetParent(this);
    ReportEvent(KNotifyNodeModified);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::RemoveKidL
// ----------------------------------------------------------------------------
//
TInt CRsfwFileEntry::RemoveKidL(CRsfwFileEntry* aFep)
    {
    TInt i;
    for (i = 0; i < iKids.Count(); i++)
        {
        if (iKids[i] == aFep)
            {
            ReportEvent(KNotifyNodeModified);
            iKids.Remove(i);
            // if we've just removed the last kid of the entry
            // we can add the entry to metadata LRU list
            if ( iKids.Count() == 0 )
                {
                iFileTable->Volume()->iVolumeTable->AddToMetadataLRUPriorityListL(this, ECachePriorityNormal);
                }
            return KErrNone;
            }
        }
    DEBUGSTRING(("remove kid %d not found in %d",
                 aFep->Fid().iNodeId,
                 Fid().iNodeId));
    return KErrNotFound;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::KidsCount
// ----------------------------------------------------------------------------
//
TInt CRsfwFileEntry::KidsCount()
    {
    return iKids.Count();
    }


// ----------------------------------------------------------------------------
// CRsfwFileEntry::UnmarkKids
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::UnmarkKids()
    {
    TInt i;
    for (i = 0; i < iKids.Count(); i++)
        {
        iKids[i]->Unmark();
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::DropUnmarkedKidsL
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::DropUnmarkedKidsL()
    {
    TInt i = 0;
    while (i < iKids.Count())
        {
        if (!iKids[i]->IsMarked())
            {
            iKids[i]->DropLD();
            }
        else
            {
            i++;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::DropLD
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::DropLD()
    {
    DEBUGSTRING(("CRsfwFileEntry::DropLD"));
    TInt i = 0;
    while (i < iKids.Count())
        {
        iKids[i]->DropLD();
        }
        
    iFileTable->RemoveL(this);
    delete this;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::GetAttributes
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::GetAttributes(TDirEntAttr& aAttr) const
    {
    aAttr.iAtt = Att();
    aAttr.iSize = Size();
    aAttr.iModified = Modified();
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::GetAttributesL
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::GetAttributesL(CRsfwDirEntAttr& aAttr) const
    {
    aAttr.SetAtt(Att());
    aAttr.SetSize(Size());
    aAttr.SetModified(Modified());
    if (iOpaqueFileId)
        {
        aAttr.SetETagL(*OpaqueFileId());
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetAttributesL
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetAttributesL(CRsfwDirEntAttr& aAttr,
                                    TBool aAllMetaData)
    {
    SetAtt(aAttr.Att());
    if (aAllMetaData) 
        {
        SetSize(aAttr.Size());
        SetModified(aAttr.Modified());
        if (aAttr.MimeType())
            {
            SetMimeTypeL(*aAttr.MimeType());
            }
        if (aAttr.ETag())
            {
            SetOpaqueFileIdL(*aAttr.ETag());
            }
        SetUid(aAttr.Uid());
        SetAttribValidationTime();
        }

    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::CacheFileName
// ----------------------------------------------------------------------------
//
TDesC* CRsfwFileEntry::CacheFileName()
    {
    DEBUGSTRING(("CRsfwFileEntry::CacheFileName"));
    if (iCacheName.Length())
        {
        return &iCacheName;
        }
    return NULL;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetCacheFileName
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetCacheFileName(TDesC* aFn)
    {
    DEBUGSTRING16(("SetCacheFileName for file %S", Name()));
    if (aFn)
        {
        iCacheName = *aFn;
        ReportEvent(KNotifyNodeModified);
        }
    else
        {
        if (iCacheName.Length())
            {
            if (IsCached())
                {
                // Remove the cache list entry...
                iFileTable->
                    Volume()->
                    iVolumeTable->RemoveFromLRUPriorityList(this);
                }
            // This is a request to discard the container
            RFs fs = CRsfwRfeServer::Env()->iFs;
            TInt err = fs.Delete(iCacheName);
            if (err != KErrNone)
                {
                DEBUGSTRING(("Cannot purge cache file (err=%d)", err));
                }
            iCacheName.Zero();
            // Reset locally dirty in case this is a directory.
            // "locally dirty" means that the container
            // doesn't have the "cached"/"protected" indicator bits up to date
            // (these indicators refer to files contained in the directory).
            iFlags &= ~KNodeLocallyDirty;
            ReportEvent(KNotifyNodeModified);
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::IsCached
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEntry::IsCached() const
    {
   DEBUGSTRING(("CRsfwFileEntry::IsCached, iAtt = %d, iFlags = %d", iAtt, iFlags));
    if (((iAtt & KEntryAttRemote) == 0) ||
        (iFlags & KNodePartlyCached))
        {
         DEBUGSTRING(("returning ETrue"));
        // File is either fully or partly cached
        return ETrue;
        }
        
    DEBUGSTRING(("returning EFalse"));
    return EFalse;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::IsFullyCached
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEntry::IsFullyCached() const
    {
    DEBUGSTRING(("CRsfwFileEntry::IsFullyCached"));
    DEBUGSTRING(("iCachedSize = %d, iSize = %d", iCachedSize, iSize));
    if (Type() == KNodeTypeDir)
        {
        return IsCached();
        }
    else
        {
        if (iCachedSize == iSize)
            {
            return IsCached();
            }
        else
            {
            return EFalse;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetCached
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetCached(TBool aCached)
    {
    DEBUGSTRING(("CRsfwFileEntry::SetCached"));
    TUint oldAtt = iAtt;
    if (aCached)
        {
        if (Type() == KNodeTypeDir)
            {
            // set to fully cached
            DEBUGSTRING(("set directory to fully cached"));
            iAtt &= ~KEntryAttRemote;
            iFlags &= ~KNodePartlyCached;
            }
        else
            {
            if (iCachedSize == iSize)
                {
                // set file to fully cached
                DEBUGSTRING(("set file to fully cached"));
                iAtt &= ~KEntryAttRemote;
                iFlags &= ~KNodePartlyCached;
                }
            else
                {
                // Set file to partly cached
                DEBUGSTRING(("set file to partly cached"));
                iAtt |= KEntryAttRemote;
                iFlags |= KNodePartlyCached;
                }
            }
        }
    else
        {
        // set to "fully" remote
        DEBUGSTRING(("set to fully remote"));
        iFlags &= ~KNodePartlyCached;
        iAtt |= KEntryAttRemote;
        iUseCachedData = EFalse;
        iCachedSize = 0;
        }
    if (iAtt != oldAtt)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetCachedSize
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetCachedSize(TInt aFetchedSize)
    {
    TInt oldCachedSize = iCachedSize;
    iCachedSize = aFetchedSize;
    if (iCachedSize != oldCachedSize)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::RemoveCacheFile
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::RemoveCacheFile()
    {
   DEBUGSTRING(("CRsfwFileEntry::RemoveCacheFile"));
    if (IsCached() && iFileTable)
        {
        // Remove the cache list entry...
        iFileTable->Volume()->iVolumeTable->RemoveFromLRUPriorityList(this);
        }

    if (iCacheName.Length())
        {
        RFs fs = CRsfwRfeServer::Env()->iFs;
        TInt err = fs.Delete(iCacheName);
        if ((err != KErrNone) && (err != KErrNotFound))
            {
            DEBUGSTRING(("Cannot delete cache file (err=%d)", err));
            }
        iCacheName.Zero();
        }
    SetCached(EFalse);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::ValidateCacheFile
// Function checks whether cache file has not been accidentally or intentionally
// removed from the cache (which would mean the cache has been corrupted)
// In case the corruption has happened, the function sets entry as non-cached
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::ValidateCacheFile()
    {
    if (iCacheName.Length() > 0)
        {
        RFs fs = CRsfwRfeServer::Env()->iFs;
        if (! BaflUtils::FileExists(fs, iCacheName))
            {
            SetCached(EFalse);        
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::PrintL
// ----------------------------------------------------------------------------
//
#ifdef _DEBUG
void CRsfwFileEntry::PrintL(TInt aLevel, TBool aKids, TBool aAll) const
    {
    if (!IsCached() && !aAll)
        {
        // Print only information about cached files
        return;
        }

    HBufC* sBuf = HBufC::NewLC(KMaxPath);
    TPtr s = sBuf->Des();

    s.Fill(' ', 4 * aLevel);
    s.AppendNum(iFid.iNodeId);
    s.Append('|');
    s.Append(*iName);
    switch (iType)
        {
    case KNodeTypeDir:
        s.Append('/');
        break;

    case KNodeTypeFile:
        break;

    default:
        s.Append('?');
        break;
        }

    if (IsCached())
        {
        s.Append('|');
        s.Append(iCacheName);
        }

    DEBUGBUFFER((s));

    CleanupStack::PopAndDestroy(sBuf); // sBuf

    if (aKids)
        {
        TInt i;
        for (i = 0; i < iKids.Count(); i++)
            {
            iKids[i]->PrintL(aLevel + 1, aKids, aAll);
            }
        }
    }
#else
void CRsfwFileEntry::PrintL(TInt, TBool, TBool) const
    {
    }
#endif //DEBUG


// ----------------------------------------------------------------------------
// CRsfwFileEntry::FullNameLC
// Construct full name relative to the root.
// The caller is responsible for deallocating the return value.
// ----------------------------------------------------------------------------
//
HBufC* CRsfwFileEntry::FullNameLC() const
    {
    // We know that we can't have more than KMaxPath entries,
    // because each entry is minimally "/"
    CRsfwFileEntry* entList[KMaxPath / 2];

    HBufC* fn = HBufC::NewLC(KMaxPath);
    TPtr fnp = fn->Des();
    CRsfwFileEntry* fep = const_cast<CRsfwFileEntry*>(this);
    TInt depth = 0;
    do
        {
        if (depth >= (KMaxPath / 2))
            {
            // Too deep hierarchy
            DEBUGSTRING(("CRsfwFileEntry::FullNameLC - Too deep hierarchy! %d", depth));
            User::Leave(KErrGeneral);
            }
        entList[depth++] = fep;
        fep = fep->iParent;
        }
    while (fep);

    // We want to avoid going right to the root to avoid dots
    depth--;

    TInt i;
    for (i = depth - 1; i >= 0; i--)
        {
        TPtr name = entList[i]->iName->Des();
        if (i != (depth - 1))
            {
            // Skip "this" directories (should not happen)
            if ((name[0] == '.') && (name.Length() == 1))
                {
                continue;
                }
            }
        if ((fnp.Length() + name.Length()) >= (KMaxPath - 1))
            {
            // Too long name
            DEBUGSTRING(("CRsfwFileEntry::FullNameLC - Too long name!"));
            User::Leave(KErrGeneral);
            }
        fnp.Append(name);
        if (i != 0)
            {
            fnp.Append('/');
            }
        }

    return fn;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::TotalCachedSize
// ----------------------------------------------------------------------------
//
TInt CRsfwFileEntry::TotalCachedSize()
    {
    TInt cachedSize = 0;
    TInt i;
    
    for (i = 0; i < iKids.Count(); i++)
        {
        TInt newSize = iKids[i]->TotalCachedSize();
        cachedSize = cachedSize + newSize;
        }
    cachedSize = cachedSize + iCachedSize;
    return cachedSize;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::TotalEntryCount
// ----------------------------------------------------------------------------
//
TInt CRsfwFileEntry::TotalEntryCount()
    {
    TInt entryCount = 0;
    TInt i;
    for (i = 0; i < iKids.Count(); i++)
        {
        TInt kidCount = iKids[i]->TotalEntryCount();
        entryCount += kidCount;
        }
    entryCount += 1; // itself
    return entryCount;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::Lookup
// ----------------------------------------------------------------------------
//
CRsfwFileEntry* CRsfwFileEntry::Lookup(const TFid& aFid)
    {
    // linear search - immediate kids first
    TInt i;
    for (i = 0; i < iKids.Count(); i++)
        {
        CRsfwFileEntry* fep = iKids[i];
        if (fep->Fid().iNodeId == aFid.iNodeId)
            {
            return iKids[i];
            }
        }
    // Not found - lookup the kids' kids 
    for (i = 0; i < iKids.Count(); i++)
        {
        CRsfwFileEntry* fep;
        fep = iKids[i]->Lookup(aFid);
        if (fep)
            {
            return fep;
            }
        }
    return NULL;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetLockedL
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetLockedL(CRsfwLockManager* lockManager, TDesC8* aLockToken)
    {
    DEBUGSTRING16(("Set locked: marking file '%S' locked", Name()));
    if (iLockTimeout > 0)
        {
        if (!iLockTimer)
            {
            iLockTimer = CPeriodic::NewL(CActive::EPriorityHigh);
            }

        // attempt to refresh when one third of the timeout has expired
        TCallBack callBack(CRsfwFileEntry::LockTimerExpiredL, this);
        iLockTimer->Start(1000000*(iLockTimeout/KLockRefreshAdjustment),
                          1000000*(iLockTimeout/KLockRefreshAdjustment),
                          callBack);
        }
    iFlags |= KNodeHasValidLock;
    iLockManager = lockManager;
    iLockManager->AddLockedEntryL(this);
    if (aLockToken)
        {
        // We were not just refreshing the lock
        delete iLockToken;
        iLockToken = aLockToken;
        }
    ReportEvent(KNotifyNodeModified);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::RemoveLocked
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::RemoveLocked()
    {
    DEBUGSTRING16(("Remove locked: marking file '%S' unlocked", Name()));
    if (iFlags & KNodeHasValidLock)
        {
        iLockManager->RemoveLockedEntry(this);
        iLockManager = NULL; // will be set in SetLockedL, if needed once again
        iFlags &= ~KNodeHasValidLock;
       ReportEvent(KNotifyNodeModified);
        }
    
    if (iLockToken) 
        {
        delete iLockToken;
        iLockToken = NULL;
        }
    if (iLockTimer) 
        {
        delete iLockTimer;
        iLockTimer = NULL;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::LockTimerExpiredL
// ----------------------------------------------------------------------------
//
TInt CRsfwFileEntry::LockTimerExpiredL(TAny* aParam)
    {
    CRsfwFileEntry* fe = static_cast<CRsfwFileEntry*>(aParam);
    DEBUGSTRING16(("Lock timer expired for '%S'", fe->Name()));
    fe->iLockManager->RefreshLockL(fe);
    return KErrNone;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::UseCachedData
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEntry::UseCachedData()
    {
    // now meta data should tell us whether to use cached data or not
    return iUseCachedData && !RemotelyDirty();
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetAttribValidationTime
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetAttribValidationTime()
    {
    iAttribValidation.UniversalTime();
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::ExternalizeL
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::ExternalizeL(RWriteStream& aStream) const
    {
    DEBUGSTRING16(("CRsfwFileEntry::ExternalizeL for node %d", iFid.iNodeId));
    DEBUGSTRING16(("iFlags: %d", &iFlags));
    // The node Id must be the first entry

    // iNodeId, iParentNodeId, iType, iSize, iAtt, iModified, iFlags,
    // iCachedSize, iCachePriority
    // iCacheName, iName, iMimeType,
    // iOpaqueFileId, iLockToken

    aStream.WriteInt32L(iFid.iNodeId);
    if (iParent)
        {
        aStream.WriteUint32L(iParent->Fid().iNodeId);
        }
    else
        {
        // Root
        aStream.WriteUint32L(0);
        }
    aStream.WriteUint8L(iType);
    aStream.WriteInt32L(iSize);
    aStream.WriteUint32L(iAtt);
    aStream.WriteUint32L(I64HIGH(iModified.Int64()));
    aStream.WriteUint32L(I64LOW(iModified.Int64()));
    aStream.WriteUint32L(iFlags);
    aStream.WriteInt32L(iCachedSize);
    aStream.WriteInt32L(iCachePriority);
    aStream.WriteInt32L(iUseCachedData);
    aStream << iCacheName;

    HBufC* null = HBufC::NewLC(0);
    if (iName)
        {
        aStream << *iName;
        }
    else
        {
        aStream << *null;
        }

    if (iMimeType)
        {
        aStream << *iMimeType;
        }
    else
        {
        aStream << *null;
        }

    if (iOpaqueFileId)
        {
        aStream << *iOpaqueFileId;
        }
    else
        {
        aStream << *null;
        }

    if (iLockToken)
        {
        aStream << *iLockToken;
        }
    else
        {
        aStream << *null;
        }

    CleanupStack::PopAndDestroy(null); // null
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::InternalizeL
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::InternalizeL(RReadStream& aStream)
    {
    DEBUGSTRING16(("CRsfwFileEntry::InternalizeL for node %d", iFid.iNodeId));
    // iNodeId, iParentNodeId, iType, iSize, iAtt, iModified, iFlags,
    // iCachedSize, iCachePriority
    // iCacheName, iName, iMimeType,
    // iOpaqueFileId, iLockToken
    
    // make some basic checking whether data being internalized is correct
    iFid.iNodeId = aStream.ReadInt32L();
    if (iFid.iNodeId < 0)
        {
        User::Leave(KErrCorrupt);
        }
    iParentNodeId = aStream.ReadInt32L();
    if (iParentNodeId < 0)
        {
        User::Leave(KErrCorrupt);
        }
    iType = aStream.ReadUint8L();
    iSize = aStream.ReadInt32L();
    if (iSize < 0)
        {
        User::Leave(KErrCorrupt);
        }    
    iAtt = aStream.ReadUint32L();
    TInt highTime = aStream.ReadUint32L();
    TInt lowTime = aStream.ReadUint32L();
    iModified = MAKE_TINT64(highTime, lowTime);
    iFlags = aStream.ReadUint32L();
    DEBUGSTRING16(("iFlags: %d", &iFlags));
    iCachedSize = aStream.ReadInt32L();
    if (iCachedSize < 0)
        {
        User::Leave(KErrCorrupt);
        }
    iCachePriority = aStream.ReadInt32L();
    iUseCachedData = aStream.ReadInt32L();
    aStream >> iCacheName;

    HBufC* buf = HBufC::NewL(aStream, KMaxPath);
    if (buf->Length())
        {
        iName = buf;
        }
    else
        {
        delete buf;
        buf = NULL;
        }

    // MimeType
    HBufC8* buf8 = HBufC8::NewL(aStream, KMaxPath);
    if (buf8->Length())
        {
        iMimeType = buf8;
        }
    else
        {
        delete buf8;
        }

    // OpaqueFileId
    buf8 = HBufC8::NewL(aStream, KMaxPath);
    if (buf8->Length())
        {
        iOpaqueFileId = buf8;
        }
    else
        {
        delete buf8;
        }

    // LockToken
    buf8 = HBufC8::NewL(aStream, KMaxPath);
    if (buf8->Length())
        {
        iLockToken = buf8;
        }
    else
        {
        delete buf8;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetType
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetType(TUint8 aType)
    {
    TUint8 oldType = iType;
    iType = aType;
    if (aType == KNodeTypeDir)
        {
        iAtt |= KEntryAttDir;
        }
    else
        {
        iAtt &= ~KEntryAttDir;
        }
    if (iType != oldType)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetSize
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetSize(TInt aSize)
    {
    TInt oldSize = iSize;
    iSize = aSize;
    if (iSize != oldSize)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetModified
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetModified(const TTime& aModified)
    {
    TTime oldModified = iModified;
    iModified = aModified;
    if (iModified != oldModified)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetAtt
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetAtt(TUint aAtt)
    {
    // Don't change caching and protected state
    TUint oldAtt = iAtt;
    if (IsFullyCached())
        {
        aAtt &= ~KEntryAttRemote;
        }
    else
        {
        aAtt |= KEntryAttRemote;
        }
    iAtt = aAtt;

    // Set node type
    if (iAtt & KEntryAttDir)
        {
        iType = KNodeTypeDir;
        }
    else
        {
        iType = KNodeTypeFile;
        }

    if (iAtt != oldAtt)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetMimeTypeL
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetMimeTypeL(const TDesC8& aMimeType)
    {
    if (iMimeType)
        {
        delete iMimeType;
        iMimeType = NULL;
        }
    if (aMimeType.Length())
        {
        iMimeType = aMimeType.AllocL();
        }

    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetOpaqueFileIdL
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetOpaqueFileIdL(const TDesC8& aOpaqueFileId)
    {
    if (iOpaqueFileId)
        {
        delete iOpaqueFileId;
        iOpaqueFileId = NULL;
        }
    if (aOpaqueFileId.Length())
        {
        iOpaqueFileId = aOpaqueFileId.AllocL();
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::IsLocallyDirty
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEntry::IsLocallyDirty() const
    {
    DEBUGSTRING16(("IsLocallyDirty for file %S", Name()));
    return (iFlags & KNodeLocallyDirty) != 0;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::IsCancelled
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEntry::IsCancelled() const
    {
    DEBUGSTRING16(("CRsfwFileEntry::IsCancelled()"));
    return (iFlags & KNodeWritingCancelled) != 0;
    }


// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetLocallyDirty
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetLocallyDirty()
    {
    DEBUGSTRING16(("SetLocallyDirty for file %S", Name()));
    TUint oldFlags = iFlags;
    iFlags |= KNodeLocallyDirty;
    if (iFlags != oldFlags)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::ResetLocallyDirty
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::ResetLocallyDirty()
    {
    DEBUGSTRING16(("ResetLocallyDirty for file %S", Name()));
    TUint oldFlags = iFlags;
    iFlags &= ~KNodeLocallyDirty;
    if (iFlags != oldFlags)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::RemotelyDirty
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEntry::RemotelyDirty() const
    {
    return (iFlags & KNodeRemotelyDirty) != 0;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetRemotelyDirty
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetRemotelyDirty()
    {
    TUint oldFlags = iFlags;
    iFlags |= KNodeRemotelyDirty;
    if (iFlags != oldFlags)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::ResetRemotelyDirty
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::ResetRemotelyDirty()
    {
    TUint oldFlags = iFlags;
    iFlags &= ~KNodeRemotelyDirty;
    if (iFlags != oldFlags)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::IsMarked
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEntry::IsMarked() const
    {
    return (iFlags & KNodeMarked) != 0;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::Mark
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::Mark()
    {
    // This is transient state (so, it need not be saved persistently)
    iFlags |= KNodeMarked;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::Unmark
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::Unmark()
    {
    iFlags &= ~KNodeMarked;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetFlags
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetFlags(TUint aFlags)
    {
    TUint oldFlags = iFlags;
    iFlags |= aFlags;
    if (iFlags != oldFlags)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::ResetFlags
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::ResetFlags(TUint aFlags)
    {
    TUint oldFlags = iFlags;
    iFlags &= ~aFlags;
    if (iFlags != oldFlags)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetOpenedForWriting
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetOpenedForWriting(TBool aOpenedForWriting)
    {
    TUint oldFlags = iFlags;
    if (aOpenedForWriting)
        {
        DEBUGSTRING(("CRsfwFileEntry::SetOpenedForWriting TRUE"));
        iFlags |= KNodeOpenedForWriting;
        }
    else
        {
        DEBUGSTRING(("CRsfwFileEntry::SetOpenedForWriting FALSE"));
        iFlags &= ~KNodeOpenedForWriting;
        }
    if (iFlags != oldFlags)
        {
        ReportEvent(KNotifyNodeModified);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::IsOpenedForWriting
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEntry::IsOpenedForWriting() const
    {
    DEBUGSTRING(("CRsfwFileEntry::IsOpenedForWriting"));
    return (iFlags & KNodeOpenedForWriting) != 0;
    }
     

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetNewlyCreated
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::SetNewlyCreated()
    {
    iFlags |= KNodeNewlyCreated;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::ResetNewlyCreated
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::ResetNewlyCreated()
    {
    iFlags &= ~KNodeNewlyCreated;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::IsNewlyCreated
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEntry::IsNewlyCreated() const
    {
    return (iFlags & KNodeNewlyCreated) != 0;
    }

    
// ----------------------------------------------------------------------------
// CRsfwFileEntry::IsLocked
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEntry::IsLocked() const
    {
    return (iFlags & KNodeHasValidLock) != 0;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::ReportEvent
// ----------------------------------------------------------------------------
//
void CRsfwFileEntry::ReportEvent(TInt aEvent)
    {
    // If there is no file table,
    // this is a transient entry
    if (iFileTable && iFileTable->Permanence())
        {
        iFileTable->HandleMetaDataEvent(aEvent, this);
        }
    }


void CRsfwFileEntry::ResolveDirtyFilesL() 
    {
    DEBUGSTRING(("CRsfwFileEntry::ResolveDirtyFilesL"));
    if (this->Type() == KNodeTypeDir)
        {
        for (int i = 0; i < iKids.Count(); i++)
            {
            iKids[i]->ResolveDirtyFilesL();
            }        
        }
    else  if (this->Type() == KNodeTypeFile)
        {
        // this is a leaf
        iFileTable->ResolveDirtyFileL(this);
        }

    }
