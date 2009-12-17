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
* Description:  Directory entry container
 *
*/


// INCLUDE FILES
#include "rsfwdirentattr.h"

// ====================== CRsfwDirEntAttr MEMBER FUNCTIONS ========================

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::NewLC
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwDirEntAttr* CRsfwDirEntAttr::NewLC()
    {
    CRsfwDirEntAttr* self = new (ELeave) CRsfwDirEntAttr();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::NewL
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwDirEntAttr* CRsfwDirEntAttr::NewL()
    {
    CRsfwDirEntAttr* self = NewLC();
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::CRsfwDirEntAttr
// ----------------------------------------------------------------------------
//
CRsfwDirEntAttr::CRsfwDirEntAttr()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwDirEntAttr::ConstructL()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::~CRsfwDirEntAttr
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwDirEntAttr::~CRsfwDirEntAttr()
    {
    TInt i;
    for (i = 0; i < EDirEntAttrStringCount; i ++)
        {
        delete iStringValues[i];
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::Att
// ----------------------------------------------------------------------------
//
EXPORT_C TUint CRsfwDirEntAttr::Att() const
    {
    return iAtt;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::SetAtt
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEntAttr::SetAtt(TUint aAtt)
    {
    iAtt = aAtt;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::SetAttFlags
// ----------------------------------------------------------------------------
//    
EXPORT_C void CRsfwDirEntAttr::SetAttFlags(TUint aFlags)
    {
    iAtt |= aFlags;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::ResetAttFlags
// ----------------------------------------------------------------------------
//    
EXPORT_C void CRsfwDirEntAttr::ResetAttFlags(TUint aFlags)
    {
    iAtt &= ~aFlags;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::Size
// ----------------------------------------------------------------------------
//    
EXPORT_C TInt CRsfwDirEntAttr::Size() const
    {
    return iSize;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::SetSize
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEntAttr::SetSize(TInt aSize)
    {
    iSize = aSize;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::Modified
// ----------------------------------------------------------------------------
//
EXPORT_C TTime CRsfwDirEntAttr::Modified() const
    {
    return iModified;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::SetModified
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEntAttr::SetModified(const TTime& aModified)
    {
    iModified = aModified;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::Uid
// ----------------------------------------------------------------------------
//
EXPORT_C const TUid& CRsfwDirEntAttr::Uid() 
    {
    return iUid;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::SetUid
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEntAttr::SetUid(TUid aUid) 
    {
    iUid = aUid;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::StringValue
// ----------------------------------------------------------------------------
//
EXPORT_C const TDesC8* CRsfwDirEntAttr::StringValue(TInt aIndex) const
    {
    if ((aIndex < 0) || (aIndex >= EDirEntAttrStringCount))
        {
        return NULL;
        }
    return iStringValues[aIndex];
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::SetStringValueL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEntAttr::SetStringValueL(TInt aIndex, const TDesC8& aString) 
    {
    if ((aIndex < 0) || (aIndex >= EDirEntAttrStringCount))
        {
        User::Leave(KErrArgument);
        }
    HBufC8** s = &iStringValues[aIndex];
    if (*s)
        {
        delete *s;
        *s = NULL;
        }
    if (aString.Length())
        {
        *s = aString.AllocL();
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::MimeType
// ----------------------------------------------------------------------------
//
EXPORT_C const TDesC8* CRsfwDirEntAttr::MimeType() const
    {
    return StringValue(EDirEntAttrStringMimeType);
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::SetMimeTypeL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEntAttr::SetMimeTypeL(const TDesC8& aMimeType) 
    {
    SetStringValueL(EDirEntAttrStringMimeType, aMimeType);
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::ETag
// ----------------------------------------------------------------------------
//
EXPORT_C const TDesC8* CRsfwDirEntAttr::ETag() const
    {
    return StringValue(EDirEntAttrStringETag);
    }

// ----------------------------------------------------------------------------
// CRsfwDirEntAttr::SetETagL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEntAttr::SetETagL(const TDesC8& aETag) 
    {
    SetStringValueL(EDirEntAttrStringETag, aETag);
    }


// End of File
