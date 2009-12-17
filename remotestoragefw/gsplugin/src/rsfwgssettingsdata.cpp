/*
* Copyright (c) 2005 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Rsfw GS plugin data holding class for Rsfw setting list
*
*/


// INCLUDE FILES
#include "rsfwgssettingsdata.h"

_LIT(KDefaultAddress, "https://");

// -----------------------------------------------------------------------------
// CRsfwGsSettingsData::NewL
// -----------------------------------------------------------------------------
//
CRsfwGsSettingsData *CRsfwGsSettingsData::NewL()
    {
    CRsfwGsSettingsData *self = CRsfwGsSettingsData::NewLC();
    CleanupStack::Pop(self);
    return self;
    }

// -----------------------------------------------------------------------------
// CRsfwGsSettingsData::NewLC()
// -----------------------------------------------------------------------------
//
CRsfwGsSettingsData *CRsfwGsSettingsData::NewLC()
    {
    CRsfwGsSettingsData *self = new (ELeave) CRsfwGsSettingsData();
    CleanupStack::PushL(self);

    self->ConstructL();

    return self;
    }

// -----------------------------------------------------------------------------
// CRsfwGsSettingsData::~CRsfwGsSettingsData()
// -----------------------------------------------------------------------------
//
CRsfwGsSettingsData::~CRsfwGsSettingsData()
    {
    }

// -----------------------------------------------------------------------------
// CRsfwGsSettingsData::CRsfwGsSettingsData()
// -----------------------------------------------------------------------------
//
CRsfwGsSettingsData::CRsfwGsSettingsData()
    {
    // initialise local data
    Reset();
    }

// -----------------------------------------------------------------------------
// CRsfwGsSettingsData::ConstructL() 
// -----------------------------------------------------------------------------
//
void CRsfwGsSettingsData::ConstructL() 
    {
    iURL = KNullDesC;
    iURL.Copy(KDefaultAddress);
    }

// -----------------------------------------------------------------------------
// CRsfwGsSettingsData::Reset()
// -----------------------------------------------------------------------------
//
void CRsfwGsSettingsData::Reset()
    {
    iSettingName = KNullDesC;
    iAccessPoint = KErrNotFound;
    iAccessPointDes = KNullDesC;
    iURL = KNullDesC;
    iURL.Copy(KDefaultAddress);
    iUserID= KNullDesC;
    iPassword = KNullDesC;
    iAccessPointName = KNullDesC; 
    iInActivityTimeout.Num(KDefaultInactivityTimeout);
    iDriveLetter = 0;
    }

//  End of File

