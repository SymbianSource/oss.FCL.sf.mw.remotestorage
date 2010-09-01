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
* Description:  State machine for getting attributes of a file
*
*/

#ifndef C_RSFW_GETATTRIBUTESSTATEMACHINE_H
#define C_RSFW_GETATTRIBUTESSTATEMACHINE_H


#include "rsfwgetattributesstatemachine.h"
#include "rsfwattributerefreshingstatemachine.h"

class CRsfwGetAttributesStateMachine : public CRsfwAttributeRefreshingStateMachine
    {
public:
    CRsfwGetAttributesStateMachine();
    
public:
    // STATES
    // get remote attributes
    class TRefreshAttributesState :
        public CRsfwGetAttributesStateMachine::TState
        {
    public:
        TRefreshAttributesState(CRsfwAttributeRefreshingStateMachine *aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        TState* CompleteOurRequestL(TInt aErr);
    private:
        // iOperation = CRsfwGetAttributesStateMachine or COpenByPath 
        CRsfwAttributeRefreshingStateMachine* iOperation;    
        };     
        
public:   
    TState* CompleteRequestL(TInt aError);   
  };

#endif // C_RSFW_GETATTRIBUTESSTATEMACHINE_H