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
* Description:  State machine for fetching and caching files and directories
*
*/


#ifndef C_RSFW_FETCHANDCACHESTATEMACHINE_H
#define C_RSFW_FETCHANDCACHESTATEMACHINE_H

#include "rsfwwaitnotestatemachine.h"

/**
 *  State machine for fetching and caching files and directories
 *
 */
class CRsfwFetchAndCacheStateMachine : public CRsfwWaitNoteStateMachine
    {
public:
    CRsfwFetchAndCacheStateMachine();
    TState* ErrorOnStateExit(TInt aError);

public:
    // STATES
    // Before DoFetch we call UpdateAttributesL,
    // where it is decided whether cached data is used...
    // so this operation has only one state that fetches the data -
    // possibly from the server or then from the local cache
    class TFetchDataState : public CRsfwFetchAndCacheStateMachine::TState
        {
    public:
        TFetchDataState(CRsfwFetchAndCacheStateMachine* aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        CRsfwFetchAndCacheStateMachine* iOperation;
        };
        
public:
    TState* CompleteRequestL(TInt aError);

public:
    // input params
    // the first byte requested
    // = the current cached size
    TInt iFirstByte;

    // output params:
    // last byte of the container file after fetch
    TInt iLastByte;
    
    // directory entries, when used to fetch directory contents...
    RPointerArray<CRsfwDirEnt> iDirEnts;
    
    // length of the data fetched
    TInt iLength;
   
    };

#endif // C_RSFW_FETCHANDCACHESTATEMACHINE_H