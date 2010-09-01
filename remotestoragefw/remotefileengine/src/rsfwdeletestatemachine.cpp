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
* Description:  Delete a file or directory
*
*/


#include "rsfwdeletestatemachine.h"
#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwinterface.h"
#include "rsfwfileengine.h"
#include "mdebug.h"
#include "rsfwdirent.h"


// ----------------------------------------------------------------------------
// CRsfwDeleteStateMachine::CRsfwDeleteStateMachine
// ----------------------------------------------------------------------------
//
CRsfwDeleteStateMachine::CRsfwDeleteStateMachine(TUint aNodeType)
    : iNodeType(aNodeType)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwDeleteStateMachine::~CRsfwDeleteStateMachine
// ----------------------------------------------------------------------------
//
CRsfwDeleteStateMachine::~CRsfwDeleteStateMachine()
    {
    iDirEnts.ResetAndDestroy();
    }

// ----------------------------------------------------------------------------
// CRsfwDeleteStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState* CRsfwDeleteStateMachine::CompleteRequestL(TInt aError)
    {
    if (iKidPath)
        {
        delete iKidPath;
        iKidPath = NULL;
        }

    // If the return code was KErrInUse, the directory we were about to delete
    // was not empty, and we couldn't delete it. In this case it has anyway
    //been added to the cache (as a side effect),
    // so we don't want to delete the fep.
    if (aError != KErrInUse && iKidCreated)
        {
        delete iKidFep;
        iKidFep = NULL;
        }

    CompleteAndDestroyState()->SetErrorCode(aError);
    return CompleteAndDestroyState();
    }

// Check if exists


// ----------------------------------------------------------------------------
// CRsfwDeleteStateMachine::TCheckIfCanBeDeleted::TCheckIfCanBeDeleted
// ----------------------------------------------------------------------------
//
CRsfwDeleteStateMachine::
TCheckIfCanBeDeleted::TCheckIfCanBeDeleted(CRsfwDeleteStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwDeleteStateMachine::TCheckIfCanBeDeleted::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwDeleteStateMachine::TCheckIfCanBeDeleted::EnterL()
    {
    // Kidname is in the same place in remove and rmdir structures,,,
    TRfeRemoveInArgs* inArgs =
        static_cast<TRfeRemoveInArgs*>(iOperation->iInArgs);
    TPtrC kidName(inArgs->iName);

    // the parent from which we are removing
    if (!iOperation->Node())
        {
        User::Leave(KErrNotFound);
        }

    DEBUGSTRING16(("removing entry '%S' from fid %d",
                   &kidName,
                   iOperation->Node()->Fid().iNodeId));

    // Do we know about the kid yet?
    iOperation->iKidFep = iOperation->Node()->FindKidByName(kidName);
    if (!iOperation->iKidFep)
        {
        // Create a temporary file entry for the target
        iOperation->iKidFep = CRsfwFileEntry::NewL(kidName, iOperation->Node());
        iOperation->iKidCreated = ETrue;
        }

    // Ensure the type matches with the operation (RmDir() or Delete())
    if ((iOperation->iKidFep->Type() != KNodeTypeUnknown) &&
        (iOperation->iKidFep->Type() != iOperation->iNodeType))
        {
        DEBUGSTRING(("object type does not match the parameter type!"));
        User::Leave(KErrArgument);
        }

    // if the type is unknown, set it from the operation parameter
    if (iOperation->iKidFep->Type() == KNodeTypeUnknown)
        {
        iOperation->iKidFep->SetType(iOperation->iNodeType);
        }

    // If it is a directory, check that it is empty
    if (!iOperation->FileEngine()->Disconnected() &&
        iOperation->iNodeType == KNodeTypeDir)
        {
        TInt zero = 0;
        iOperation->FileEngine()->
            FetchAndCacheL(*iOperation->iKidFep,
                           0,
                           &zero,
                           &iOperation->iDirEnts,
                           iOperation);
        }
    else
        {
        iOperation->HandleRemoteAccessResponse(0, KErrNone);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDeleteStateMachine::TCheckIfCanBeDeleted::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwDeleteStateMachine::TState*
CRsfwDeleteStateMachine::TCheckIfCanBeDeleted::CompleteL()
    {
    if ((iOperation->iNodeType == KNodeTypeDir) &&
        ((iOperation->iDirEnts).Count() > 0))
        {
        // Attach the directory itself if it was unknown
        if (iOperation->iKidCreated)
            {
            iOperation->FileEngine()->iFileTable->AddL(iOperation->iKidFep);
            iOperation->Node()->AddKid(*iOperation->iKidFep);
            }
            
        // if the directory is not empty we cannot delete it
        // however, let's add its entries to the cache
        iOperation->FileEngine()->AddToCacheL(
            *iOperation->iKidFep,
            &iOperation->iDirEnts,
            iOperation->FileEngine(),
            0);

        return iOperation->CompleteRequestL(KErrInUse);
        }
    else
        {
        return new CRsfwDeleteStateMachine::TDeleteNodeState(iOperation);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDeleteStateMachine::TCheckIfCanBeDeleted::ErrorL
// ----------------------------------------------------------------------------
//
CRsfwDeleteStateMachine::TState*
CRsfwDeleteStateMachine::TCheckIfCanBeDeleted::ErrorL(TInt aCode)
    {
    DEBUGSTRING(("kid didn't exist!"));
    return iOperation->CompleteRequestL(aCode);
    }

// Remove directory

// ----------------------------------------------------------------------------
// CRsfwDeleteStateMachine::TDeleteNodeState::TDeleteNodeState
// ----------------------------------------------------------------------------
//
CRsfwDeleteStateMachine::
TDeleteNodeState::TDeleteNodeState(CRsfwDeleteStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwDeleteStateMachine::TDeleteNodeState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwDeleteStateMachine::TDeleteNodeState::EnterL()
    {
    // Get the path for the actual remove
    iOperation->iKidPath =
        iOperation->FileEngine()->FullNameL(*iOperation->iKidFep);
    if (!iOperation->FileEngine()->Disconnected())
        {
        if (iOperation->iNodeType == KNodeTypeDir)
            {
            iOperation->
                FileEngine()->
                RemoteAccessL()->DeleteDirectoryL(*iOperation->iKidPath,
                                                  iOperation);
            }
        else // KNodeTypFile
            {
            iOperation->
                FileEngine()->
                RemoteAccessL()->DeleteFileL(*iOperation->iKidPath,
                                             iOperation);
            }
        }
    else
        {
        // Disconnected
        iOperation->HandleRemoteAccessResponse(0, KErrNone);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDeleteStateMachine::TDeleteNodeState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwDeleteStateMachine::TState* CRsfwDeleteStateMachine::TDeleteNodeState::CompleteL()
    {
    if (!iOperation->FileEngine()->Disconnected())
        {
        // If the target was already known, remove and destroy it
        if (!iOperation->iKidCreated)
            {
            iOperation->FileEngine()->iFileTable->RemoveL(iOperation->iKidFep);
            delete iOperation->iKidFep;
            iOperation->iKidFep = NULL;
            }
        }

    iOperation->Node()->SetLocallyDirty();
    return iOperation->CompleteRequestL(KErrNone);
    }

// ----------------------------------------------------------------------------
// CRsfwDeleteStateMachine::TDeleteNodeState::ErrorL
// ----------------------------------------------------------------------------
//
CRsfwDeleteStateMachine::TState*
CRsfwDeleteStateMachine::TDeleteNodeState::ErrorL(TInt aCode)
    {
    return iOperation->CompleteRequestL(aCode);
    }



