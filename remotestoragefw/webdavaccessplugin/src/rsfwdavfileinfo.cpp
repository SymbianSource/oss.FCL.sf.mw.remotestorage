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
* Description:  Maintain WebDAV info about resources
 *
*/


// INCLUDE FILES
#include "rsfwdavfileinfo.h"

// ============================ MEMBER FUNCTIONS ==============================

CRsfwDavFileInfo* CRsfwDavFileInfo::NewL()
    {
    return new (ELeave) CRsfwDavFileInfo();
    }


CRsfwDavFileInfo::~CRsfwDavFileInfo()
    {
    delete iName;
    delete iLockToken;
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::Name
// ----------------------------------------------------------------------------
//
HBufC* CRsfwDavFileInfo::Name()
    {
    return iName;
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::SetNameL
// ----------------------------------------------------------------------------
//
void CRsfwDavFileInfo::SetNameL(const TDesC& aName)
    {
    SetL(iName, aName);
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::LockToken
// ----------------------------------------------------------------------------
//
HBufC8* CRsfwDavFileInfo::LockToken()
    {
    if (iFlags & TRsfwDavFileInfoFlags::EUnlockPending)
        {
        return NULL;
        }
    return iLockToken;
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::SetLockTokenL
// ----------------------------------------------------------------------------
//
void CRsfwDavFileInfo::SetLockTokenL(const TDesC8& aLockToken)
    {
    SetL(iLockToken, aLockToken);
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::ResetLockToken
// ----------------------------------------------------------------------------
//
void CRsfwDavFileInfo::ResetLockToken()
    {
    delete iLockToken;
    iLockToken = NULL;
    iFlags &= ~TRsfwDavFileInfoFlags::EUnlockPending;
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::Timeout
// ----------------------------------------------------------------------------
//
TUint CRsfwDavFileInfo::Timeout()
    {
    return iTimeout;
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::SetTimeout
// ----------------------------------------------------------------------------
//
void CRsfwDavFileInfo::SetTimeout(TUint aTimeout)
    {
    iTimeout = aTimeout;
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::IsFlag
// ----------------------------------------------------------------------------
//
TBool CRsfwDavFileInfo::IsFlag(TUint aFlag)
    {
    return iFlags & aFlag != 0;
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::SetFlag
// ----------------------------------------------------------------------------
//
void CRsfwDavFileInfo::SetFlag(TUint aFlag)
    {
    iFlags |= aFlag;
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::ResetFlag
// ----------------------------------------------------------------------------
//
void CRsfwDavFileInfo::ResetFlag(TUint aFlag)
    {
    iFlags &= ~aFlag;
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::SetL
// ----------------------------------------------------------------------------
//
void CRsfwDavFileInfo::SetL(HBufC8*& aDst, const TDesC8& aSrc)
    {
    if (!aDst)
        {
        aDst  = HBufC8::NewMaxL(aSrc.Length());
        *aDst = aSrc;
        }
    else if (aSrc.Length() > aDst->Length())
        {
        aDst  = aDst->ReAllocL(aSrc.Length()); // expand
        *aDst = aSrc;
        }
    else
        {
        *aDst = aSrc;
        if (aSrc.Length() < aDst->Length())
            {
            aDst = aDst->ReAllocL(aSrc.Length()); // reclaim space
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavFileInfo::SetL
// ----------------------------------------------------------------------------
//
void CRsfwDavFileInfo::SetL(HBufC*& aDst, const TDesC& aSrc)
    {
    if (!aDst)
        {
        aDst  = HBufC::NewMaxL(aSrc.Length());
        *aDst = aSrc;
        }
    else if (aSrc.Length() > aDst->Length())
        {
        aDst  = aDst->ReAllocL(aSrc.Length()); // expand
        *aDst = aSrc;
        }
    else
        {
        *aDst = aSrc;
        if (aSrc.Length() < aDst->Length())
            {
            aDst = aDst->ReAllocL(aSrc.Length()); // reclaim space
            }
        }
    }

//  End of File
