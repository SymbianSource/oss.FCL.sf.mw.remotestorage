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
* Description:  State machine for renaming files
*
*/


#include "rsfwrenamefilestatemachine.h"
#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwfileengine.h"
#include "rsfwlockmanager.h"
#include "mdebug.h"


// ----------------------------------------------------------------------------
// CRsfwRenameFileStateMachine::CRsfwRenameFileStateMachine
// ----------------------------------------------------------------------------
// 
CRsfwRenameFileStateMachine::CRsfwRenameFileStateMachine()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwRenameFileStateMachine::~CRsfwRenameFileStateMachine
// ----------------------------------------------------------------------------
// 
CRsfwRenameFileStateMachine::~CRsfwRenameFileStateMachine() 
    {
    delete iLockToken;
    }

// ----------------------------------------------------------------------------
// CRsfwRenameFileStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
// 
CRsfwRfeStateMachine::TState*
CRsfwRenameFileStateMachine::CompleteRequestL(TInt aError) 
    {
    if (iSrcKidCreated)
        {
        if (aError)
            {
            delete iSrcKidFep;
            iSrcKidFep = NULL;
            }
        }
    if (iDstKidCreated)
        {
        delete iDstKidFep;
        iDstKidFep = NULL;
        }

    // it may happen that the new name is equal to iLastFailedLookup value
    FileEngine()->ResetFailedLookup();

    CompleteAndDestroyState()->SetErrorCode(aError);
    return CompleteAndDestroyState();
    }



// Rename the file

// ----------------------------------------------------------------------------
// CRsfwRenameFileStateMachine::TRenameFileState::TRenameFileState
// ----------------------------------------------------------------------------
// 
CRsfwRenameFileStateMachine::
TRenameFileState::TRenameFileState(CRsfwRenameFileStateMachine* aParent)
    : iOperation(aParent)
    {   
    }

// ----------------------------------------------------------------------------
// CRsfwRenameFileStateMachine::TRenameFileState::EnterL
// ----------------------------------------------------------------------------
// 
void CRsfwRenameFileStateMachine::TRenameFileState::EnterL() 
    {
    // rename or replace
    TRfeRenameInArgs* inArgs =
        static_cast<TRfeRenameInArgs*>(iOperation->iInArgs);
    iOperation->iOverWrite = inArgs->iOverWrite;
        
    TFid* dstParentFidp = &(inArgs->iDstFid);
    TPtrC16 srcKidName(inArgs->iSrcName);
    iOperation->iDstKidName.Set(inArgs->iDstName);
    
    // Get the parent from which we are removing
    if (!iOperation->Node())
        {
        User::Leave(KErrNotFound);
        }

    // Get the parent to which we are removing
    iOperation->iDstParentFep =
        iOperation->FileEngine()->iFileTable->Lookup(*dstParentFidp);
    if (!iOperation->iDstParentFep)
        {
        User::Leave(KErrNotFound);
        }

    // Do we know the target kid yet?
    iOperation->iDstKidFep =
        iOperation->iDstParentFep->FindKidByName(iOperation->iDstKidName);
    if (!iOperation->iDstKidFep)
        {
        // Create a temporary file entry for building the full path
        iOperation->iDstKidFep =
            CRsfwFileEntry::NewL(iOperation->iDstKidName,
                             iOperation->iDstParentFep);
        iOperation->iDstKidCreated = ETrue;
        }

    // Do we know the source kid yet?
    iOperation->iSrcKidFep = iOperation->Node()->FindKidByName(srcKidName);
    if (!iOperation->iSrcKidFep)
        {
        iOperation->iSrcKidFep = CRsfwFileEntry::NewL(srcKidName,
                                                  iOperation->Node());
        iOperation->iSrcKidCreated = ETrue;
        }
   
    if (!iOperation->FileEngine()->Disconnected())
        {
        // Do the rename
        HBufC* srcKidPath =
            iOperation->FileEngine()->FullNameLC(*iOperation->iSrcKidFep);
        if ((*iOperation->iSrcKidFep).Type() == KNodeTypeDir)
            { // if source is a directory, make sure the name ends with a slash
            if ((*srcKidPath)[(srcKidPath->Length() - 1)] != '/')
                {
                TPtr srcKidAppend = srcKidPath->Des();         
                srcKidAppend.Append('/');   
                }   
            }
        
        HBufC* dstKidPath =
            iOperation->FileEngine()->FullNameLC(*iOperation->iDstKidFep);
        if ((*iOperation->iDstKidFep).Type() == KNodeTypeDir)
            { // if source is a directory, make sure the name ends with a slash
            if ((*dstKidPath)[(dstKidPath->Length() -1)] != '/')
                {
                TPtr dstKidAppend = dstKidPath->Des();         
                dstKidAppend.Append('/');   
                }   
            }
        iOperation->
            FileEngine()->
            RemoteAccessL()->RenameL(*srcKidPath,
                                     *dstKidPath,
                                     iOperation->iOverWrite,
                                     iOperation);         
        CleanupStack::PopAndDestroy(2, srcKidPath); // dstKidPath, srcKidPath
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRenameFileStateMachine::TRenameFileState::CompleteL
// ----------------------------------------------------------------------------
// 
CRsfwRenameFileStateMachine::TState*
CRsfwRenameFileStateMachine::TRenameFileState::CompleteL()
    {
    if (!iOperation->iSrcKidCreated)
        {
        // If we didn't create the srcKidFep,
        // we must remove it from parent's list
        iOperation->Node()->RemoveKidL(iOperation->iSrcKidFep);
        }
    else
        {
        // Enter source into the file table
        iOperation->Node()->iFileTable->AddL(iOperation->iSrcKidFep);
        }

    if (!iOperation->iDstKidCreated)
        {
        // If we didn't create the dstKidFep, we must remove and destroy it
        // unless the node has a removed marking set
        // (should not happen, except for removed nodes)
        iOperation->FileEngine()->iFileTable->RemoveL(iOperation->iDstKidFep);
        delete iOperation->iDstKidFep;
        iOperation->iDstKidFep = NULL;
        }

    // Change srcKidFep's name,
    // and insert the fep into the dstParentFep directory
    iOperation->iSrcKidFep->RenameL(iOperation->iDstKidName);
    iOperation->iDstParentFep->AddKid(*iOperation->iSrcKidFep);

    // Refresh both parent directories
    iOperation->Node()->SetLocallyDirty();
    if (iOperation->Node() != iOperation->iDstParentFep)
        {
        iOperation->Node()->SetLocallyDirty();
        }

    return new CRsfwRenameFileStateMachine::TAcquireLockState(iOperation);
    }

// ----------------------------------------------------------------------------
// CRsfwRenameFileStateMachine::TRenameFileState::ErrorL
// ----------------------------------------------------------------------------
// 
CRsfwRenameFileStateMachine::TState*
CRsfwRenameFileStateMachine::TRenameFileState::ErrorL(TInt aCode)
    { 
    DEBUGSTRING(("remote rename failed"));
    return iOperation->CompleteRequestL(aCode);         
    }
   
// ----------------------------------------------------------------------------
// CRsfwRenameFileStateMachine::TAcquireLockState::TAcquireLockState
// ----------------------------------------------------------------------------
//  
CRsfwRenameFileStateMachine::
TAcquireLockState::TAcquireLockState(CRsfwRenameFileStateMachine* aParent)
    {
    iOperation = aParent;  
    iRequestedLock = EFalse;  
    }

// ----------------------------------------------------------------------------
// CRsfwRenameFileStateMachine::TAcquireLockState::EnterL
// ----------------------------------------------------------------------------
// 
void CRsfwRenameFileStateMachine::TAcquireLockState::EnterL() 
    {
    if (!iOperation->FileEngine()->WriteDisconnected())
        {
        // Now we have updated everything necessary in iSrcKidFep
        // possibly lock timer is ticking with etc.
        // However, at least WebDAV does not lock the new file in move,
        // so if the old file was locked we need to tell
        // the access protocol plug-in that it must lock the new file.
        if (iOperation->iSrcKidFep->IsLocked())
            {
            iOperation->iSrcKidFep->iLockTimer->Cancel();
            iOperation->FileEngine()->LockManager()->
                ObtainLockL(iOperation->iSrcKidFep,
                            EFileWrite,
                            iOperation->iLockToken,
                            iOperation);
            iRequestedLock = ETrue;
            }
        else 
            {
            iOperation->HandleRemoteAccessResponse(0, KErrNone);
            }
        }
    else 
        {
        iOperation->HandleRemoteAccessResponse(0, KErrNone);       
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRenameFileStateMachine::TAcquireLockState::CompleteL
// ----------------------------------------------------------------------------
// 
CRsfwRenameFileStateMachine::TState*
CRsfwRenameFileStateMachine::TAcquireLockState::CompleteL()
    {
    if (iRequestedLock) 
        {
        iOperation->
            iSrcKidFep->SetLockedL(iOperation->FileEngine()->LockManager(),
                                   iOperation->iLockToken);
        iOperation->iLockToken = NULL;
        }
    return iOperation->CompleteRequestL(KErrNone); 
    }

// ----------------------------------------------------------------------------
// CRsfwRenameFileStateMachine::TAcquireLockState::ErrorL
// ----------------------------------------------------------------------------
// 
CRsfwRenameFileStateMachine::TState*
CRsfwRenameFileStateMachine::TAcquireLockState::ErrorL(TInt aCode)
    {       
    return iOperation->CompleteRequestL(aCode);   
    }
    
