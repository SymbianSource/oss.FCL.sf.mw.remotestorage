/*
* Copyright (c) 2004-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Lock manager for locked remote files
*
*/


#include "rsfwfileentry.h"
#include "rsfwrfestatemachine.h"
#include "rsfwlockmanager.h"
#include "rsfwfileentry.h"
#include "rsfwconfig.h"
#include "mdebug.h"

// ----------------------------------------------------------------------------

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// CRsfwLockManager::NewL
// ----------------------------------------------------------------------------
//
CRsfwLockManager* CRsfwLockManager::NewL(CRsfwRemoteAccess* aRemoteAccess)
    {
    CRsfwLockManager* self = CRsfwLockManager::NewLC(aRemoteAccess);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwLockManager::NewLC
// ----------------------------------------------------------------------------
//
CRsfwLockManager* CRsfwLockManager::NewLC(CRsfwRemoteAccess* aRemoteAccess)
    {
    DEBUGSTRING(("CRsfwLockManager::NewLC"));
    CRsfwLockManager* self = new (ELeave) CRsfwLockManager();
    CleanupStack::PushL(self);
    self->ConstructL(aRemoteAccess);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwLockManager::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwLockManager::ConstructL(CRsfwRemoteAccess* aRemoteAccess)
    {
    iRemoteAccess = aRemoteAccess;
    }

// ----------------------------------------------------------------------------
// CRsfwLockManager::~CRsfwLockManager
// ----------------------------------------------------------------------------
//
CRsfwLockManager::~CRsfwLockManager()
    {
    iLockRefreshContexts.Close();

    // set unlock flag for all the entries locked
    while (iLockedEntries.Count() > 0)
        {
        CRsfwFileEntry* entry = iLockedEntries[0];
        iLockedEntries.Remove(0);
        // note that RemoveLocked will call CRsfwLockManager::RemoveLockedEntry
        entry->RemoveLocked();
        }
    iLockedEntries.Close();    
    }


// ----------------------------------------------------------------------------
// CRsfwLockManager::HandleRemoteAccessResponse
// For handling the response from RefreshLockL().
// If the lock refresh is successful we restart the timer
// as the server may have changed the timeout.
// If the refresh request returns an error we remove the lock
// We assume that server does not hold the lock anymore,
// so we cannot re-acquire it simply by trying to refresh it again.
// Instead, we should do a fresh lock operation
// ----------------------------------------------------------------------------
//
void CRsfwLockManager::HandleRemoteAccessResponse(TUint aId,
                                              TInt aStatus)
    {
    DEBUGSTRING(("CRsfwLockManager::HandleRemoteAccessResponse id: %d, status: %d", aId, aStatus));
    TPendingLockRefreshContext lockRefresh;
    lockRefresh.iId = aId;
    TInt index = iLockRefreshContexts.Find(lockRefresh);
    if (index != KErrNotFound) 
        {
        lockRefresh = iLockRefreshContexts[index];
        if (aStatus == KErrNone) 
            {
            // Note that this can leave only when creating the timer
            // so it shouldn't really leave anymore at this point.
            // Also resetting the timer and calling Start() do not even
            // return an error, so there is no need to examine err or ret value
            TRAP_IGNORE(lockRefresh.iFileEntry->SetLockedL(this, NULL));
            }
        else 
            {
            lockRefresh.iFileEntry->RemoveLocked();
            }
        iLockRefreshContexts.Remove(index);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwLockManager::ObtainLockL
// ----------------------------------------------------------------------------
//
void CRsfwLockManager::ObtainLockL(CRsfwFileEntry *aFileEntry,
                               TUint aLockFlags,
                               TDesC8*& aLockToken,
                               CRsfwRfeStateMachine* aOperation)
    {
    DEBUGSTRING(("CRsfwLockManager::ObtainLockL"));
    if (aFileEntry->iLockTimeout == 0)
        {
        // No locking wanted,
        // we use notsupported as a return code in this case too....
        DEBUGSTRING(("lock timeout in CFileEntry is 0, no locking"));
        aOperation->HandleRemoteAccessResponse(0, KErrNotSupported); 
        }
    else 
        {
        HBufC* fullName = aFileEntry->FullNameLC();
         if (!iRemoteAccess) 
            {
            DEBUGSTRING(("iRemoteAccess NULL"));
            User::Leave(KErrNotReady);
            }
        else 
            {
            DEBUGSTRING(("calling iRemoteAccess::ObtainLockL()"));
            iRemoteAccess->ObtainLockL(*fullName,
                                   aLockFlags,
                                   aFileEntry->iLockTimeout,
                                   aLockToken,
                                   aOperation);
            }

        CleanupStack::PopAndDestroy(fullName); // fullname  
        }
    
    }

// ----------------------------------------------------------------------------
// CRsfwLockManager::ReleaseLockL
// ----------------------------------------------------------------------------
//
void CRsfwLockManager::ReleaseLockL(CRsfwFileEntry* aFileEntry,
                                CRsfwRfeStateMachine* aOperation)
    {
    DEBUGSTRING(("CRsfwLockManager::ReleaseLockL"));
    if (aFileEntry->iLockTimeout == 0)
        {
        // No locking
        User::Leave(KErrNotFound);
        }
    

    if (!iRemoteAccess) 
        {
        User::Leave(KErrNotReady);
        }
    else 
        {
        HBufC* fullName = aFileEntry->FullNameLC();
#ifdef _DEBUG
        TInt err;
        err = iRemoteAccess->ReleaseLockL(*fullName, aOperation);
        TPtrC p = fullName->Des();
        DEBUGSTRING16(("ReleaseLockL(): returned %d for file '%S'", err, &p));     



#else        
        iRemoteAccess->ReleaseLockL(*fullName, aOperation);
#endif
        CleanupStack::PopAndDestroy(fullName); // fullname
        }
    
    }

// ----------------------------------------------------------------------------
// CRsfwLockManager::RefreshLockL
// ----------------------------------------------------------------------------
//
void CRsfwLockManager::RefreshLockL(CRsfwFileEntry* aFileEntry)
    {
   DEBUGSTRING(("CRsfwLockManager::RefreshLockL"));
    TInt id = 0;
    if (aFileEntry->iLockTimeout > 0) // timeout = 0 indicates no locking
        {
        aFileEntry->iLockTimer->Cancel(); // cancel the old timer
        HBufC* fullName = aFileEntry->FullNameLC();
        TRAPD(err, id = iRemoteAccess->RefreshLockL(*fullName,
                                                    aFileEntry->iLockTimeout,
                                                    this)); 
        if (err == KErrNone) 
            {
            TPendingLockRefreshContext lockRefresh;
            lockRefresh.iId = id;
            lockRefresh.iFileEntry = aFileEntry;
            iLockRefreshContexts.AppendL(lockRefresh);
            } 
        else 
            {
            // This error would come from the lower layers of the communication
            // stack, not from the server.
            // We use the timer mechanism to try again
            // but set the timeout to smaller.
            // Note that we don't touch aFileEntry->iLockTimer, 
            // which will be used to set the timeout requested from the server
            TInt lockTimeout =
                Min((aFileEntry->iLockTimeout / KLockRefreshAdjustment) / 2,
                    KMinLockRefreshAttempt);
            TCallBack callBack(CRsfwFileEntry::LockTimerExpiredL, this);
            aFileEntry->iLockTimer->Start(1000000 * lockTimeout,
                                          1000000 * lockTimeout,
                                          callBack);
            }
        CleanupStack::PopAndDestroy(fullName); // fullname
        }
    }
  
// ----------------------------------------------------------------------------
// CRsfwLockManager::LockedCount
// ----------------------------------------------------------------------------
//  
TInt CRsfwLockManager::LockedCount()
    {
    return iLockedEntries.Count();
    }

// ----------------------------------------------------------------------------
// CRsfwLockManager::AddLockedEntryL
// ----------------------------------------------------------------------------
//
void CRsfwLockManager::AddLockedEntryL(CRsfwFileEntry* aEntry)
    {
    // prevent from adding the same item twice
    if (iLockedEntries.Find(aEntry) != KErrNotFound)
        {
        return;
        }
    
    iLockedEntries.AppendL(aEntry);
    DEBUGSTRING(("Update locked count %d -> %d",
                 iLockedEntries.Count() - 1,
                 iLockedEntries.Count()));
    }

// ----------------------------------------------------------------------------
// CRsfwLockManager::RemoveLockedEntry
// ----------------------------------------------------------------------------
//
void CRsfwLockManager::RemoveLockedEntry(CRsfwFileEntry* aEntry)
    {
    TInt index = iLockedEntries.Find(aEntry);
    if (index != KErrNotFound)
        {
        iLockedEntries.Remove(index);        
        DEBUGSTRING(("Update locked count %d -> %d",
                 iLockedEntries.Count() + 1,
                 iLockedEntries.Count()));

        }
    }

// ----------------------------------------------------------------------------
// CRsfwLockManager::PopulateExternalLockTokenCacheL
// ----------------------------------------------------------------------------
//
void CRsfwLockManager::PopulateExternalLockTokenCacheL(CRsfwFileEntry* aRoot)
    {
    if (aRoot)
        {
        const TDesC8* lockToken = aRoot->LockToken();
        if (lockToken)
            {
            HBufC* path = aRoot->FullNameLC();
            TPtr pathPtr = path->Des();
            if (aRoot->Type() == KNodeTypeDir)
                {
                // the MaxLength() of path is KMaxPath, so we can append
            if (pathPtr.Length() && (pathPtr[pathPtr.Length() - 1] != '/'))
                {
                pathPtr.Append('/');
                }
                }
            iRemoteAccess->SetLockToken(pathPtr, *lockToken);
            CleanupStack::PopAndDestroy(path);
            }
        
        RPointerArray<CRsfwFileEntry>* kids = aRoot->Kids();
        TInt i;
        for (i = 0; i < kids->Count(); i++)
            {
            PopulateExternalLockTokenCacheL((*kids)[i]);
            }
        }
    }

//  End of File
