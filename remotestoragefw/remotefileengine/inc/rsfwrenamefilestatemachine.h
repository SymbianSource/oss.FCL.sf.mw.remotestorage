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
* Description:  State machine for renaming files
*
*/


#ifndef C_RSFW_RENAMEFILESTATEMACHINE_H
#define C_RSFW_RENAMEFILESTATEMACHINE_H

#include "rsfwrfestatemachine.h"

/**
 *  Renames a file
 *
 *  State machine for renaming a file. Re-acquires possible lock.
 *
 */
 class CRsfwRenameFileStateMachine : public CRsfwRfeStateMachine
    {
public:
    CRsfwRenameFileStateMachine();
    ~CRsfwRenameFileStateMachine();
        
public: 
    class TRenameFileState : public CRsfwRenameFileStateMachine::TState
        {
    public:
        TRenameFileState(CRsfwRenameFileStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwRenameFileStateMachine* iOperation;    
        };
        
    class TAcquireLockState : public CRsfwRenameFileStateMachine::TState
        {
    public:
        TAcquireLockState(CRsfwRenameFileStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwRenameFileStateMachine* iOperation;
    private:
        TBool iRequestedLock; 
        };
             
public:
    TState* CompleteRequestL(TInt aError);
        
public:
    // rename or replace
    TBool iOverWrite;
    
    TPtrC iDstKidName;
        
    CRsfwFileEntry* iDstParentFep;
    CRsfwFileEntry* iSrcKidFep;
    CRsfwFileEntry* iDstKidFep;
        
    TBool iSrcKidCreated;
    TBool iDstKidCreated;
        
    CRsfwDirEntAttr* iDirEntAttr;    
    TDesC8* iLockToken;
    };


#endif // C_RSFW_RENAMEFILESTATEMACHINE_H