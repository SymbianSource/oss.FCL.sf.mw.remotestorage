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
* Description:  State machine for creating files
*
*/


#include "rsfwcreatefilestatemachine.h"
#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwinterface.h"
#include "rsfwfileengine.h"
#include "rsfwlockmanager.h"
#include "mdebug.h"
#include "rsfwdirentattr.h"
#include "rsfwvolumetable.h"


// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::CRsfwCreateFileStateMachine
// ----------------------------------------------------------------------------
//
CRsfwCreateFileStateMachine::CRsfwCreateFileStateMachine()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::~CRsfwCreateFileStateMachine
// ----------------------------------------------------------------------------
//
CRsfwCreateFileStateMachine::~CRsfwCreateFileStateMachine()
    {
    delete iDirEntAttr;
    delete iLockToken;
    }

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState*
CRsfwCreateFileStateMachine::CompleteRequestL(TInt aError)
    {
    TRfeCreateOutArgs* outArgs =
        static_cast<TRfeCreateOutArgs*>(iOutArgs);
    if (!aError)
        { 
        // Set the return values for the create call
        TFid* kidFidp = &(outArgs->iFid);
        TDirEntAttr* oAttrp = &(outArgs->iAttr);
        *kidFidp = iKidFep->Fid();
        iKidFep->GetAttributes(*oAttrp);
        }

    if (iKidCreated && aError)
        {
        delete iKidFep;
        iKidFep = NULL;
        }
        
    // it may happen by chance that the new name is equal to iLastFailedLookup value
    FileEngine()->ResetFailedLookup();
        
    CompleteAndDestroyState()->SetErrorCode(aError);
    return CompleteAndDestroyState();
    }

// Check if exists

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::TCheckIfExistsState::TCheckIfExistsState
// ----------------------------------------------------------------------------
//
CRsfwCreateFileStateMachine::
TCheckIfExistsState::TCheckIfExistsState(CRsfwCreateFileStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::TCheckIfExistsState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwCreateFileStateMachine::TCheckIfExistsState::EnterL()
    {
    DEBUGSTRING(("CRsfwCreateFileStateMachine::TCheckIfExistsState::EnterL()"));
    TRfeCreateInArgs* inArgs =
        static_cast<TRfeCreateInArgs*>(iOperation->iInArgs);
    TPtrC kidName(inArgs->iEntry.iName);
    iExclp = inArgs->iExcl;

    // used to pass the file open mode
    // could be something simpler, e.g. only one int
    iOperation->iFlags = inArgs->iEntry.iAttr.iAtt;

    // Get the parent to which we are creating this
    if (!iOperation->Node())
        {
        User::Leave(KErrNotFound);
        }


    DEBUGSTRING16(("creating file '%S' in fid %d",
                   &kidName,
                   iOperation->Node()->Fid().iNodeId));

    // Do we know about the kid yet?
    iOperation->iKidFep = iOperation->Node()->FindKidByName(kidName);
    if (!iOperation->iKidFep)
        {
        // This is either a completely new file, or a file that we
        // have not yet created a file entry for.
                
        if (! iOperation->Volumes()->EnsureMetadataCanBeAddedL(iOperation->Node()))
            {
            User::Leave(KErrNoMemory);
            }
        iOperation->iKidFep = CRsfwFileEntry::NewL(kidName, iOperation->Node());
        iOperation->iKidCreated = ETrue;
        iOperation->iKidFep->SetNewlyCreated();
        }

    if (iOperation->FileEngine()->Disconnected())
        {
        if (iOperation->iKidFep->Type() != KNodeTypeUnknown)
            {
            // "file exists"
            iOperation->HandleRemoteAccessResponse(0, KErrNone);
            }
        else
            {
            iOperation->HandleRemoteAccessResponse(0, KErrNotFound);
            }
        }
    else
        {
        iOperation->FileEngine()->GetAttributesL(*iOperation->iKidFep,
                                                 iOperation->iDirEntAttr,
                                                 KNodeTypeFile,
                                                 iOperation);
        }
    }


// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::TCheckIfExistsState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwCreateFileStateMachine::TState*
CRsfwCreateFileStateMachine::TCheckIfExistsState::CompleteL()
    {
    DEBUGSTRING(("CRsfwCreateFileStateMachine::TCheckIfExistsState::CompleteL()"));
    if (iExclp)
        {
        DEBUGSTRING(("kid exists!"));
        return iOperation->CompleteRequestL(KErrAlreadyExists);
        }
    else
        { // file with the same name exists, but exclusive is false
        return new CRsfwCreateFileStateMachine::TCreateNodeState(iOperation); 
        }
    }

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::TCheckIfExistsState::ErrorL
// ----------------------------------------------------------------------------
//
CRsfwCreateFileStateMachine::TState*
CRsfwCreateFileStateMachine::TCheckIfExistsState::ErrorL(TInt aCode)
    {
    DEBUGSTRING16(("CRsfwCreateFileStateMachine::TCheckIfExistsState::ErrorL error=%d", aCode));
   	return new CRsfwCreateFileStateMachine::TCreateNodeState(iOperation); 
    }

// create node

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::TCreateNodeState::TCreateNodeState
// ----------------------------------------------------------------------------
//
CRsfwCreateFileStateMachine::
TCreateNodeState::TCreateNodeState(CRsfwCreateFileStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::TCreateNodeState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwCreateFileStateMachine::TCreateNodeState::EnterL()
    {
    DEBUGSTRING(("CRsfwCreateFileStateMachine::TCreateNodeState::EnterL()"));
    if (iOperation->iKidCreated)
        {
        iOperation->iKidFep->SetType(KNodeTypeFile);
        }

    if (!iOperation->FileEngine()->WriteDisconnected())
        {
        // Create the file
        HBufC16* kidPath =
            iOperation->FileEngine()->FullNameLC(*iOperation->iKidFep);
        // iDirEntAttr exists, we know we are overwriting
        // pass this info to the access module (needed e.g. by UPnP)
        iOperation->
            FileEngine()->
            RemoteAccessL()->CreateFileL(*kidPath,
                                         (iOperation->iDirEntAttr != NULL),
                                         iOperation);
        CleanupStack::PopAndDestroy(kidPath);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::TCreateNodeState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwCreateFileStateMachine::TState*
CRsfwCreateFileStateMachine::TCreateNodeState::CompleteL()
    {
    DEBUGSTRING(("CRsfwCreateFileStateMachine::TCreateNodeState::CompleteL()"));
    return new CRsfwCreateFileStateMachine::TAcquireLockState(iOperation);
    }

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::TCreateNodeState::ErrorL
// ----------------------------------------------------------------------------
//
CRsfwCreateFileStateMachine::TState*
CRsfwCreateFileStateMachine::TCreateNodeState::ErrorL(TInt aCode)
    {
    DEBUGSTRING16(("CRsfwCreateFileStateMachine::TCreateNodeState::ErrorL error=%d", aCode));
    DEBUGSTRING(("remote create failed!"));
    return iOperation->CompleteRequestL(aCode);
    }

// Acquire lock

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::TAcquireLockState::TAcquireLockState
// ----------------------------------------------------------------------------
//
CRsfwCreateFileStateMachine::
TAcquireLockState::TAcquireLockState(CRsfwCreateFileStateMachine* aParent)
    {
    iOperation = aParent;
    iRequestedLock = EFalse;
    }

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::TAcquireLockState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwCreateFileStateMachine::TAcquireLockState::EnterL()
    {
    DEBUGSTRING(("CRsfwCreateFileStateMachine::TAcquireLockState::EnterL()"));
    if (!iOperation->FileEngine()->WriteDisconnected())
        {
        // There are two state machines currently,
        // which may take a lock for a file based on the mode,
        // OpenByPath and this.
        // Currently the mode check is different which is not a good
        // thing, there is a clear risk of an error where they
        // acquire a lock in different situations.
        if (!iOperation->iKidFep->IsLocked()
            && iOperation->iFlags != KEntryAttReadOnly)
            {
            iOperation->FileEngine()->LockManager()->
                ObtainLockL(iOperation->iKidFep,
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
// CRsfwCreateFileStateMachine::TAcquireLockState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwCreateFileStateMachine::TState*
CRsfwCreateFileStateMachine::TAcquireLockState::CompleteL()
    {
    DEBUGSTRING(("CRsfwCreateFileStateMachine::TAcquireLockState::CompleteL()"));
    if (iRequestedLock)
        {
        iOperation->iKidFep->
            SetLockedL(iOperation->FileEngine()->LockManager(),
                       iOperation->iLockToken);
        iOperation->iLockToken = NULL;
        }

    // Note that the kid has to be attached to the file table
    // to get a NodeId before the cache file is created.
    if (iOperation->iKidCreated)
        {
        // Attach a new kid to its parent
        iOperation->FileEngine()->iFileTable->AddL(iOperation->iKidFep);
        iOperation->Node()->AddKid(*iOperation->iKidFep);
        }

    // Create an empty container file locally
    iOperation->FileEngine()->CreateContainerFileL(*iOperation->iKidFep);

    iOperation->iKidFep->SetSize(0);
    iOperation->iKidFep->SetCachedSize(0);
    iOperation->iKidFep->SetCached(ETrue);
    iOperation->iKidFep->SetAttribValidationTime();

    iOperation->FileEngine()->SetupAttributes(*iOperation->iKidFep);

    iOperation->Node()->SetLocallyDirty();

    return iOperation->CompleteRequestL(KErrNone);
    }

// ----------------------------------------------------------------------------
// CRsfwCreateFileStateMachine::TAcquireLockState::ErrorL
// ----------------------------------------------------------------------------
//
CRsfwCreateFileStateMachine::TState*
CRsfwCreateFileStateMachine::TAcquireLockState::ErrorL(TInt aCode)
    {
    DEBUGSTRING16(("CRsfwCreateFileStateMachine::TAcquireLockState::ErrorL error=%d", aCode));
    if (aCode == KErrNotSupported)
        {
        iRequestedLock = EFalse;
        return this->CompleteL();
        }
    else
        {
        return iOperation->CompleteRequestL(aCode);
        }
    }



