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
* Description:  State machine for opening a file or directory
*
*/


#ifndef C_RSFW_OPENBYPATHSTATEMACHINE_H
#define C_RSFW_OPENBYPATHSTATEMACHINE_H

#include "rsfwattributerefreshingstatemachine.h"

class TDirEntAttr;

/**
 *  State machine for OpenByPath operation.
 *
 *  Assumes FID is not yet cached. Note that here we know FID <-> pathname 
 *  association and in principle the file with that pathname should exist
 * 	as this call has been preceded by a call to Entry()
 *   - of course, there is a possibility that the file has been deleted from 
 *  the server
 *
 */
class CRsfwOpenByPathStateMachine : public CRsfwAttributeRefreshingStateMachine
    {
public:
    CRsfwOpenByPathStateMachine();
    ~CRsfwOpenByPathStateMachine();
    
public:
    //STATES
    // requiring opening in the relevant mode - e.g. obtain a write lock
    class TRequestOpenModeState : public CRsfwOpenByPathStateMachine::TState
        {
    public:
        TRequestOpenModeState(CRsfwOpenByPathStateMachine* aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwOpenByPathStateMachine* iOperation;
        TBool iRequestedLock;
        };
        
public:
    TState* CompleteRequestL(TInt aError);
    
public: 
    // input parameters;
    TBool iRealOpen;
    // file open mode
    TUint iFlags;
                
    //output params:
    TDesC* iCacheName;
    TDirEntAttr* iAttrp;
    TDesC8* iLockToken;
    };


#endif // C_RSFW_OPENBYPATHSTATEMACHINE_H