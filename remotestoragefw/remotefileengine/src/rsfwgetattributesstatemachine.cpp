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
* Description:  State machine for getting attributes of a file
*
*/


#include "rsfwgetattributesstatemachine.h"
#include "rsfwopenbypathstatemachine.h"
#include "rsfwinterface.h"
#include "rsfwrferequest.h"
#include "rsfwrfeoperation.h"
#include "rsfwfileentry.h"
#include "rsfwfileengine.h"
#include "mdebug.h"
#include "rsfwdirentattr.h"
#include "rsfwvolumetable.h"

// ----------------------------------------------------------------------------
// CRsfwGetAttributesStateMachine::CRsfwGetAttributesStateMachine
// ----------------------------------------------------------------------------
//
CRsfwGetAttributesStateMachine::CRsfwGetAttributesStateMachine()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwGetAttributesStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState* CRsfwGetAttributesStateMachine::CompleteRequestL(TInt aError)
    {
    TRfeGetAttrOutArgs* outArgs = static_cast<TRfeGetAttrOutArgs*>(iOutArgs);

    if (!aError)
        {
        outArgs->iAttr.iAtt = Node()->Att();
        outArgs->iAttr.iSize = Node()->Size();
        outArgs->iAttr.iModified = Node()->Modified();
        }

    CompleteAndDestroyState()->SetErrorCode(aError);
    return CompleteAndDestroyState();
    }

// ----------------------------------------------------------------------------
// CRsfwGetAttributesStateMachine::TRefreshAttributesState::TRefreshAttributesState
// ----------------------------------------------------------------------------
//
CRsfwGetAttributesStateMachine::
TRefreshAttributesState::
TRefreshAttributesState(CRsfwAttributeRefreshingStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwGetAttributesStateMachine::TRefreshAttributesState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwGetAttributesStateMachine::TRefreshAttributesState::EnterL()
    {
    DEBUGSTRING(("CRsfwGetAttributesStateMachine::TRefreshAttributesState::EnterL"));
    TInt err = KErrNone;

    if (iOperation->Node())
        {
        DEBUGSTRING(("getting attributes of fid %d",
                     iOperation->Node()->Fid().iNodeId));

        // as the entry is "needed" move it to the back of metadata LRU list
        iOperation->Volumes()->MoveToTheBackOfMetadataLRUPriorityListL(iOperation->Node());

        if (!(iOperation->FileEngine()->UseCachedAttributes(*iOperation->Node())))
            {
            // If we find the file entry and
            // the time window to use cached attributes has passed.
            // Store the old attributes
            delete iOperation->iDirEntAttrOld;
            iOperation->iDirEntAttrOld = NULL;
            iOperation->iDirEntAttrOld = CRsfwDirEntAttr::NewL();
            iOperation->Node()->GetAttributesL(*iOperation->iDirEntAttrOld);
            if (!iOperation->FileEngine()->WriteDisconnected())
                {
                iOperation->FileEngine()->GetAttributesL(
                    *(iOperation->Node()),
                    iOperation->iDirEntAttr,
                    iOperation->Node()->Type(),
                    iOperation);
                }
            else
                {
                iOperation->HandleRemoteAccessResponse(0, KUpdateNotRequired);
                }
            if (err)
                {
                User::Leave(err);
                }
            }
        else
            {
            // use cached attributes
            iOperation->HandleRemoteAccessResponse(0, KUpdateNotRequired);
            }
        }
    else
        {
        User::Leave(KErrPathNotFound);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwGetAttributesStateMachine::TRefreshAttributesState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwGetAttributesStateMachine::TState*
CRsfwGetAttributesStateMachine::TRefreshAttributesState::CompleteL()
    {
    
    DEBUGSTRING(("TRefreshAttributesState::CompleteL for fid %d",
                     iOperation->Node()->Fid().iNodeId));
                
    // from CRsfwFileEngine::UpdateAttributes()
    iOperation->Node()->SetAttributesL(*iOperation->iDirEntAttr, ETrue);
    
    if (iOperation->Node()->IsOpenedForWriting())
        {
        iOperation->Node()->iUseCachedData = ETrue;
        }
    else if (iOperation->FileEngine()->DataChanged(*iOperation->iDirEntAttrOld,
                                                   *iOperation->iDirEntAttr))
        {
        // discard the old cache file
        // this will also call SetCached(EFalse) etc...
        iOperation->Node()->RemoveCacheFile();
        }

    DEBUGSTRING(("Attributes: attr=0x%x, size=%d, time=",
                 iOperation->Node()->Att(),
                 iOperation->Node()->Size()));
    DEBUGTIME((iOperation->Node()->Modified()));

    // from CRsfwFileEngine::UpdateFileAttributes/UpdateDirAttributes
    return CompleteOurRequestL(KErrNone);
    }

// ----------------------------------------------------------------------------
// CRsfwGetAttributesStateMachine::TRefreshAttributesState::ErrorL
// ----------------------------------------------------------------------------
//
CRsfwGetAttributesStateMachine::TState*
CRsfwGetAttributesStateMachine::TRefreshAttributesState::ErrorL(TInt aCode)
    {
    DEBUGSTRING(("CRsfwGetAttributesStateMachine::TRefreshAttributesState::ErrorL %d", aCode));
    if (aCode == KUpdateNotRequired)
        {
        // note that we should NOT set iUseCachedData to ETrue here
        // (if it is false, only after fetch it should be set to true
        // , or openbypath if we are writing to the file)
        aCode = KErrNone;
            
       DEBUGSTRING(("update was not required"));
        
        DEBUGSTRING(("Attributes: attr=0x%x, size=%d, time=",
                 iOperation->Node()->Att(),
                 iOperation->Node()->Size()));
     DEBUGTIME((iOperation->Node()->Modified()));

        }
    else
        {
        // from CRsfwFileEngine::UpdateAttributes()
        if (!(iOperation->Node()->IsOpenedForWriting()))
            {
            // "iOperation->Node()" has been removed from the server??
            // : remove FEP here
            iOperation->Node()->RemoveCacheFile();
            }
        }
    return CompleteOurRequestL(aCode);
    }

// ----------------------------------------------------------------------------
// CRsfwGetAttributesStateMachine::TRefreshAttributesState::CompleteOurRequestL
// ----------------------------------------------------------------------------
//
CRsfwGetAttributesStateMachine::TState*
CRsfwGetAttributesStateMachine::TRefreshAttributesState::CompleteOurRequestL(TInt aCode) 
    {
    if (iOperation->Request()->Operation()->Function() == EGetAttr)
        {
        // we are running in GetAttr()
        return iOperation->CompleteRequestL(aCode);
        }
    else if (iOperation->Request()->Operation()->Function() == EOpenByPath)
        {
        // we are running in OpenByPath()
        if (aCode == KErrNone) 
            {
            return new (ELeave) CRsfwOpenByPathStateMachine::TRequestOpenModeState(
                (CRsfwOpenByPathStateMachine *)iOperation);
            }
        else 
            {
            // attributes expired, refrshing them failed, do not open the file
            return iOperation->CompleteRequestL(aCode);
            }
        }
    else
        {
        return NULL;
        }
    }


