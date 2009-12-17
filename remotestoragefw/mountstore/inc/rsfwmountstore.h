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
* Description:  Mount configuration repository management
 *
*/


#ifndef CRSFWMOUNTSTORE_H
#define CRSFWMOUNTSTORE_H

// INCLUDES
#include <cenrepnotifyhandler.h>

// DATA TYPES
// Event types for MMountStoreObserver
enum TMountStoreEvent
    {
    EMountStoreEventMountConfigurationChanged = 1
    };

// FORWARD DECLARATIONS
class CRepository;
class CDesC16Array;
class CRsfwMountEntry;

// CLASS DECLARATION
/**
 *  Interface for receiving mounting events
 *
 *  @lib mountman.dll
 *  @since Series 60 3.1
 */

class MRsfwMountStoreObserver
    {
public:
    /**
     * Handles an event emanating from a CRsfwMountStore class.
     * Used for notifying about changes in the configurations
     *
     * @param aEventType type of the event
     * @param aStatus status code
     * @param aArg miscellaneous arguments
     */
    virtual void HandleMountStoreEvent(TMountStoreEvent aEvent,
                                       TInt aStatus,
                                       TAny* aArg) = 0;
    };

// CLASS DECLARATION
/**
 *  Maintain permanent storage of mount configuration entries
 *
 *  @lib mountstore.lib
 *  @since Series 60 3.1
 */
class CRsfwMountStore: public CBase, public MCenRepNotifyHandlerCallback
    {
public: // Constructors and destructor
    /**
     * Two-phased constructor.
     * At the construction phase, all mount configurations are loaded
     * into memory (that is, LoadEntries() is called).
     * @param aMountStoreObserver store change notification handler
     */
    IMPORT_C static CRsfwMountStore* NewL(
        MRsfwMountStoreObserver* aMountStoreObserver);

    CRsfwMountStore();

    /**
     * Destructor.
     */
    IMPORT_C virtual ~CRsfwMountStore();

public: // New functions
    /**
     * (Re)loads the mount configurations into memory from the repository.
     * 
     */
    IMPORT_C void LoadEntriesL();

    /**
     * Gets a list of friendly names of all mount configurations stored
     * in Central Repository
     *
     * @param aNames returned information
     * @return nothing
     *  this array must be created by the caller
     */
    IMPORT_C void GetNamesL(CDesC16Array* aNames);
    
     /**
     * Gets remote drive letters of all mount configurations stored
     * in Central Repository (in correct order)
     *
     * @param aDriveList returned information
     * @return nothing
     *  this array must be created by the caller
     */
    IMPORT_C void GetDriveLettersL(TDriveList& aDriveList);


    /**
     * Finds the configuration configuration entry by drive letter.
     * @param aDrive drive letter
     * @return a pointer to the configuration entry or NULL if not found
     */
    IMPORT_C const CRsfwMountEntry* LookupEntryByDriveL(TChar aDriveLetter);

    /**
     * Finds the mount configuration entry by friendly name.
     * The ownership of the entry is not moved
     * @param aName friendly name
     * @return a pointer to the configuration entry or NULL if not found
     *  the ownership of the pointer is NOT transferred
     */
    IMPORT_C const CRsfwMountEntry* LookupEntryByNameL(const TDesC& aName);

    /**
     * Adds a new mount configuration entry in the list of entries.
     * If an entry with the same name already exists,
     * that entry is replaced with the new entry.
     * The entry is positioned among the existing entries
     * according to the EMountEntryItemIndex value
     * The drive letter in aMountEntry must be set before calling.
     * @param aMountEntry mount configuration entry -
     *  the ownership of the pointer is transferred to CMountStore
     * @leave KErrArgument drive letter not set
     * @leave KErrInUse all nine allowed remote drives are in use
     *
     */
    IMPORT_C void AddEntryL(CRsfwMountEntry* aMountEntry);

    /**
     * Removes a mount configuration entry from the list of entries.
     * Nothing is done if no entry with the name is found
     * @param aDriveLetter name of the entry to be removed
     */
    IMPORT_C void RemoveEntryL(const TDesC& aName);

    /**
     * Removes a mount configuration entry from the list of entries.
     * Nothing is done if no entry with the drive letter is found.
     * @param aDriveLetter name of the entry to be removed
     */
    IMPORT_C void RemoveEntryL(const TChar aDriveLetter);

    /**
     * Saves the mount configuration entries permanently.
     * This function must be called after changes have been made in the
     * mount configuration entries in order to store the cahnges permanently.
     */
    IMPORT_C void CommitL();
    
private:
    void ConstructL(MRsfwMountStoreObserver* aMountStoreObserver);
    void ReloadEntriesL();
    void AddToRepositoryL(const CRsfwMountEntry* aMountEntry);
    void RemoveFromRepositoryL(const CRsfwMountEntry* aMountEntry);
    void ClearRepositoryL();
    void SynchronizeIndexesL();
    // From MCenRepNotifyHandlerCallback
    void HandleNotifyError(TUint32 aId,
                           TInt aError,
                           CCenRepNotifyHandler* aHandler);
    void HandleNotifyGeneric(TUint32 aId);
    TInt MapDriveLetterToRecordIdL(const HBufC* drive);


private: // Data
    RPointerArray<CRsfwMountEntry> iMountEntries;
    CRepository*                   iRepository;
    CCenRepNotifyHandler*          iCenRepNotifyHandler;
    TBool                          iReceivingCenRepNotifications;
    TBool                          iCenRepChanged;
    MRsfwMountStoreObserver*       iMountStoreObserver;
    };

#endif // CRSFWMOUNTSTORE_H

// End of File
