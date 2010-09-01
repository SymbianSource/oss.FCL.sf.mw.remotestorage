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


#ifndef C_RSFW_DELETESTATEMACHINE_H
#define C_RSFW_DELETESTATEMACHINE_H

#include "rsfwrfestatemachine.h"

/**
 *  Deletes a file or directory
 *
 *  State machine for deleting a file or directory.
 *
 */ 
 class CRsfwDeleteStateMachine : public CRsfwRfeStateMachine
    {
public:
    CRsfwDeleteStateMachine(TUint iNodeType);
    ~CRsfwDeleteStateMachine();
    
public:
    // STATES
    // TCheckIfCanBeDeleted: 
    // If a directory is not empty, it must not be deleted
    class TCheckIfCanBeDeleted : public CRsfwDeleteStateMachine::TState
        {
    public:
        TCheckIfCanBeDeleted(CRsfwDeleteStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwDeleteStateMachine* iOperation;  
        };
        
    class TDeleteNodeState : public CRsfwDeleteStateMachine::TState 
        {
    public:
        TDeleteNodeState(CRsfwDeleteStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwDeleteStateMachine* iOperation;    
        };
        
public:
    TState* CompleteRequestL(TInt aError);
        
    TBool iKidCreated;
    HBufC* iKidPath;
    CRsfwFileEntry* iKidFep;
        
    TUint iNodeType; // are we removing file or directory...
    
    // directory entries
    // used to fetch directory contents in TCheckIfCanBeDeleted
    RPointerArray<CRsfwDirEnt> iDirEnts;    
    }; 


#endif // C_RSFW_DELETESTATEMACHINE_H
