/*
* Copyright (c) 2005-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  State machine for opening a file or directory
*
*/


#include "rsfwopenbypathstatemachine.h"
#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwinterface.h"
#include "rsfwfileengine.h"
#include "rsfwlockmanager.h"
#include "mdebug.h"


// ----------------------------------------------------------------------------
// CRsfwOpenByPathStateMachine::CRsfwOpenByPathStateMachine
// ----------------------------------------------------------------------------
//
CRsfwOpenByPathStateMachine::CRsfwOpenByPathStateMachine()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwOpenByPathStateMachine::~CRsfwOpenByPathStateMachine
// ----------------------------------------------------------------------------
//
CRsfwOpenByPathStateMachine::~CRsfwOpenByPathStateMachine()
    {
    delete iLockToken;
    }

// ----------------------------------------------------------------------------
// CRsfwOpenByPathStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState*
CRsfwOpenByPathStateMachine::CompleteRequestL(TInt aError)
    {
    DEBUGSTRING(("CRsfwOpenByPathStateMachine::CompleteRequestL %d", aError));
    TRfeOpenByPathOutArgs* outArgs =
        static_cast<TRfeOpenByPathOutArgs*>(iOutArgs);
    if(!aError)
        {
        if ((Node()->Type() == KNodeTypeFile) &&
            iRealOpen) 
            {
            // file opened successfully and was not already opened in create
            Node()->iFileTable->UpdateOpenFileCount(1);
            }
  
        outArgs->iPath.Copy(*iCacheName);
        iAttrp->iAtt = Node()->Att();
        iAttrp->iSize = Node()->Size();
        iAttrp->iModified = Node()->Modified();
        }

    CompleteAndDestroyState()->SetErrorCode(aError);
    return CompleteAndDestroyState();
    }

// ----------------------------------------------------------------------------
// CRsfwOpenByPathStateMachine::TRequestOpenModeState::TRequestOpenModeState
// ----------------------------------------------------------------------------
//
CRsfwOpenByPathStateMachine::TRequestOpenModeState::TRequestOpenModeState(
    CRsfwOpenByPathStateMachine* aParent)
    : iOperation(aParent)
    {
    iRequestedLock = EFalse;
    }

// ----------------------------------------------------------------------------
// CRsfwOpenByPathStateMachine::TRequestOpenModeState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwOpenByPathStateMachine::TRequestOpenModeState::EnterL()
    {
    
    DEBUGSTRING(("CRsfwOpenByPathStateMachine::TRequestOpenModeState::EnterL"));
    
    TRfeOpenByPathInArgs* inArgs =
        static_cast<TRfeOpenByPathInArgs*>(iOperation->iInArgs);
    TRfeOpenByPathOutArgs* outArgs =
        static_cast<TRfeOpenByPathOutArgs*>(iOperation->iOutArgs);
    iOperation->iRealOpen = inArgs->iTrueOpen;
    // Use inArgs->iFlags to pass desired locking information
    iOperation->iFlags = inArgs->iFlags;
    iOperation->iAttrp = &(outArgs->iAttr);

    if (!iOperation->Node())
        {
        User::Leave(KErrNotFound);
        }

    if (iOperation->Node()->Size() > iOperation->Volumes()->iMaxCacheSize)
        {
        DEBUGSTRING(("OPENBYPATH failed: file too large: file size %d, max cache size %d",
                     iOperation->Node()->Size(),
                     iOperation->Volumes()->iMaxCacheSize));
        User::Leave(KErrTooBig);
        }

    DEBUGSTRING(("opening fid %d with flags 0x%x",
                 iOperation->Node()->Fid().iNodeId,
                 iOperation->iFlags));

    if (!iOperation->FileEngine()->WriteDisconnected())
        {
        if (!(iOperation->Node()->IsLocked()) &&
            (iOperation->Node()->Type() != KNodeTypeDir) &&
            (iOperation->iFlags & EFileWrite))
            {
            DEBUGSTRING(("requesting lock"));
            // For the time being
            // if flag is 0 we are not interested in getting a lock
            iOperation->FileEngine()->LockManager()->ObtainLockL(
                iOperation->Node(),
                iOperation->iFlags,
                iOperation->iLockToken,
                iOperation);
            iRequestedLock = ETrue;
            }
        else
            {
            // IsLocked() || !(iOperation->iFlags & EFileWrite)
            iOperation->HandleRemoteAccessResponse(0, KErrNone);
            }
        }
    else
        {
        // if WriteDisconnected()
        iOperation->HandleRemoteAccessResponse(0, KErrNone);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwOpenByPathStateMachine::TRequestOpenModeState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwOpenByPathStateMachine::TState*
CRsfwOpenByPathStateMachine::TRequestOpenModeState::CompleteL()
    {
     DEBUGSTRING(("CRsfwOpenByPathStateMachine::TRequestOpenModeState::CompleteL"));
    if (iRequestedLock)
        {
        //from CRsfwLockManager::ObtainLock()
#ifdef _DEBUG
        TPtrC p = iOperation->Node()->FullNameLC()->Des();
        DEBUGSTRING16(("ObtainLockL(): flags %d returned %d for file '%S'",
                       iOperation->iFlags,
                       KErrNone,
                       &p));
        CleanupStack::PopAndDestroy();
#endif // DEBUG
        iOperation->Node()->SetLockedL(
            iOperation->FileEngine()->LockManager(),
            iOperation->iLockToken);
        iOperation->iLockToken = NULL;
        }

    iOperation->iCacheName = iOperation->Node()->CacheFileName();

    // make sure that the relevant cache file is on place
    // (i.e. it has not been removed e.g. by other apps)
    iOperation->Node()->ValidateCacheFile();

    if (!iOperation->Node()->IsCached())
        {
        if (iOperation->FileEngine()->Disconnected())
            {
            if ((iOperation->Node()->Type() != KNodeTypeDir) &&
                !(iOperation->iFlags & EFileWrite))
                {
                // While disconnected,
                // write access of files returns "not found"
                return iOperation->CompleteRequestL(KErrNotFound);
                }
            }
        iOperation->FileEngine()->CreateContainerFileL(*(iOperation->Node()));
        iOperation->iCacheName = iOperation->Node()->CacheFileName();
        }
    else // is cached
        {
        if (iOperation->Node()->IsLocallyDirty())
            {
            // This is a directory which has at least one kid
            // that has been cached or flushed since the last opening
            // of the directory
            iOperation->FileEngine()->UpdateDirectoryContainerL(
                *(iOperation->Node()));
            }

        // If this is a cached file,
        // we must remove it from LRUlist as it is now open
        if (iOperation->iRealOpen)
            {
            iOperation->Node()->iFileTable->Volume()->iVolumeTable->
                RemoveFromLRUPriorityList(iOperation->Node());
            }
        }

    // if file or dir has just been opened, remove it from metadata LRU list
    iOperation->Node()->iFileTable->Volume()->iVolumeTable->
        RemoveFromMetadataLRUPriorityList(iOperation->Node());

    return iOperation->CompleteRequestL(KErrNone);
    }

// ----------------------------------------------------------------------------
// CRsfwOpenByPathStateMachine::TRequestOpenModeState::ErrorL
// fileEngine->RequestConnectionStateL() has returned with error
// ----------------------------------------------------------------------------
//
CRsfwOpenByPathStateMachine::TState*
CRsfwOpenByPathStateMachine::TRequestOpenModeState::ErrorL(TInt aCode)
    {
    DEBUGSTRING(("CRsfwOpenByPathStateMachine::TRequestOpenModeState::ErrorL %d", aCode));
    // from CRsfwFileEngine::DoOpenByPath()
    if (aCode == KErrNotSupported)
        {
        iRequestedLock = EFalse;
        // locks not supported is not an error state
        // instead we run the success state...
        return CompleteL();
        }

    //from CRsfwLockManager::ObtainLock()
#ifdef _DEBUG
    TPtrC p = iOperation->Node()->FullNameLC()->Des();
    DEBUGSTRING16(("ObtainLockL(): flags %d returned %d for file '%S'",
                   iOperation->iFlags,
                   aCode,
                   &p));
    CleanupStack::PopAndDestroy();
#endif

    return iOperation->CompleteRequestL(aCode);
    }

