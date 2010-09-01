/*
* Copyright (c) 2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  States that use a global wait dialog
*
*/

#ifndef C_RSFW_WAITNOTESTATEMACHINE_H
#define C_RSFW_WAITNOTESTATEMACHINE_H

#include "rsfwrfestatemachine.h"
#include "rsfwnotpluginrequest.h"

// Wait Note States
enum TRemoteWaitNoteStates
    {
    ERemoteWaitNoteStateOk = 0,
    ERemoteWaitNoteStateInProgress
    };
    
// Operation type supported for wait note
enum TRemoteOperationType
    {
    ERemoteOpIdle = 0,
    ERemoteOpConnecting,
    ERemoteOpDirDownloading,
    ERemoteOpAuthDialog,
    ERemoteUnavailableRetry,
    ERemoteSaveToLocal,
    ERemoteWarnDisconnect,
    };

/**
 *  Parent class for states that use global wait dialogs
 *
 */  
 class CRsfwWaitNoteStateMachine : public CRsfwRfeStateMachine   
    {
 public: 
    void CancelTransaction();
    void ShowWaitNoteL(TRemoteOperationType aResourceId); 
    void DeleteWaitNoteL(TBool aCancelOpWait);
    TState* ErrorOnStateEntry(TInt aError);
    TState* ErrorOnStateExit(TInt aError);
    TState* CompleteRequestL(TInt aError);
 public: 
    TRsfwNotPluginRequest iGlobalWaitNoteRequest;
    TUint iTransactionId; // for cancelling requests
    TUint iNoteId; 	// id of the global note
    };    
    
#endif // C_RSFW_WAITNOTESTATEMACHINE_H