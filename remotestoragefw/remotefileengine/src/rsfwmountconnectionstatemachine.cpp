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
* Description:  State machine for changing mount state, e.g. online->offline
*
*/


#include "rsfwmountconnectionstatemachine.h"
#include "rsfwvolumetable.h"
#include "rsfwfileengine.h"
#include "rsfwfiletable.h"
#include "rsfwlockmanager.h"
#include "rsfwwaitnotemanager.h"
#include "mdebug.h"
#include "rsfwrfeserver.h"

/**********************************************************
 * CRsfwMountConnectionStateMachine
 *
 * If connect to server fails, deletes the remote accessor 
 * (this comes from the way synch version fo CRsfwFileEngine::ConnectL() used to
 * to work, but I'm not quite sure how correct behaviour this is).
 * Note that EnteredConnectionState() in CompleteL
 * may start reintegration.
 * However instead of writing special reintegration states here
 * it is better to later implement a reintegration thred.
 *********************************************************/


// ----------------------------------------------------------------------------
// CRsfwMountConnectionStateMachine::CRsfwMountConnectionStateMachine
// ----------------------------------------------------------------------------
// 
CRsfwMountConnectionStateMachine::CRsfwMountConnectionStateMachine(TChar aDriveLetter,
                                                           TUint aState)
    {
    iDriveLetter = aDriveLetter;
    iState = aState;
    }


// ----------------------------------------------------------------------------
// CRsfwMountConnectionStateMachine::TChangeConnectionState::TChangeConnectionState
// ----------------------------------------------------------------------------
//  
CRsfwMountConnectionStateMachine::TChangeConnectionState::TChangeConnectionState(
    CRsfwMountConnectionStateMachine* aParent)
    : iOperation(aParent)
    {}

// ----------------------------------------------------------------------------
// CRsfwMountConnectionStateMachine::TChangeConnectionState::EnterL
// ----------------------------------------------------------------------------
// 
void CRsfwMountConnectionStateMachine::TChangeConnectionState::EnterL()
    {   
    // The user has forced us to go offline/online
    iVolume =
        iOperation->Volumes()->VolumeByDriveLetter(iOperation->iDriveLetter);
    if (iVolume)
        {
        DEBUGSTRING(("Set state of volume %d to %d",
                     iVolume->iMountInfo.iMountStatus.iVolumeId,
                     iOperation->iState));

        // Note that eventually the volume state may differ
        // from the engine state
        if ((iVolume->iFileEngine->ConnectionState() == KMountStronglyConnected)
            && (iOperation->iState == KMountNotConnected))
            {
            // request to move from connected to disconnected state
            // warn user if this volume contains open files
           if (iVolume->iFileEngine->iFileTable->OpenFileCount() > 0) 
                {
                iOperation->Volumes()->WaitNoteManager()
                    ->StartWaitNoteL(ERemoteWarnDisconnect, iOperation);  
                }
           else 
                {
                // actual move happens in CRsfwMountConnectionStateMachine::TChangeConnectionState::CompleteL
                iOperation->HandleRemoteAccessResponse(0, KErrNone);     
                }
            
            }
        else 
            {
            // currently setting mount state to Connected state not supported
            // but user is assumed to use Mount() instead
            // this used to work, but authentication options and retry dialogs
            // are now missing
            User::Leave(KErrNotSupported);                                
            }
        }
    else 
        {
        if (iOperation->iState == KMountNotConnected) 
            {
            // when volume is not yet found, we execute dormant mount
            TInt err;
            TInt drivenumber;
            TRsfwMountConfig mountConfig;
            CRsfwRfeServer::Env()->iFs.CharToDrive(iOperation->iDriveLetter, drivenumber);
            mountConfig.iDriveLetter = iOperation->iDriveLetter;
            err = iOperation->Volumes()->GetMountConfigL(mountConfig);
            if (!err) 
                {
                iOperation->Volumes()->MountDormantL(mountConfig, drivenumber);
                iOperation->HandleRemoteAccessResponse(0, KErrNone);     
                }
            else 
                {
                iOperation->HandleRemoteAccessResponse(0, err); 
                }
            }
        else 
            {
            User::Leave(KErrNotSupported);  
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMountConnectionStateMachine::TChangeConnectionState::CompleteL
// ----------------------------------------------------------------------------
//     
CRsfwMountConnectionStateMachine::TState*
CRsfwMountConnectionStateMachine::TChangeConnectionState::CompleteL()
    {  
    // from CRsfwFileEngine::RequestConnectionStateL
    if (iVolume && iVolume->iFileEngine) 
        {
        
        // we successfully disconnected
        iVolume->iFileEngine->RequestConnectionStateL(
                     KMountNotConnected,
                      NULL);
        
        if (iVolume->iFileEngine->ConnectionState() != iOperation->iState)
            {
            iVolume->iFileEngine->EnteredConnectionStateL(iOperation->iState, EFalse);
            }
        }
    
    if (iVolume) 
        {
        // from CRsfwVolumeTable::SetMountConnectionStateL  
        iVolume->iMountInfo.iMountStatus.iConnectionState =
            iVolume->iFileEngine->ConnectionState();     
        }
    
    return iOperation->CompleteRequestL(KErrNone);
    }

// ----------------------------------------------------------------------------
// CRsfwMountConnectionStateMachine::TChangeConnectionState::ErrorL
// ----------------------------------------------------------------------------
// 
CRsfwMountConnectionStateMachine::TState*
CRsfwMountConnectionStateMachine::TChangeConnectionState::ErrorL(TInt aCode)
    {
    return iOperation->CompleteRequestL(aCode);
    }

