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
* Description:  inlines for file entry data structure
*
*/



// ----------------------------------------------------------------------------
// CRsfwFileEntry::Fid
// ----------------------------------------------------------------------------
// 
inline const TFid& CRsfwFileEntry::Fid() const
    {
    return iFid;
    }
    
// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetFid
// ----------------------------------------------------------------------------
//     
inline void CRsfwFileEntry::SetFid(const TFid& aFid)
    {
    iFid = aFid;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::Name
// ----------------------------------------------------------------------------
// 
inline const TDesC* CRsfwFileEntry::Name() const
    {
    return iName;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::Type
// ----------------------------------------------------------------------------
// 
inline TUint8 CRsfwFileEntry::Type() const
    {
    return iType;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::Size
// ----------------------------------------------------------------------------
// 
inline TInt CRsfwFileEntry::Size() const
    {
    return iSize;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::Modified
// ----------------------------------------------------------------------------
// 
inline TTime CRsfwFileEntry::Modified() const
    {
    return iModified;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::Att
// ----------------------------------------------------------------------------
// 
inline TUint CRsfwFileEntry::Att() const
    {
    return iAtt;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::MimeType
// ----------------------------------------------------------------------------
// 
inline const TDesC8* CRsfwFileEntry::MimeType() const
    {
    return iMimeType;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::OpaqueFileId
// ----------------------------------------------------------------------------
// 
inline const TDesC8* CRsfwFileEntry::OpaqueFileId() const
    {
    return iOpaqueFileId;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetUid
// ----------------------------------------------------------------------------
// 
inline void CRsfwFileEntry::SetUid(TUid anUid) 
    {
    iUid = anUid;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::Parent
// ----------------------------------------------------------------------------
// 
inline CRsfwFileEntry* CRsfwFileEntry::Parent()
    {
    return iParent;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::SetParent
// ----------------------------------------------------------------------------
// 
inline void CRsfwFileEntry::SetParent(CRsfwFileEntry* aParent)
    {
    // This meta data event is handle in kid Add/Remove functions
    iParent = aParent;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::CachePriority
// ----------------------------------------------------------------------------
// 
inline TInt CRsfwFileEntry::CachePriority() const
    {
    return iCachePriority;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::IsFlagged
// ----------------------------------------------------------------------------
// 
inline TBool CRsfwFileEntry::IsFlagged(TUint aFlag) const
    {
    return (iFlags & aFlag) != 0;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::Kids
// ----------------------------------------------------------------------------
// 
inline RPointerArray<CRsfwFileEntry>* CRsfwFileEntry::Kids()
    {
    return &iKids;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::LockToken
// ----------------------------------------------------------------------------
// 
inline const TDesC8* CRsfwFileEntry::LockToken()
    {
    return iLockToken;
    }

 