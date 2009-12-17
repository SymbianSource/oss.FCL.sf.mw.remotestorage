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
* Description:  State machine for creating files
*
*/


#ifndef C_RSFW_CREATEFILESTATEMACHINE_H
#define C_RSFW_CREATEFILESTATEMACHINE_H

#include "rsfwrfestatemachine.h"

/**
 *  Creates a file
 *
 *  State machine for creating a file. Possibly could be combined with 
 *	CRsfwMkDirStateMachine, the differences are not that big...
 *
 */ 
class CRsfwCreateFileStateMachine : public CRsfwRfeStateMachine
    {
public: 
    CRsfwCreateFileStateMachine();
    ~CRsfwCreateFileStateMachine();
        
public:
    
    class TCheckIfExistsState : public CRsfwCreateFileStateMachine::TState
        {
    public:
        TCheckIfExistsState(CRsfwCreateFileStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwCreateFileStateMachine* iOperation;
        TInt iExclp;    
        }; 
        
    class TCreateNodeState : public CRsfwCreateFileStateMachine::TState
        {
    public:
        TCreateNodeState(CRsfwCreateFileStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwCreateFileStateMachine* iOperation;    
        };
      
    class TAcquireLockState : public CRsfwCreateFileStateMachine::TState
        {
    public:
        TAcquireLockState(CRsfwCreateFileStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwCreateFileStateMachine* iOperation;
    private:
        TBool iRequestedLock;   
        };
    
public:
    TState* CompleteRequestL(TInt aError);
    
    CRsfwFileEntry* iKidFep;
    TBool iKidCreated;
    // file open mode
    TUint iFlags;
    
    // used in TCheckIfExistsState
    CRsfwDirEntAttr* iDirEntAttr;   
    TDesC8* iLockToken;
    };


#endif // C_RSFW_CREATEFILESTATEMACHINE_H