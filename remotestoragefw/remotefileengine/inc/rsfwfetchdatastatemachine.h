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
* Description:  State machine for fetching data without caching it permanently
*
*/

#ifndef C_RSFW_FETCHDATASTATEMACHINE_H
#define C_RSFW_FETCHDATASTATEMACHINE_H

#include "rsfwwaitnotestatemachine.h"

/**
 *  State machine for fetching data without caching it permanently.
 *
 *  Fetches data to a temporary cache file, i.e. the data
 *  does not become part of the permanent cache.
 *
 */
class CRsfwFetchDataStateMachine : public CRsfwWaitNoteStateMachine
    {
public:
    CRsfwFetchDataStateMachine();
    
public:
    // STATES
    class TFetchDataState : public CRsfwFetchDataStateMachine::TState
        {
    public:
        TFetchDataState(CRsfwFetchDataStateMachine* aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwFetchDataStateMachine* iOperation;
        };
public:
    TState* CompleteRequestL(TInt aError);
    
public:
    // directory entries, when used to fetch directory contents...
    RPointerArray<CRsfwDirEnt> iDirEnts;
    // output params
    TDesC* iCacheName;
    TInt iLength;
    };

#endif // C_RSFW_FETCHDATASTATEMACHINE_H