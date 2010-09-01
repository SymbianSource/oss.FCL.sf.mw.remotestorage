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
* Description:  Mount configuration entry
 *
*/


// INCLUDE FILES
#include <uri16.h>
#include <rsfwmountentry.h>
#include "rsfwmountutils.h"

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// CRsfwMountEntry::NewLC
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwMountEntry* CRsfwMountEntry::NewLC()
    {
    CRsfwMountEntry* self = new (ELeave) CRsfwMountEntry();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwMountEntry::NewL
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwMountEntry* CRsfwMountEntry::NewL()
    {
    CRsfwMountEntry* self = NewLC();
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwMountEntry::CRsfwMountEntry
// ----------------------------------------------------------------------------
//
CRsfwMountEntry::CRsfwMountEntry()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwMountEntry::~CRsfwMountEntry
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwMountEntry::~CRsfwMountEntry()
    {
    Clear();
    iFs.Close();
    }

// ----------------------------------------------------------------------------
// CRsfwMountEntry::SetItemL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountEntry::SetItemL(TInt aIndex, const TDesC& aValue)
    {
    TChar driveChar;
     
    if ((aIndex < EMountEntryItemIndex) || (aIndex >= EMountEntryItemCount))
        {
        User::Leave(KErrArgument);
        }

    switch (aIndex) 
        {
        case EMountEntryItemName:
            if (aValue.Length() > KMaxMountNameLength) 
                {
                User::Leave(KErrArgument);
                }      
            // name must not contain chars not allowed in Symbian file names
            // as the name will be stored as drive friendly name
            // (rules for allowed chars are the same)
            if (!(RsfwMountUtils::IsFriendlyNameValid(aValue))) 
                {
                User::Leave(KErrArgument);
                }
            break;
        case EMountEntryItemUri:
            if (aValue.Length() > KMaxMountUriLength)
                {
                User::Leave(KErrArgument);
                }
            break;
        case EMountEntryItemUserName:
            if (aValue.Length() > KMaxMountUserNameLength)
                {
                User::Leave(KErrArgument);
                }
            break;   
        case EMountEntryItemPassword:
             if (aValue.Length() > KMaxMountPasswordLength)
                {
                User::Leave(KErrArgument);
                } 
            break;
        case EMountEntryItemDrive:
            // remote drives can take drive letters from  J through Y
            // (see documentation for RFs::DriveList())
            driveChar = aValue[0];
            TInt driveNumber;
            User::LeaveIfError(iFs.CharToDrive(driveChar, driveNumber));
            if (!((driveNumber >= EDriveJ) && (driveNumber < EDriveZ)))
                {
                User::Leave(KErrArgument);
                }
            break;
        default:
            // check that the value is not too long
            if (aValue.Length() > KMaxMountConfItemLength) 
                {
                User::Leave(KErrArgument);
                }
            break;            
        }
        
    HBufC*& p = iMountEntryItems[aIndex];
    if (p)
        {
        delete p;
        p = NULL;
        }
    p = aValue.AllocL();
    }

// ----------------------------------------------------------------------------
// CRsfwMountEntry::SetItemL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountEntry::SetItemL(TInt aIndex, const TDesC8& aValue)
    {
    HBufC* p = HBufC::NewLC(aValue.Length());
    TPtr pPtr = p->Des();
    pPtr.Copy(aValue);
    SetItemL(aIndex, pPtr);
    CleanupStack::PopAndDestroy(p);
    }

// ----------------------------------------------------------------------------
// CRsfwMountEntry::SetEntryL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountEntry::SetEntryL(TInt aIndex,
                                     const TDesC& aName,
                                     TChar aDriveLetter,
                                     const TDesC& aUri,
                                     const TDesC& aUserName,
                                     const TDesC& aPassword,
                                     const TDesC& aIap)
    {
    TBuf<KIndexAsStringLength> index;
    index.Num(aIndex);
    SetItemL(EMountEntryItemIndex, index);
    SetItemL(EMountEntryItemName, aName);
    TBuf<KIndexAsStringLength> drive;
    drive.Append(aDriveLetter);
    SetItemL(EMountEntryItemDrive, drive);
    SetItemL(EMountEntryItemUri, aUri);
    SetItemL(EMountEntryItemUserName, aUserName);
    SetItemL(EMountEntryItemPassword, aPassword);
    SetItemL(EMountEntryItemIap, aIap);
    }

// ----------------------------------------------------------------------------
// CRsfwMountEntry::Item
// ----------------------------------------------------------------------------
//
EXPORT_C const HBufC* CRsfwMountEntry::Item(TInt aIndex) const
    {
    if ((aIndex >= EMountEntryItemIndex) && (aIndex < EMountEntryItemCount))
        {
        return iMountEntryItems[aIndex];
        }
    return NULL;
    }

// ----------------------------------------------------------------------------
// CRsfwMountEntry::Clear
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountEntry::Clear()
    {
    TInt i;
    for (i = EMountEntryItemIndex;  i < EMountEntryItemCount;  i++)
        {
        if (iMountEntryItems[i])
            {
            delete iMountEntryItems[i];
            iMountEntryItems[i] = NULL;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountEntry::CloneL
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwMountEntry* CRsfwMountEntry::CloneL() const
    {
    CRsfwMountEntry* entry = CRsfwMountEntry::NewLC();
    TInt i;
    for (i = EMountEntryItemIndex;  i < EMountEntryItemCount;  i++)
        {
        HBufC* item = iMountEntryItems[i];
        if (item)
            {
            entry->iMountEntryItems[i] = (*item).AllocL();
            }
        }
    CleanupStack::Pop(entry);
    return entry;
    }

// ----------------------------------------------------------------------------
// CRsfwMountEntry::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwMountEntry::ConstructL()
    {
    User::LeaveIfError(iFs.Connect());
    }

//  End of File
