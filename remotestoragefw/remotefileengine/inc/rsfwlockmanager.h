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


#ifndef C_RSFWLOCKMANAGER_H
#define C_RSFWLOCKMANAGER_H

#include <e32base.h>

#include "rsfwrfesession.h"
#include "rsfwremoteaccess.h"

class CRsfwFileEntry;
class CRsfwRfeStateMachine;

/** lock timeout in seconds */
const TInt KDefaultLockTimeout              = 900;  

/** KDefaultLockTimeout is the lock timeout requested from the server
 Our lock timer is set to value KDefaultLockTimeout / KLockRefreshAdjustment.
 It must be smaller, as it is started only when we receive the reply
 from the server, and when it expires we still must have time to sent the
 refresh request */
const TInt KLockRefreshAdjustment           = 3;

/** If lock refresh attempt results in an error from the protocol stack
 we use timeout mechanism to try again, but with a small timeout
 as we are not even sending packets to the server. */
const TInt KMinLockRefreshAttempt           = 5;

/**
 *  Pending lock renewal requests
 *
 *  @lib remotefe.exe
 *  @since Series 60 3.1
 */
class TPendingLockRefreshContext
    {
public: 
    // Lock refresh request transaction Id
    TUint                         iId;
    // Pointer to the file entry waiting for this refresh 
    CRsfwFileEntry*                   iFileEntry;
    };

class CRsfwLockManager: public CBase, public MRsfwRemoteAccessResponseHandler
    {
public:
    static CRsfwLockManager* NewL(CRsfwRemoteAccess* aRemoteAccess);
    static CRsfwLockManager* NewLC(CRsfwRemoteAccess* aRemoteAccess);
    ~CRsfwLockManager();

    void HandleRemoteAccessResponse(TUint aId, TInt aStatus);
    void ObtainLockL(CRsfwFileEntry*
                     aFileEntry,
                     TUint aLockFlags,
                     TDesC8*& aLockToken,
                     CRsfwRfeStateMachine* aOperation);
    void ReleaseLockL(CRsfwFileEntry* aFileEntry, CRsfwRfeStateMachine* aOperation);
    void RefreshLockL(CRsfwFileEntry* aFileEntry);
    TInt LockedCount();
    void AddLockedEntryL(CRsfwFileEntry* aEntry);
    void RemoveLockedEntry(CRsfwFileEntry* aEntry);
    void PopulateExternalLockTokenCacheL(CRsfwFileEntry* aRoot);
    
private:
    void ConstructL(CRsfwRemoteAccess*) ;

private:
    CRsfwRemoteAccess* iRemoteAccess;    // remote file transport module
    RArray<TPendingLockRefreshContext>  iLockRefreshContexts;
    RPointerArray<CRsfwFileEntry> iLockedEntries;
    };

#endif // LOCKMANAGER_H

// End of File
