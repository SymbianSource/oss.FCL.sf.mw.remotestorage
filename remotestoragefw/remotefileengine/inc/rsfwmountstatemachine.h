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
* Description:  State machine for mounting
*
*/

#ifndef C_RSFW_MOUNTSTATEMACHINE_H
#define C_RSFW_MOUNTSTATEMACHINE_H

#include <rsfwmountentry.h> // KMaxMountNameLength

#include "rsfwwaitnotestatemachine.h"
#include "rsfwcontrol.h"
#include "rsfwauthenticationdlgrequest.h"

class CRsfwVolume;
class CRsfwVolumeTable;

/**
 *  State machine for mounting.
 *
 *  This state machine assumes that connection awareness is off.
 *  In "connection awareness" error state for mounting is quite different,
 *  as failure to connect to the server is not considered to be fatal,
 *  but instead puts the engine to disconnected mode.
 *  Feasible strategy is probably to create an alternative initial state
 *  with a different ErrorL, set if connection awareness is on.
 *
 */
class CRsfwMountStateMachine : public CRsfwWaitNoteStateMachine
    {
public:
    static CRsfwMountStateMachine* NewL(TRsfwMountConfig aMountConfig, 
                                        TInt aMountState,
                                        CRsfwVolumeTable* aVolumeTable);
private:
    void ConstructL(TRsfwMountConfig aMountConfig, 
                                        TInt aMountState,
                                        CRsfwVolumeTable* aVolumeTable);    
public:
    // STATES 
    // requesting connection state e.g. sending OPTIONS to WebDAV server 
    class TRequestConnectionState : public CRsfwMountStateMachine::TState
        {
    public:
        TRequestConnectionState(CRsfwMountStateMachine* aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        // backpointer to the operation
        CRsfwMountStateMachine* iOperation;
        };
        
    // dismiss the wait note    
    class TDismissConnectionWaitNoteState : public CRsfwMountStateMachine::TState
        {
    public:
        TDismissConnectionWaitNoteState(CRsfwMountStateMachine* aOperation);   
        void EnterL(); 
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        // backpointer to the operation
        CRsfwMountStateMachine* iOperation;       
        };    
    
    // asynchronously waits for user to type in authentication infomation    
    class TGetAuthCredentials : public CRsfwMountStateMachine::TState
        {
    public:
        TGetAuthCredentials(CRsfwMountStateMachine* aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        // backpointer to the operation
        CRsfwMountStateMachine* iOperation;
        TRsfwAuthenticationDlgRequest iAuthRequest;
        
        };    

      // asynchronously waits for "drive unavailabe, retry? query" 
    class TUnavailableRetry : public CRsfwMountStateMachine::TState
        {
    public:
        TUnavailableRetry(CRsfwMountStateMachine* aOperation);
        void EnterL();
        TState* CompleteL();
        TState* ErrorL(TInt aCode);
    private:
        // backpointer to the operation
        CRsfwMountStateMachine* iOperation;
        TRsfwNotPluginRequest iRetryRequest;
        };    

        
public: 
    CRsfwRfeStateMachine::TState* CompleteRequestL(TInt aError);
    TState* ErrorOnStateEntry(TInt aError);                                            
public:
    CRsfwVolume* iVolume; // volume to be mounter or recovered

    // parameters of the operation  
    // we read from mountconfig a drive letter to be mounted or recovered
    // currently volumeId is always set by us
    TRsfwMountConfig iMountConfig;
    TInt iVolumeId; 
    TInt iMountState;
private:        
    TBuf<KMaxMountNameLength> iFriendlyName;
    TBool iRequestingConnection; // flag indicating 'opening connection' event
    TInt iConnectingError; // remember why the connection attempt failed
    };


#endif // C_RSFW_MOUNTSTATEMACHINE_H