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
* Description:  data struct for a remote file metadata
*
*/


#ifndef C_RSFWFILEENTRY_H
#define C_RSFWFILEENTRY_H

#include "rsfwinterface.h"
#include "rsfwlrulistnode.h"

class TDirEntAttr;
class CRsfwLockManager;
class CRsfwDirEntAttr;
class CRsfwFileTable;


class CRsfwFileEntry: public CBase
    {
    friend class CRsfwFileTable;
    friend class CRsfwVolumeTable;
    friend class CRsfwLockManager;

public:
    ~CRsfwFileEntry();
    static CRsfwFileEntry* NewLC(const TDesC& aName, CRsfwFileEntry* aParent);
    static CRsfwFileEntry* NewL(const TDesC& aName, CRsfwFileEntry* aParent);
    static CRsfwFileEntry* NewL(RReadStream& aStream);

    CRsfwFileEntry* FindKidByName(const TDesC& aName);
    void RenameL(const TDesC& aName);
    void AddKid(CRsfwFileEntry& aFe);
    TInt RemoveKidL(CRsfwFileEntry* aFep);
    TInt KidsCount();
    void UnmarkKids();
    void DropUnmarkedKidsL();
    void DropLD();
    void GetAttributes(TDirEntAttr& aAttr) const;
    void GetAttributesL(CRsfwDirEntAttr& aAttr) const;
    void SetAttributesL(CRsfwDirEntAttr& aAttr, TBool aAllMetaData);
    TDesC* CacheFileName();
    void SetCacheFileName(TDesC* aFn);
    TBool IsCached() const;
    TBool IsFullyCached() const;
    void SetCached(TBool aCached);
    void SetCachedSize(TInt aFetchedSize);
    void RemoveCacheFile();
    void ValidateCacheFile();
    void PrintL(TInt aLevel, TBool aKids, TBool aall) const;
    HBufC* FullNameLC() const;
    TInt TotalCachedSize();
    TInt TotalEntryCount();
    CRsfwFileEntry* Lookup(const TFid& aFid);
    void SetLockedL(CRsfwLockManager* aLockManager, TDesC8* aLockToken);
    void RemoveLocked();
    static int LockTimerExpiredL(TAny* aParam);
    TBool UseCachedData();
    void SetAttribValidationTime();
    void ExternalizeL(RWriteStream& aStream) const;
    void InternalizeL(RReadStream& aStream);
    void SetType(TUint8 aType);
    void SetSize(TInt aSize);
    void SetModified(const TTime& aModified);
    void SetAtt(TUint aAtt);
    void SetMimeTypeL(const TDesC8& aMimeType);
    void SetOpaqueFileIdL(const TDesC8& aOpaqueFileId);
    TBool IsLocallyDirty() const;
    TBool IsCancelled() const;
    void SetLocallyDirty();
    void ResetLocallyDirty();
    TBool RemotelyDirty() const;
    void SetRemotelyDirty();
    void ResetRemotelyDirty();
    void SetNewlyCreated();
    void ResetNewlyCreated();
    TBool IsNewlyCreated() const;
    TBool IsMarked() const;
    void Mark();
    void Unmark();
    void SetFlags(TUint aFlags);
    void ResetFlags(TUint aFlags);
    void SetOpenedForWriting(TBool aOpenedForWriting);
    TBool IsOpenedForWriting() const;
    // for a remote file which has been locally modified, but not yet written back to the
    // server the cache file is set as read only until the file has been resolved
    TBool IsLocked() const;
    void ReportEvent(TInt aEvent);
    inline const TFid& Fid() const;
    inline void SetFid(const TFid& aFid);
    inline const TDesC* Name() const;
    inline TUint8 Type() const;
    inline TInt Size() const;
    inline TTime Modified() const;
    inline TUint Att() const;
    inline const TDesC8* MimeType() const;
    inline const TDesC8* OpaqueFileId() const;
    inline void SetUid(TUid anUid);
    inline CRsfwFileEntry* Parent();
    inline void SetParent(CRsfwFileEntry* aFep);
    inline TInt CachePriority() const;
    inline TBool IsFlagged(TUint aFlag) const;
    inline RPointerArray<CRsfwFileEntry>* Kids();
    inline const TDesC8* LockToken();
    inline const TDesC* ProtectionDomainName() const;
    inline void SetCachePriority(TCachePriority);
    inline TInt ProtectionDomainId() const;
    void ResolveDirtyFilesL();

private:
    void ConstructL(const TDesC& aName, CRsfwFileEntry* aParent);
    void ConstructL(RReadStream& aStream);
    
    void SetLockTimeout();

public:

    // how much has been cached
    TInt                    iCachedSize;

    // when the file attributes have been fetched from the server
    TTime                   iAttribValidation;

    // uid of the symbian app which handles this datatype
    TUid                    iUid;

    // Timer associated with a possible file locked
    CPeriodic*              iLockTimer;


protected:
    RPointerArray<CRsfwFileEntry> iKids;             // contained files and dirs

private:
    TFid                    iFid;            // id
    HBufC*                  iName;           // name
    TInt                    iSize;           // file size in bytes
    TUint                   iAtt;            // attribute bits
    TUint                   iFlags;          // local state
    TTime                   iModified;       // last modified
    TUint8                  iType;           // type (unknown, file, dir)
    HBufC8*                 iMimeType;       // MIME type
    HBufC8*                 iOpaqueFileId;   // e.g. ETag in WebDAV
    CRsfwFileEntry*             iParent;         // parent dir
    TBuf<KMaxPath>  iCacheName;   // local cache filename
    TDesC8*                 iLockToken;      // lock token
    // cache priority is currently always KCachePriorityNormal
    TInt                    iCachePriority;
    
    // Lock timeout for this locked file
    // In practise we need to store this per-file
    // as server may always ignore our request
    // and e.g. mod_dav allows setting different min timeout
    // for different directories.
    TUint                   iLockTimeout;

public:
    // pointer to lock manager that can be called to refresh the lock
    CRsfwLockManager*           iLockManager;
    TBool                   iUseCachedData;
    // only used at recovery when iParent is not set (yet)
    TInt                    iParentNodeId;
    // the table in which the entry belongs
    CRsfwFileTable*             iFileTable;
    };

#include "rsfwfileentry.inl"

#endif