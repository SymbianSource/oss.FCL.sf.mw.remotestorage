/*
* Copyright (c) 2007 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  State machine for flushing file contexts
*
*/


#ifndef C_RSFW_FLUSHSTATEMACHINE_H
#define C_RSFW_FLUSHSTATEMACHINE_H

#include "rsfwwaitnotestatemachine.h"
#include "rsfwsavetodlgrequest.h"

/**
 *  State machine for flushing a file
 *
 */

class CRsfwFlushStateMachine : public CRsfwWaitNoteStateMachine
    {
public:
    CRsfwFlushStateMachine();
        
public:
    // STATES
    class TFlushDataToServerState : public CRsfwFlushStateMachine::TState
        {
    public:
        TFlushDataToServerState(CRsfwFlushStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwFlushStateMachine* iOperation;
        };
    
public: 
    TState* CompleteL(TInt aError);
    TState* CompleteRequestL(TInt aError);
   
  };



#endif // C_RSFW_FLUSHSTATEMACHINE_H