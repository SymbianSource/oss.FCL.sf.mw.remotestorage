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
* Description:  Access mount configuration storage
 *
*/


// In order to use the central repository,
// The repository initialization file must be put in
// [epoc32/release/winscw/udeb/]z/private/10202be9/101F9775.txt

// The mount configurations form a table where
// individual configuration entries form the rows and
// the entry items form the columns.
// - rows are indexed with the upper 24 bits of the Id, starting from 1.
// - columns are indexed by the lower 8 bits of the Id, starting from 1.
//
// Row number 0 is reserved for system config name/value pairs (see RsfwConfig)

// INCLUDES
#include <centralrepository.h>
#include <badesca.h>
#include <aknnotewrappers.h> 
#include <rsfwmountman.h>
#include <rsfwmountentry.h>
#include <rsfwmountentryitem.h>

#include "rsfwmountstore.h"


// CONSTANTS
const TUid  KCRUidRsfwCtrl    = { 0x101F9775 };

// Current mount configuration repository version
const TInt  KCurrentVersion   = 1000;
const TUint KCurrentVersionId = 0xff;

const TInt  KColumnMaskSize   = 8;
const TUint KRowMask          = 0xffffff00;
const TUint KColumnMask       = 0x000000ff;

const TUint KColumnName       = EMountEntryItemName;

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// CRsfwMountStore::NewL
// Two-phased constructor.
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwMountStore* CRsfwMountStore::NewL(
    MRsfwMountStoreObserver* aMountStoreObserver)
    {
    CRsfwMountStore* self = new (ELeave) CRsfwMountStore();
    CleanupStack::PushL(self);
    self->ConstructL(aMountStoreObserver);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::CRsfwMountStore
// ----------------------------------------------------------------------------
//
CRsfwMountStore::CRsfwMountStore()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwMountStore::ConstructL(MRsfwMountStoreObserver* aMountStoreObserver)
    {
    iMountStoreObserver = aMountStoreObserver;
    // if CenRep not found, just leave
    iRepository = CRepository::NewL(KCRUidRsfwCtrl);

    // Check version
    TInt version;
    // allow versionless repositories until the version id is
    // included in the backup/restore scheme.
    if (iRepository->Get(KCurrentVersionId, version) == KErrNone)
        {
        if (version != KCurrentVersion)
            {
            // This causes the repository to be cleared and
            // stamped with the current version
            CommitL();
            }
        }
    LoadEntriesL();

    // Start listening to changes in the repository
    iCenRepNotifyHandler = CCenRepNotifyHandler::NewL(*this, *iRepository);
    iCenRepNotifyHandler->StartListeningL();
    iReceivingCenRepNotifications = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::~CRsfwMountStore
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwMountStore::~CRsfwMountStore()
    {
    iMountEntries.ResetAndDestroy();
    if (iCenRepNotifyHandler) 
      {
      iCenRepNotifyHandler->StopListening();
      delete iCenRepNotifyHandler;
      }
    delete iRepository;
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::LoadEntriesL
// CenRep keeps data in alphabetical order, and we want also iMountEntries to
// to be filled in the same order
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountStore::LoadEntriesL()
    {
    iMountEntries.ResetAndDestroy();

    // go through the records in CenRep and populate the array
    RArray<TUint32> nameIds;
    CleanupClosePushL(nameIds);
    User::LeaveIfError(iRepository->FindL(KColumnName, KColumnMask, nameIds));		
   	HBufC* item = HBufC::NewLC(KMaxMountConfItemLength);
   	TPtr itemPtr = item->Des();
   	TInt row;
	for (row = 0; row < nameIds.Count(); row++)
    	{
    	TUint rowId = (nameIds[row] & KRowMask);
    	// don't touch record number 0, as it stores RSFW general data
    	if (rowId > 0)
        	{
        	CRsfwMountEntry* entry = CRsfwMountEntry::NewLC();
        	TUint i;
        	for (i = EMountEntryItemIndex; i < EMountEntryItemCount; i++)
            	{
            	TInt err = iRepository->Get(rowId | i, itemPtr);
                if ( err != KErrNone && err != KErrNotFound )
                    {
                    User::Leave(err);
                    }
           	    entry->SetItemL(i, itemPtr);	
            	}
            User::LeaveIfError(iMountEntries.Append(entry));
            // ownership's been taken by the array
            CleanupStack::Pop(entry);
        	}
    	}
    CleanupStack::PopAndDestroy(item);
    CleanupStack::PopAndDestroy(&nameIds);
    
    // now mount entries array is up-to-date
    iCenRepChanged = EFalse;
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::GetNamesL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountStore::GetNamesL(CDesC16Array* aNames)
    {
    ReloadEntriesL();
    aNames->Reset();
    HBufC* name = HBufC::NewLC(KMaxMountConfItemLength);
    TPtr namePtr = name->Des();
    TInt i;
    for (i = 0; i < iMountEntries.Count(); i++)
        {
        namePtr.Copy(*iMountEntries[i]->Item(EMountEntryItemName));
        aNames->AppendL(namePtr);
        }
    CleanupStack::PopAndDestroy(name);
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::GetDriveLettersL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountStore::GetDriveLettersL(TDriveList& aDriveList)
    {
    ReloadEntriesL();
    aDriveList.Zero();
    HBufC* name = HBufC::NewLC(KMaxMountConfItemLength);
    TPtr namePtr = name->Des();
    TInt i;
    for (i = 0; i < iMountEntries.Count(); i++)
        {
        namePtr.Copy(*iMountEntries[i]->Item(EMountEntryItemDrive));
        aDriveList.Append(namePtr);
        }
    CleanupStack::PopAndDestroy(name);
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::LookupEntryByName
// ----------------------------------------------------------------------------
//
EXPORT_C const CRsfwMountEntry* CRsfwMountStore::LookupEntryByNameL(const TDesC& aName)
    {
    ReloadEntriesL();
    TInt i;
    for (i = 0; i < iMountEntries.Count(); i++)
        {
        CRsfwMountEntry* entry = iMountEntries[i];
        const HBufC* name = entry->Item(EMountEntryItemName);
        if (name && name->CompareF(aName) == 0)
            {
            return entry;
            }
        }
    return NULL;
    }
 
// ----------------------------------------------------------------------------
// CRsfwMountStore::LookupEntryByDrive
// ----------------------------------------------------------------------------
//
EXPORT_C const CRsfwMountEntry* CRsfwMountStore::LookupEntryByDriveL(TChar aDriveLetter)
    {
    ReloadEntriesL();
    TBuf<1> driveBuf;
    driveBuf.Append(aDriveLetter);
    TInt i;
    for (i = 0; i < iMountEntries.Count(); i++)
        {
        CRsfwMountEntry* entry = iMountEntries[i];
        const HBufC* drive = entry->Item(EMountEntryItemDrive);
        if (drive && drive->CompareF(driveBuf) == 0)
            {
            return entry;
            }
        }
    return NULL;
    }
 
// ----------------------------------------------------------------------------
// CRsfwMountStore::AddEntryL
// Adds entry to CenRep, replaces existing item, so it can be used 
// for edit operation as well
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountStore::AddEntryL(CRsfwMountEntry* aMountEntry)
    {
    // Take ownership
    CleanupStack::PushL(aMountEntry);

    if ((!aMountEntry) || 
        (!aMountEntry->Item(EMountEntryItemDrive)))
        {
        User::Leave(KErrArgument);
        }

    ReloadEntriesL();

    TPtrC drive(*aMountEntry->Item(EMountEntryItemDrive));

    // Check whether an entry with given drive letter exists.
    // If so, delete it and remember index of deleted item.
    // Otherwise find the index at which a new item should be added.
    // Keep in mind the order is aphabetical.
    TInt i;
    TInt index = -1;
    for (i = 0; i < iMountEntries.Count(); i++)
        {
        CRsfwMountEntry* entry = iMountEntries[i];
        TInt cmp = entry->Item(EMountEntryItemDrive)->CompareF(drive);
        if (cmp == 0)
            {
            // Replace an existing entry
            RemoveFromRepositoryL(entry);
            delete entry;
            entry = NULL;
            iMountEntries.Remove(i);
            // save the position, new item will be added here
            index = i;
            break;
            }
        else if (cmp > 0)
            {
            // we've found the entry whose drive letter is larger, which
            // means we've found the place where a new entry should be added
            index = i;
            break;
            }
        }
    
    // before we add a new entry, make sure that we don't exceed max drives allowed
    if (iMountEntries.Count() >= KMaxRemoteDrives)    
        {
        User::Leave(KErrInUse);
        }

    // add entry to the array
    if (index < 0)
        {
        // this means the drive letter of newly added entry is the latest 
        // in the alphabet. hence just append the entry
        User::LeaveIfError(iMountEntries.Append(aMountEntry));        
        }
    else
        {
        User::LeaveIfError(iMountEntries.Insert(aMountEntry, index));
        }


    // add to repository  
    AddToRepositoryL(aMountEntry);

    // cleanup
    CleanupStack::Pop(aMountEntry);
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::RemoveEntryL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountStore::RemoveEntryL(const TDesC& aName)
    {
    ReloadEntriesL();
    TInt i;
    for (i = 0; i < iMountEntries.Count(); i++)
        {
        CRsfwMountEntry* entry = iMountEntries[i];
        const HBufC* name = entry->Item(EMountEntryItemName);
        if (name && name->CompareF(aName) == 0)
            {
            RemoveFromRepositoryL(entry);
            iMountEntries.Remove(i);
            delete entry;
            break;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::RemoveEntryL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountStore::RemoveEntryL(TChar aDrive)
    {
    ReloadEntriesL();
    TBuf<1> driveBuf;
    driveBuf.Append(aDrive);
    TInt i;
    for (i = 0; i < iMountEntries.Count(); i++)
        {
        CRsfwMountEntry* entry = iMountEntries[i];
        const HBufC* drive = entry->Item(EMountEntryItemDrive);
        if (drive && drive->CompareF(driveBuf) == 0)
            {
            RemoveFromRepositoryL(entry);
            iMountEntries.Remove(i);
            delete entry;
            break;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::CommitL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwMountStore::CommitL()
    {
    if (iRepository) 
        {
        // We don't want to get reports of our own changes
        iCenRepNotifyHandler->StopListening();
        ClearRepositoryL();
        TInt i;
        for (i = 0; i < iMountEntries.Count(); i++)
            {
            CRsfwMountEntry* entry = iMountEntries[i];
            AddToRepositoryL(entry);
            }
        // Set correct version id
        User::LeaveIfError(iRepository->Set(KCurrentVersionId, KCurrentVersion));
        // Restart listening
        iCenRepNotifyHandler->StartListeningL();
        }
    else 
        {
        _LIT(KWarningText, "Cannot store entries permanently!");
        TRAP_IGNORE(CAknWarningNote* note = new (ELeave) CAknWarningNote;
                    note->ExecuteLD(KWarningText);
            );
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::AddToRepositoryL
// ----------------------------------------------------------------------------
//
void CRsfwMountStore::AddToRepositoryL(const CRsfwMountEntry* aMountEntry)
    {
    // retrieve drive letter
    const HBufC* drive = aMountEntry->Item(EMountEntryItemDrive);

    // calculate record ID for the drive letter
    TInt recordId = MapDriveLetterToRecordIdL(drive);

    // write down the entry settings to given record ID
    TInt i = 0;
    for (i = EMountEntryItemIndex; i < EMountEntryItemCount; i++)
        {
        TInt fieldId = (recordId << KColumnMaskSize) | i;
        const HBufC* item = aMountEntry->Item(i);
        if (item)
            {
            User::LeaveIfError(iRepository->Create(fieldId, *item));
            }
        else
            {
            TPtrC null;
            User::LeaveIfError(iRepository->Create(fieldId, null));
            }
        }
    }    

// ----------------------------------------------------------------------------
// CRsfwMountStore::RemoveFromRepositoryL
// ----------------------------------------------------------------------------
//
void CRsfwMountStore::RemoveFromRepositoryL(const CRsfwMountEntry* aMountEntry)
    {
    // retrieve drive letter
    const HBufC* drive = aMountEntry->Item(EMountEntryItemDrive);

    // calculate record ID for the drive letter
    TInt recordId = MapDriveLetterToRecordIdL(drive);

    // delete settings from given record ID
    TInt i = 0;
    for (i = EMountEntryItemIndex; i < EMountEntryItemCount; i++)
        {
        TInt fieldId = (recordId << KColumnMaskSize) | i;
        User::LeaveIfError(iRepository->Delete(fieldId));
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::ClearRepositoryL
// ----------------------------------------------------------------------------
//
void CRsfwMountStore::ClearRepositoryL()
    {
    // find all non-empty records
    RArray<TUint32> nameIds;
    CleanupClosePushL(nameIds);
    User::LeaveIfError(iRepository->FindL(KColumnName, KColumnMask, nameIds));
    
    TInt record;
    for (record = 0; record < nameIds.Count(); record++)
        {
        TUint recordId = (nameIds[record] & KRowMask);
        // don't touch record number 0, as it stores RSFW general data
        if (recordId > 0)
            {
            TInt i;
            for (i = EMountEntryItemIndex; i < EMountEntryItemCount; i++)
                {
                TUint fieldId = (recordId | i);
                User::LeaveIfError(iRepository->Delete(fieldId));
                }            
            }
        }
    CleanupStack::PopAndDestroy(&nameIds);
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::ReloadEntriesL
// ----------------------------------------------------------------------------
//
void CRsfwMountStore::ReloadEntriesL()
    {
    if (!iReceivingCenRepNotifications || iCenRepChanged)
        {
        // We only need reload if it is not done automatically
        // through notifications
        LoadEntriesL();
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::HandleNotifyGeneric
// ----------------------------------------------------------------------------
//
void CRsfwMountStore::HandleNotifyGeneric(TUint32 /* aId */)
    {
    iCenRepChanged = ETrue;
    if (iMountStoreObserver)
        {
        iMountStoreObserver->HandleMountStoreEvent(
            EMountStoreEventMountConfigurationChanged,
            0,
            NULL);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::HandleNotifyError
// ----------------------------------------------------------------------------
//
void CRsfwMountStore::HandleNotifyError(TUint32 /* aId */,
                                    TInt /* aError */, 
                                    CCenRepNotifyHandler* /* aHandler */)
    {
    iReceivingCenRepNotifications = EFalse;
    }

// ----------------------------------------------------------------------------
// CRsfwMountStore::MapDriveLetterToRecordIdL
// drive with letter 'J' will take record number 1
// drive with letter 'K' will take record number 2
// etc.
// (remember that record number 0 stores RSFW general data, so don't touch it)
// ----------------------------------------------------------------------------
//
TInt CRsfwMountStore::MapDriveLetterToRecordIdL(const HBufC* drive)
    {
    if (!drive || !drive->Length())
        {
        User::Leave(KErrArgument);
        }
        
    // we have to convert HBufC to TChar and then TChar to TInt
    TChar driveChar = (*drive)[0];        
    TInt driveNumber;
    RFs fs;
	User::LeaveIfError(fs.Connect());
	TInt err = fs.CharToDrive(driveChar, driveNumber);
	fs.Close();
    User::LeaveIfError(err);

    // count record ID based on drive number
    // +1 is to omit record number 0!
    return driveNumber - EDriveJ + 1;
    }

//  End of File
