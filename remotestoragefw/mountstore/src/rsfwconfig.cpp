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
* Description:  Rsfw operational parameter config using central repository
 *
*/


// INCLUDE FILES
#include <centralrepository.h>

#include "rsfwconfig.h"


// =========================== MEMBER FUNCTIONS ===============================
// ----------------------------------------------------------------------------
// CRsfwConfig::ConstructL
// Symbian 2nd phase constructor can leave.
// ----------------------------------------------------------------------------
//
void CRsfwConfig::ConstructL(TUid aRepositoryUid)
    {
    TRAPD(err, iRepository = CRepository::NewL(aRepositoryUid));
    if (err) 
        {
        iRepository = NULL;
        }
    }

// ----------------------------------------------------------------------------
// RsfwConfig::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwConfig* CRsfwConfig::NewL(TUid aRepositoryUid)
    {
    CRsfwConfig* self = new (ELeave) CRsfwConfig();
    CleanupStack::PushL(self);
    self->ConstructL(aRepositoryUid);
    CleanupStack::Pop(self);
    return self;
    }

// Destructor
CRsfwConfig::~CRsfwConfig()
    {
    delete iRepository;
    }

// ----------------------------------------------------------------------------
// CRsfwConfig::Set
// See RsfwConfig.h
// ----------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwConfig::Set(TUint aId, TInt& aValue)
    {
    if (iRepository) 
        {
        return iRepository->Set(aId, aValue);
        }
    else 
        {
        return KErrNotFound;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwConfig::Set
// See RsfwConfig.h
// ----------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwConfig::Set(TUint aId, TDes& aValue)
    {
    if (iRepository) 
        {
        return iRepository->Set(aId, aValue);
        }
    else 
        {
        return KErrNotFound;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwConfig::Get
// See RsfwConfig.h
// ----------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwConfig::Get(TUint aId, TInt& aValue)
    {
    if (iRepository) 
        {
        return iRepository->Get(aId, aValue);
        }
    else 
        {
        return KErrNotFound;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwConfig::Get
// See RsfwConfig.h
// ---------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwConfig::Get(TUint aId, TDes& aValue)
    {
    if (iRepository) 
        {
        return iRepository->Get(aId, aValue);
        }
    else 
        {
        return KErrNotFound;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwConfig::IsTrue
// See RsfwConfig.h
// ----------------------------------------------------------------------------
//
EXPORT_C TBool CRsfwConfig::IsTrue(TUint aId)
    {
    if (iRepository) 
        {
        TBuf<KMaxRsfwConfItemLength> value;
        TInt err = iRepository->Get(aId, value);
        if (err == KErrNone)
            {
            TChar c = value[0];
            c.UpperCase();
            if ((c == '1') || (c == 'Y') || (c == 'T'))
                {
                return ETrue;
                }
            }   
        }
    return EFalse;
    }

// ----------------------------------------------------------------------------
// CRsfwConfig::IsFalse
// See RsfwConfig.h
// ----------------------------------------------------------------------------
//
EXPORT_C TBool CRsfwConfig::IsFalse(TUint aId)
    {
    if (iRepository) 
        {
        TBuf<KMaxRsfwConfItemLength> value;
        TInt err = iRepository->Get(aId, value);
        if (err == KErrNone)
            {
            TChar c = value[0];
            c.UpperCase();
            if ((c == '0') || (c == 'N') || (c == 'F'))
                {
                return ETrue;
                }
            }   
        }
    return EFalse;
    }

// End of File
