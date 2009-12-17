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
* Description:  State machine for changing mount state, e.g. online->offline
*
*/

#ifndef C_RSFW_MOUNTCONNECTIONSTATEMACHINE_H
#define C_RSFW_MOUNTCONNECTIONSTATEMACHINE_H

#include "rsfwwaitnotestatemachine.h"
#include "rsfwvolume.h"

/**
 *  State machine for changing mount state, e.g. online->offline
 *  This state machine is currently only used for disconnecting
 *  For connecting CRsfWMountStateMachine is used
 *  Disconnecting does not currently send packets to network,
 *  but it is possible that this happens in the future.
 *
 */
class CRsfwMountConnectionStateMachine : public CRsfwWaitNoteStateMachine
    {
public:
    CRsfwMountConnectionStateMachine(TChar aDriveLetter, TUint aState);

public:
    // STATES
    class TChangeConnectionState : public CRsfwMountConnectionStateMachine::TState
        {
    public:
        TChangeConnectionState(CRsfwMountConnectionStateMachine* aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        // backpointer to the operation
        CRsfwMountConnectionStateMachine* iOperation;
        // 
        CRsfwVolume *iVolume;
        };
        
public:
    // parameters of the operation  
    TChar iDriveLetter;
    TUint iState;
        
    };    
    
#endif  // C_RSFW_MOUNTCONNECTIONSTATEMACHINE_H