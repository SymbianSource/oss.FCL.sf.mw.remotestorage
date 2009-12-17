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
* Description:  State machine for file lookup
*
*/


#ifndef C_RSFW_LOOKUPSTATEMACHINE_H
#define C_RSFW_LOOKUPSTATEMACHINE_H

#include "rsfwrfestatemachine.h"

/**
 *  state machine for file lookup
 *
 *
 */
class CRsfwLookupStateMachine : public CRsfwRfeStateMachine
    {
public:
    CRsfwLookupStateMachine();
    ~CRsfwLookupStateMachine();
public:
    // STATES
    // updating attributes
    // if the file type is unknow we first try file then directory
    class TUpdateKidAttributesTryFirstTypeState : public CRsfwLookupStateMachine::TState
        {
    public:
        TUpdateKidAttributesTryFirstTypeState(CRsfwLookupStateMachine* aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwLookupStateMachine *iOperation;                    
        };
        
    class TUpdateKidAttributesTrySecondTypeState : public CRsfwLookupStateMachine::TState
        {
    public:
        TUpdateKidAttributesTrySecondTypeState(CRsfwLookupStateMachine* aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwLookupStateMachine *iOperation;                    
        };
        
public:
    TState* CompleteL();    
    TState* CompleteRequestL(TInt aError);
        
public:
    // input parameters
    TUint iNodeType; // are we looking up file, directory or unknown...
        
    // output parameters:
    CRsfwFileEntry *iKidFep;
    
    TPtrC iKidName;    
    TFid* iParentFidp;  
    
    CRsfwDirEntAttr* iDirEntAttr;
    TBool iKidCreated;
    HBufC* iPath;
    };


#endif // C_RSFW_LOOKUPSTATEMACHINE_H