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
* Description:  State machine for file lookup
*
*/


#include "rsfwlookupstatemachine.h"
#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwinterface.h"
#include "rsfwfileengine.h"
#include "mdebug.h"
#include "rsfwdirentattr.h"
#include "rsfwvolumetable.h"

// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::CRsfwLookupStateMachine
// ----------------------------------------------------------------------------
//
CRsfwLookupStateMachine::CRsfwLookupStateMachine()
    {
    iKidCreated = ETrue;
    }

// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::~CRsfwLookupStateMachine
// ----------------------------------------------------------------------------
//
CRsfwLookupStateMachine::~CRsfwLookupStateMachine()
    {
    delete iDirEntAttr;
    if (iPath)
    	{
    	delete iPath;
    	}
    }

// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState* CRsfwLookupStateMachine::CompleteRequestL(TInt aError)
    {
    TRfeLookupOutArgs* outArgs = static_cast<TRfeLookupOutArgs*>(iOutArgs);
    if (aError == KUpdateNotRequired)
        {   // discard
        aError = KErrNone;
        }

    if (!aError)
        {
        outArgs->iFid = iKidFep->Fid();
        }
    CompleteAndDestroyState()->SetErrorCode(aError);
    return CompleteAndDestroyState();
    }

// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwLookupStateMachine::TState* CRsfwLookupStateMachine::CompleteL()
    {
    // from CRsfwFileEngine::UpdateFileAttributes/UpdateDirAttributes
#ifdef _DEBUG
    if (iDirEntAttr)
        {
        DEBUGSTRING(("Kid attributes: attr=0x%x, size=%d, time=",
                     iDirEntAttr->Att(),
                     iDirEntAttr->Size()));
        DEBUGTIME((iDirEntAttr->Modified()));
        }
#endif

    if (iKidCreated)
        {
        /* from CRsfwFileEngine::DoLookupL */
        // Create a file entry for this kid
        if (!Volumes()->EnsureMetadataCanBeAddedL(Node()))
            {
            User::Leave(KErrNoMemory);
            }
        iKidFep = CRsfwFileEntry::NewL(iKidName, Node());
        if (iDirEntAttr->Att() & KEntryAttDir)
            {
            iKidFep->SetType(KNodeTypeDir);
            }
        else
            {
            iKidFep->SetType(KNodeTypeFile);
            }

        iKidFep->SetAttributesL(*iDirEntAttr, ETrue);

        // Remember that we have this kid
        FileEngine()->iFileTable->AddL(iKidFep);
        Node()->AddKid(*iKidFep);
        // If we really find a new kid that we were not aware of
        // we must set the parent as locally dirty
        // (this should not happen too often)
        Node()->SetLocallyDirty();
        }

    // We now have a valid kid entry
    return CompleteRequestL(KErrNone);
    }

// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::TUpdateKidAttributesTryFirstTypeState::TUpdateKidAttributesTryFirstTypeState
// ----------------------------------------------------------------------------
//
CRsfwLookupStateMachine::
TUpdateKidAttributesTryFirstTypeState::
TUpdateKidAttributesTryFirstTypeState(CRsfwLookupStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::TUpdateKidAttributesTryFirstTypeState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwLookupStateMachine::TUpdateKidAttributesTryFirstTypeState::EnterL()
    {
    DEBUGSTRING(("CRsfwLookupStateMachine::TUpdateKidAttributesTryFirstTypeState::EnterL"));
    TRfeLookupInArgs* inArgs =
        static_cast<TRfeLookupInArgs*>(iOperation->iInArgs);
    TRfeLookupOutArgs* outArgs =
        static_cast<TRfeLookupOutArgs*>(iOperation->iOutArgs);
    iOperation->iNodeType = inArgs->iNodeType;
    iOperation->iKidName.Set(inArgs->iName);

    TInt err = KErrNone;
    iOperation->iKidFep = NULL;

    if (!iOperation->Node())
        {
        User::Leave(KErrNotFound);
        }

    DEBUGSTRING16(("looking up '%S' in fid=%d",
                   &(iOperation->iKidName),
                   iOperation->Node()->Fid().iNodeId));

    // We'd better be looking up in a directory
    if (iOperation->Node()->Type() != KNodeTypeDir)
        {
        User::Leave(KErrNotFound);
        }

    // Try to get the entry from the parent directory
    iOperation->iKidFep =
        iOperation->Node()->FindKidByName(iOperation->iKidName);
    if (!iOperation->iKidFep)
        {
        DEBUGSTRING(("no such kid!"));
        // Didn't find it
        // if the parent directory's cache entry is still valid
        // we return "not found"
        if ((iOperation->FileEngine()->UseCachedAttributes(*iOperation->Node())) &&
         	(iOperation->FileEngine()->UseCachedData(*iOperation->Node())))
        	{
			User::Leave(KErrNotFound);
        	}

        iOperation->iPath =
            iOperation->FileEngine()->FullNameL(*iOperation->Node());
        if (iOperation->iNodeType == KNodeTypeUnknown)
            {
            iOperation->FileEngine()->
                UpdateAttributesL(*iOperation->iPath,
                                  iOperation->iKidName,
                                  iOperation->iDirEntAttr,
                                  KNodeTypeFile,
                                  iOperation);
            }
        else
            {
            iOperation->FileEngine()->
                UpdateAttributesL(*iOperation->iPath,
                                  iOperation->iKidName,
                                  iOperation->iDirEntAttr,
                                  iOperation->iNodeType,
                                  iOperation);
            }

        if (err)
            {
            // communication to the server failed,
            //e.g. we are in disconnected mode
            User::Leave(err);
            }
        }
    else
        {
        // We now have a valid kid entry
        outArgs->iFid = iOperation->iKidFep->Fid();
        iOperation->HandleRemoteAccessResponse(0, KUpdateNotRequired);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::TUpdateKidAttributesTryFirstTypeState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwLookupStateMachine::TState*
CRsfwLookupStateMachine::TUpdateKidAttributesTryFirstTypeState::CompleteL()
    {
    DEBUGSTRING(("CRsfwLookupStateMachine::TUpdateKidAttributesTryFirstTypeState::CompleteL"));
    return iOperation->CompleteL();
    }

// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::TUpdateKidAttributesTryFirstTypeState::ErrorL
// If we were looking for a file return KErrNotFound
// if for directory KErrPathNotFound.
// File Server seems to always call Entry() (-->lookup()) before other file
// operations so as long as we return the right variant of "not found"
// error here other places do not matter.
// ----------------------------------------------------------------------------
//
CRsfwLookupStateMachine::TState*
CRsfwLookupStateMachine::TUpdateKidAttributesTryFirstTypeState::ErrorL(TInt aCode)
    {
    DEBUGSTRING(("CRsfwLookupStateMachine::TUpdateKidAttributesTryFirstTypeState::ErrorL %d", aCode));
    if (aCode == KUpdateNotRequired)
        {
        iOperation->iKidCreated = EFalse;
        return iOperation->CompleteL();
        }
    else if (iOperation->iNodeType == KNodeTypeUnknown)
        {
        return new CRsfwLookupStateMachine::TUpdateKidAttributesTrySecondTypeState(
            iOperation);
        }
    else if ((aCode != KErrNotFound) && (aCode != KErrPathNotFound))
   	 	{
    	return iOperation->CompleteRequestL(aCode);
    	}
    else if (iOperation->iNodeType == KNodeTypeDir)
        {
        return iOperation->CompleteRequestL(KErrPathNotFound);
        }
    else
        {
        return iOperation->CompleteRequestL(KErrNotFound);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::TUpdateKidAttributesTrySecondTypeState::TUpdateKidAttributesTrySecondTypeState
// ----------------------------------------------------------------------------
//
CRsfwLookupStateMachine::
TUpdateKidAttributesTrySecondTypeState::
TUpdateKidAttributesTrySecondTypeState(CRsfwLookupStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::TUpdateKidAttributesTrySecondTypeState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwLookupStateMachine::TUpdateKidAttributesTrySecondTypeState::EnterL()
    {
    DEBUGSTRING(("CRsfwLookupStateMachine::TUpdateKidAttributesTrySecondTypeState::EnterL"));
    // We only come here if nodetype is unknown and
    // we already tried to lookup as a file
    iOperation->FileEngine()->UpdateAttributesL(*iOperation->iPath,
                                                iOperation->iKidName,
                                                iOperation->iDirEntAttr,
                                                KNodeTypeDir, iOperation);
    }

// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::TUpdateKidAttributesTrySecondTypeState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwLookupStateMachine::TState*
CRsfwLookupStateMachine::TUpdateKidAttributesTrySecondTypeState::CompleteL()
    {
    DEBUGSTRING(("CRsfwLookupStateMachine::TUpdateKidAttributesTrySecondTypeState::CompleteL"));   
    // from CRsfwFileEngine::UpdateFileAttributes/UpdateDirAttributes
    return iOperation->CompleteL();
    }


// ----------------------------------------------------------------------------
// CRsfwLookupStateMachine::TUpdateKidAttributesTrySecondTypeState::ErrorL
// If we were looking for a file return KErrNotFound,
// if for directory KErrPathNotFound.
// File Server seems to always call Entry() (-->lookup()) before other file
// operations so as long as we return the right variant of "not found"
// error here other places do not matter.
// ----------------------------------------------------------------------------
//
CRsfwLookupStateMachine::TState*
CRsfwLookupStateMachine::TUpdateKidAttributesTrySecondTypeState::ErrorL(TInt aCode)
    {
    DEBUGSTRING(("CRsfwLookupStateMachine::TUpdateKidAttributesTrySecondTypeState::ErrorL %d", aCode));    
    // from CRsfwFileEngine::Lookup()
    // No such kid

    // cache the last failed lookup results
   	iOperation->FileEngine()->SetFailedLookup(*iOperation->iPath,
   											iOperation->iKidName);

    if (iOperation->iNodeType == KNodeTypeDir)
        {
        return iOperation->CompleteRequestL(KErrPathNotFound);
        }
    else
        {
        return iOperation->CompleteRequestL(KErrNotFound);
        }
    }

