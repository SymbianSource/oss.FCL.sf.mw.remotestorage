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
* Description:  State machine for closing a file
*
*/


#ifndef C_RSFW_CLOSESTATEMACHINE_H
#define C_RSFW_CLOSESTATEMACHINE_H


#include "rsfwwaitnotestatemachine.h"
#include "rsfwsavetodlgrequest.h"

/**
 *  State machine for closing a file
 *
 *  Consists of following states:
 *	- File is written to the server
 *	- Possible lock is released
 *	- If writing fails, file becomes "dirty" and
 *		is saved locally.
 *
 */

class CRsfwCloseStateMachine : public CRsfwWaitNoteStateMachine
    {
public:
    CRsfwCloseStateMachine();
    ~CRsfwCloseStateMachine();
        
public:
    // STATES
    class TReleaseLockState : public CRsfwCloseStateMachine::TState
        {
    public:
        TReleaseLockState(CRsfwCloseStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwCloseStateMachine* iOperation;
        };
        
    class TSaveLocallyState : public CRsfwCloseStateMachine::TState
        {
    public:
        TSaveLocallyState(CRsfwCloseStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
    	CRsfwCloseStateMachine* iOperation;
    	TRsfwSaveToDlgRequest iSaveToRequest;
        TBuf<KRsfwMaxFileSizeString> iFileSizeString;
        };
    
public: 
    TState* CompleteRequestL(TInt aError);
    TState* ErrorOnStateEntry(TInt aError);

public:
	TInt iFlags;
  };



#endif // C_RSFW_CLOSESTATEMACHINE_H