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
* Description:  State machine for creating directories
*
*/

#ifndef C_RSFW_MKDIRSTATEMACHINE_H
#define C_RSFW_MKDIRSTATEMACHINE_H

#include "rsfwrfestatemachine.h"

/**
 *  Creates a directory
 *
 *  State machine for creating directories. 
 *
 */ 
class CRsfwMkDirStateMachine : public CRsfwRfeStateMachine
    {
public:
    CRsfwMkDirStateMachine();
    ~CRsfwMkDirStateMachine();
    
public:
    // STATES
    class TCheckIfExistsState : public CRsfwMkDirStateMachine::TState
        {
    public:
        TCheckIfExistsState(CRsfwMkDirStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwMkDirStateMachine* iOperation;
        };
        
    class TMakeDirectoryState : public CRsfwMkDirStateMachine::TState 
        {
    public:
        TMakeDirectoryState(CRsfwMkDirStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwMkDirStateMachine* iOperation; 
        };
        
public:
    TState* CompleteRequestL(TInt aError);
        
    CRsfwFileEntry* iKidFep;
    TBool iKidCreated;
    CRsfwDirEntAttr* iDirEntAttr;
    }; 

 
 
#endif // C_RSFW_MKDIRSTATEMACHINE_H