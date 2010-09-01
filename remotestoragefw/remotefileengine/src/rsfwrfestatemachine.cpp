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
* Description:  Base class for all operation state machines
*
*/


#include "mdebug.h"
#include "rsfwrfestatemachine.h"
#include "rsfwinterface.h"
#include "rsfwfiletable.h"
#include "rsfwrferequest.h"
#include "rsfwfileengine.h"

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::BaseConstructL
// ----------------------------------------------------------------------------
//
void CRsfwRfeStateMachine::BaseConstructL()
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::BaseConstructL"));
    iCompleteAndDestroyState =
        new (ELeave) CRsfwRfeStateMachine::TCompleteAndDestroyState(this);   
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::~CRsfwRfeStateMachine
// Note that normally when we come here iState = iCompleteAndDestroyState,
// only delete once
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::~CRsfwRfeStateMachine()
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::~CRsfwRfeStateMachine"));
    if (CurrentState() == CompleteAndDestroyState()) 
        {
        delete iState;
        }
    else 
        {
        delete iState;
        delete iCompleteAndDestroyState;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::ErrorOnStateEntry
// Entering the current state failed, so loop in EnterState() will replace 
// it by CompleteAndDestroy state as we return that state.
// Deleting the current state happens in EnterState()
// The state machine logic does not do that, as it allows us to eg resolve
// the problem by fixing the current state. 
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState* CRsfwRfeStateMachine::ErrorOnStateEntry(TInt aError) 
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::ErrorOnStateEntry"));
    iCompleteAndDestroyState->SetErrorCode(aError);
    return iCompleteAndDestroyState;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::ChangeState
// Enters aNextState, and makes it our current state.
// Deletes the previous state
// (assumes we never go backwards in the state machine)
// ----------------------------------------------------------------------------
//
void CRsfwRfeStateMachine::ChangeState(CRsfwRfeStateMachine::TState* aNextState)
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::ChangeState"));
    if (iState && (iState != aNextState) )
        {
        delete iState;
        iState = NULL;
        }
    EnterState(aNextState);
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::EnterState
// Enters aNextState, and makes it our current state.
// ----------------------------------------------------------------------------
//
void CRsfwRfeStateMachine::EnterState(CRsfwRfeStateMachine::TState* aNextState)
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::EnterState"));
    TInt err;
    while (aNextState) 
        {// State change required.
        iState = aNextState;
        TRAP(err, iState->EnterL());
        if (err)
            {
            aNextState = ErrorOnStateEntry(err);
            delete iState;
            iState = NULL;
            }
        else
            {
            aNextState = NULL;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::SetNextState
// Sets the current state, but doesn't actually enter it.
// ----------------------------------------------------------------------------
//
void CRsfwRfeStateMachine::SetNextState(TState* aNextState)
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::SetNextState"));
    iState = aNextState;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::ReEnterCurrentState
// Re-enters the state we're currently in.
// ----------------------------------------------------------------------------
//
void CRsfwRfeStateMachine::ReEnterCurrentState()  
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::ReEnterCurrentState"));
    EnterState(iState);
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::SetRequest
// Sets the backpointer to our request
// ----------------------------------------------------------------------------
//
void CRsfwRfeStateMachine::SetRequest(CRsfwRfeRequest* aRequest)
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::SetRequest"));
    iRFeRequest = aRequest;
    }
    
// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::Request 
// ----------------------------------------------------------------------------
//
CRsfwRfeRequest* CRsfwRfeStateMachine::Request()
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::Request"));
    return iRFeRequest;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::SetVolumes
// ----------------------------------------------------------------------------
//    
void CRsfwRfeStateMachine::SetVolumes(CRsfwVolumeTable* aImplementor)
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::SetVolumes"));
    iImplementor = aImplementor;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::SetFileEngine
// ----------------------------------------------------------------------------
//
void CRsfwRfeStateMachine::SetFileEngine(CRsfwFileEngine* aFileEngine) 
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::SetFileEngine"));
    iFileEngine = aFileEngine;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::SetArguments
// ----------------------------------------------------------------------------
//
void CRsfwRfeStateMachine::SetArguments(TRfeInArgs* aInArgs, TRfeOutArgs* aOutArgs)
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::SetArguments"));
    iInArgs = aInArgs;
    iOutArgs = aOutArgs;
    
    // Set the target node for this operation.
    if (iInArgs && iFileEngine) 
        {
        TFid* fidp = &(iInArgs->iFid);
        iFep = iFileEngine->iFileTable->Lookup(*fidp);  
        }
    else 
        {
        iFep = NULL;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::Volumes
// ----------------------------------------------------------------------------
//    
CRsfwVolumeTable* CRsfwRfeStateMachine::Volumes()
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::Volumes"));
    return iImplementor;
    }
  
// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::FileEngine
// ----------------------------------------------------------------------------
//    
CRsfwFileEngine* CRsfwRfeStateMachine::FileEngine()
    {
    return iFileEngine;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::Node
// ----------------------------------------------------------------------------
//
CRsfwFileEntry* CRsfwRfeStateMachine::Node() 
    {
    return iFep;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::ErrorOnStateExit
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState* CRsfwRfeStateMachine::ErrorOnStateExit(TInt aError) 
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::ErrorOnStateExit %d", aError));
    iCompleteAndDestroyState->SetErrorCode(aError);
    return iCompleteAndDestroyState;
    }
        
// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::HandleRemoteAccessResponse
// ----------------------------------------------------------------------------
//    
void CRsfwRfeStateMachine::HandleRemoteAccessResponse(TUint /*aId*/, TInt aStatus)
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::HandleRemoteAccessResponse"));
    TState* nextState = NULL;
    TRAPD(err, nextState = aStatus ? iState->ErrorL(aStatus)
          : iState->CompleteL());
    if (err)
        {
        nextState = ErrorOnStateExit(err);
        }
    ChangeState(nextState);
    }

// empty defaults for CRsfwRfeStateMachine::TState functions CompleteL(), ErrorL() and Cancel()

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::TState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState* CRsfwRfeStateMachine::TState::CompleteL()
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::TState::CompleteL"));
    return NULL;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::TState::ErrorL
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState* CRsfwRfeStateMachine::TState::ErrorL(TInt /*aCode*/) 
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::TState::ErrorL"));
    return NULL;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::TState::Cancel
// ----------------------------------------------------------------------------
//
void CRsfwRfeStateMachine::TState::Cancel() 
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::TState::Cancel"));
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::TCompleteAndDestroyState::TCompleteAndDestroyState
// Completes the remote request with error code aErrCode and
//  destroys it (CRsfwRfeRequest) 
// as well as the classes pointed to by it, that it the instances of 
// CRsfwRfeAsyncOperation, CRsfwRfeStateMachine and CRsfwRfeStateMachine::TState classes.
//
// In RemoteFS requests error code is 0,
// as the error is returned in iOutArgs->iResult
// For RemoteEngine API requests aErrCode should have the error code
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::
TCompleteAndDestroyState::TCompleteAndDestroyState(CRsfwRfeStateMachine* aParent,
                                                   TInt aErrCode)
    : iOperation(aParent), iErrCode(aErrCode)
    {
    DEBUGSTRING(("TCompleteAndDestroyState::TCompleteAndDestroyState"));
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::TCompleteAndDestroyState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwRfeStateMachine::TCompleteAndDestroyState::EnterL()
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::TCompleteAndDestroyState::EnterL"));
    iOperation->Request()->CompleteAndDestroy(iErrCode);   
    // nothing can be called here as we have been deallocated!!!    
    }

// ----------------------------------------------------------------------------
// CRsfwRfeStateMachine::TCompleteAndDestroyState::SetErrorCode
// ----------------------------------------------------------------------------
//
void CRsfwRfeStateMachine::TCompleteAndDestroyState::SetErrorCode(TInt aErrorCode) 
    {
    DEBUGSTRING(("CRsfwRfeStateMachine::TCompleteAndDestroyState::SetErrorCode"));
    iErrCode = aErrorCode;
    }

