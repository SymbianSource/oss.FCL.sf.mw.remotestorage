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
* Description:  Base class for all operation state machines
*
*/

#ifndef C_RSFW_RFESTATEMACHINE_H
#define C_RSFW_RFESTATEMACHINE_H

#include <e32base.h>
#include "rsfwremoteaccess.h"

class CRsfwRfeRequest;
class CRsfwVolumeTable;
class CRsfwFileEngine;
class TRfeInArgs;
class TRfeOutArgs;
class CRsfwFileEntry;
class TFid;

const TInt KUpdateNotRequired = 5;

/**
 *  Base class for all operation state machines
 *
 */
class CRsfwRfeStateMachine : public CBase, public MRsfwRemoteAccessResponseHandler
    {
public:
    
    class TState
        {
    public: 
        virtual void EnterL()=0;// pure virtual
        virtual TState* CompleteL();
        virtual TState* ErrorL(TInt aCode);
        virtual void Cancel();
        };
        
    class TCompleteAndDestroyState : public CRsfwRfeStateMachine::TState 
        {
    public:
        TCompleteAndDestroyState(CRsfwRfeStateMachine* aOperation,
                                 TInt aErrCode = 0);
        void EnterL();  
        void SetErrorCode(TInt aErrorCode);
    private:
        CRsfwRfeStateMachine* iOperation;
        TInt iErrCode;
        };

public:
    void BaseConstructL();
    ~CRsfwRfeStateMachine();
    
    virtual TState* ErrorOnStateEntry(TInt aError);
    
    void ChangeState(TState* aNextState);
    void EnterState(TState* aNextState);
    void SetNextState(TState* aNextState);
    void ReEnterCurrentState(); 
    void SetRequest(CRsfwRfeRequest* aRequest);
    CRsfwRfeRequest* Request();
    inline TState* CurrentState(){return iState;};
    inline TCompleteAndDestroyState* CompleteAndDestroyState()
        {return iCompleteAndDestroyState;};
    
    // completes client's request
    virtual TState* CompleteRequestL(TInt aError)=0;
    
    void SetVolumes(CRsfwVolumeTable* aImplementor);
    void SetFileEngine(CRsfwFileEngine* aFileEngine);
    void SetArguments(TRfeInArgs* aInArgs, TRfeOutArgs* aOutArgs);
    CRsfwVolumeTable* Volumes();
    CRsfwFileEngine* FileEngine();
    CRsfwFileEntry* Node();
    virtual TState* ErrorOnStateExit(TInt aError);

    // from MRsfwRemoteAccessResponseHandler
    void HandleRemoteAccessResponse(TUint aId, TInt aStatus); 
    void DoCancel();
    
public:
    TRfeInArgs* iInArgs;
    TRfeOutArgs* iOutArgs;
    
private:
    CRsfwFileEntry* iFep;   // target file/directory parameter used by almost all state machines:
    TCompleteAndDestroyState* iCompleteAndDestroyState; // pre-created so that the request can always be completed (OOM situations etc.)
    TState*  iState; // our current state  
    CRsfwRfeRequest *iRFeRequest; // back pointer to the request we are running
    CRsfwVolumeTable* iImplementor; // class that implements the operations
    CRsfwFileEngine* iFileEngine; // the file engine    
    };
  


#endif // C_RSFW_RFESTATEMACHINE_H