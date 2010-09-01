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
#include "rsfwdirent.h"
#include "rsfwdirentattr.h"

// ======================== CRsfwDirEnt MEMBER FUNCTIONS ==========================

// ----------------------------------------------------------------------------
// CRsfwDirEnt::NewLC
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwDirEnt* CRsfwDirEnt::NewLC(const TDesC& aName, CRsfwDirEntAttr* aAttr)
    {
    CRsfwDirEnt* self = new (ELeave) CRsfwDirEnt();
    CleanupStack::PushL(self);
    self->ConstructL(aName, aAttr);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::NewLC
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwDirEnt* CRsfwDirEnt::NewLC(const TDesC8& aName, CRsfwDirEntAttr* aAttr)
    {
    CRsfwDirEnt* self = new (ELeave) CRsfwDirEnt();
    CleanupStack::PushL(self);
    self->Construct8L(aName, aAttr);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::NewL
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwDirEnt* CRsfwDirEnt::NewL(const TDesC& aName, CRsfwDirEntAttr* aAttr)
    {
    CRsfwDirEnt* self = NewLC(aName, aAttr);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::NewL
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwDirEnt* CRsfwDirEnt::NewL(const TDesC8& aName, CRsfwDirEntAttr* aAttr)
    {
    CRsfwDirEnt* self = NewLC(aName, aAttr);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::CRsfwDirEnt
// ----------------------------------------------------------------------------
//
CRsfwDirEnt::CRsfwDirEnt()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::~CRsfwDirEnt
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwDirEnt::~CRsfwDirEnt()
    {
    delete iName;
    if (!iNotOwnAttr)
        {
        delete iAttr;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::Name
// ----------------------------------------------------------------------------
//
EXPORT_C const HBufC* CRsfwDirEnt::Name() const
    {
    return iName;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::GetName
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEnt::GetName(TDes& aName) const
    {
    aName.Copy(*iName);
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::GetName
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEnt::GetName(TDes8& aName) const
    {
    aName.Copy(*iName);
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::SetNameL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEnt::SetNameL(const TDesC& aName) 
    {
    if (iName)
        {
        delete iName;
        iName = NULL;
        }
    iName = aName.AllocL();
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::SetNameL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEnt::SetNameL(const TDesC8& aName) 
    {
    if (iName)
        {
        delete iName;
        iName = NULL;
        }
    iName = HBufC::NewL(aName.Length());
    TPtr namePtr = iName->Des();
    namePtr.Copy(aName);
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::Attr
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwDirEntAttr* CRsfwDirEnt::Attr() const
    {
    return iAttr;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::ExtractAttr
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwDirEntAttr* CRsfwDirEnt::ExtractAttr()
    {
    iNotOwnAttr = ETrue;
    return iAttr;
    }

// ----------------------------------------------------------------------------
// CRsfwDirEnt::SetAttrL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwDirEnt::SetAttrL(CRsfwDirEntAttr* aAttr)
    {
    if (iAttr && !iNotOwnAttr)
        {
        delete iAttr;
        }
    if (aAttr)
        {
        iAttr = aAttr;
        }
    else
        {
        iAttr = CRsfwDirEntAttr::NewL();
        }
    iNotOwnAttr = EFalse;
    }

void CRsfwDirEnt::ConstructL(const TDesC& aName, CRsfwDirEntAttr* aAttr)
    {
    SetNameL(aName);
    SetAttrL(aAttr);
    }

void CRsfwDirEnt::Construct8L(const TDesC8& aName, CRsfwDirEntAttr* aAttr)
    {
    SetNameL(aName);
    SetAttrL(aAttr);
    }

// End of File
