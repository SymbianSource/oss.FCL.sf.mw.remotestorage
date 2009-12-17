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


#include <rsfwmountman.h>
#include "rsfwmountstatemachine.h"
#include "rsfwwaitnotemanager.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwfileengine.h"
#include "rsfwrferequest.h"
#include "rsfwnotpluginrequest.h"
#include "mdebug.h"
#include <ecomerrorcodes.h>

// CONSTANTS
// errors returned by connection layer
const TInt KRsfwErrNoNetworkCoverage = -4159; // device out of range
const TInt KRsfwErrOfflineNotPossible = -4180; // device in off-line mode

/*************************************
 * CRsfwMountStateMachine
 *
 * Note that EnteredConnectionState() in CompleteL
 * may start reintegration.
 * However instead of writing special reintegration states here
 * it is better to later implement a reintegration thread.
 *
 ***************************************/


// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::NewL
// ----------------------------------------------------------------------------
// 
CRsfwMountStateMachine* 
CRsfwMountStateMachine::NewL(TRsfwMountConfig aMountConfig,
                             TInt aMountState,
                             CRsfwVolumeTable* aVolumeTable)
    {
    CRsfwMountStateMachine* self = new (ELeave) CRsfwMountStateMachine();
    CleanupStack::PushL(self);
    self->ConstructL(aMountConfig, aMountState, aVolumeTable);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::ConstructL
// ----------------------------------------------------------------------------
// 
void CRsfwMountStateMachine::ConstructL(TRsfwMountConfig aMountConfig, 
                                        TInt aMountState,
                                        CRsfwVolumeTable* aVolumeTable)
    {
    DEBUGSTRING16(("Mounting drive '%c' with uri '%S', flags=0x%x, to=%d",
                   TUint(aMountConfig.iDriveLetter),
                   &(aMountConfig).iUri,
                   aMountConfig.iFlags,
                   aMountConfig.iInactivityTimeout));

    iMountConfig = aMountConfig;
    iMountState = aMountState;
    iFriendlyName = iMountConfig.iName; 
    SetVolumes(aVolumeTable);    
        
    iVolumeId = Volumes()->VolumeIdByDriveLetter(iMountConfig.iDriveLetter);        
    if (iVolumeId == KErrNotFound)
        {
        User::Leave(KErrArgument);
        }
    
    // set volume and file engine
    if (iMountState == KMountStateDormant)
        {
        // volume and file engine must exist
        CRsfwVolume* volume = Volumes()->VolumeByVolumeId(iVolumeId);
        if (volume) 
            {
            iVolume = volume;
            }
        else 
            {
            User::Leave(KErrNotFound);
            }
        }
    else 
        {
        // create volume 
        iVolume = new (ELeave) CRsfwVolume();   
        iVolume->iVolumeTable = Volumes();
        
        // set mountinfo for the volume
         TRsfwMountInfo& mountInfo = iVolume->iMountInfo;
         mountInfo.iMountConfig = iMountConfig;
         mountInfo.iMountStatus.iVolumeId = iVolumeId;
         iVolume->iVolumeTable = Volumes();

        if (mountInfo.iMountConfig.iInactivityTimeout < 0)
            {
            // Negative value means that we don't want to time out
            mountInfo.iMountConfig.iInactivityTimeout = 0;
             }
        else if (mountInfo.iMountConfig.iInactivityTimeout > 0)
            {
            // Positive means using system default
            mountInfo.iMountConfig.iInactivityTimeout =
                     Volumes()->iInactivityTimeout;
            }
        // Just copy the adapted parameter
        mountInfo.iMountStatus.iInactivityTimeout =
            mountInfo.iMountConfig.iInactivityTimeout;

        mountInfo.iMountStatus.iPermanence = Volumes()->iPermanence;
        if (iMountConfig.iFlags & KMountFlagOffLine)
            {
            // We are working offline
            mountInfo.iMountStatus.iConnectionState = KMountNotConnected;
            }
        else
            {
            mountInfo.iMountStatus.iConnectionState = KMountStronglyConnected;
            }
        
        // create fileengine and attack to volume
        iVolume->iFileEngine = CRsfwFileEngine::NewL(iVolume);
        }
        
    // set also fileengine pointer in the operation state machine, 
    // so that the operation can be cancelled
    SetFileEngine(iVolume->iFileEngine); 
    
        
    }

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
// 
CRsfwRfeStateMachine::TState* CRsfwMountStateMachine::CompleteRequestL(TInt aError) 
    { 
    DEBUGSTRING16(("CRsfwMountStateMachine:::CompleteRequestL error %d", aError));
    // Attach volume to the table
    // Note that this is always done, even if "request_connection_state"
    // returned an error.
    // This is because sending packets to remote server required construction
    // of CRsfwFileEngine etc. anyway, and they will be deleted from elsewhere
    // (deleting the mount configuration or the inactivity timeout)
    if (!(iMountState == KMountStateDormant)) 
        {
        Volumes()->iVolumes[iVolumeId] = iVolume;
        }
    
    // make sure engine disconnects if there was an error
    if (aError != KErrNone) 
        {
        iVolume->iFileEngine->RequestConnectionStateL(
                    KMountNotConnected,
                    NULL);
        }

    return CRsfwWaitNoteStateMachine::CompleteRequestL(aError);
    }

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::ErrorOnStateEntry
// ----------------------------------------------------------------------------
// 
CRsfwWaitNoteStateMachine::TState* CRsfwMountStateMachine::ErrorOnStateEntry(TInt aError) 
    {
    DEBUGSTRING16(("CRsfwMountStateMachine::ErrorOnStateEntry %d", aError));
   
    if (aError == KEComErrNoInterfaceIdentified)
        {
        aError = KErrNotFound;
        }

    // If error when connecting: 
    // For most errors we have to show "drive not available, retry?"
    // or some other connection error dialog
    // for this we must first dismiss the current wait note and then show
    // a new note.
    // Hower, if user presses Cancel or the red button we return immediately
    if ((iRequestingConnection) && (aError != KErrCancel))
        {
        iRequestingConnection = EFalse;
        iConnectingError = aError;
        return new CRsfwMountStateMachine::TDismissConnectionWaitNoteState(this);
        }
    else
        {
        return CRsfwWaitNoteStateMachine::ErrorOnStateEntry(aError);    
        }    
    }

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TRequestConnectionState::TRequestConnectionState
// ----------------------------------------------------------------------------
// 
CRsfwMountStateMachine::
TRequestConnectionState::TRequestConnectionState(CRsfwMountStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TRequestConnectionState::EnterL
// ----------------------------------------------------------------------------
// 
void CRsfwMountStateMachine::TRequestConnectionState::EnterL()
    {
    DEBUGSTRING16(("CRsfwMountStateMachine::TRequestConnectionState::EnterL"));
    
    // set volume mountconfig 
    iOperation->iVolume->iMountInfo.iMountConfig = iOperation->iMountConfig;
       
    // put up a 'Connecting...' global wait note

    iOperation->ShowWaitNoteL( ERemoteOpConnecting );

    // mark mount connection state as KMountConnecting
    // this is done, because will prevent engine from shutdown
    iOperation->FileEngine()->EnteredConnectionStateL(KMountConnecting, EFalse);
   
    // request KMountStronglyConnected state
    // this will open connection to the server
    iOperation->iRequestingConnection = ETrue;
    TUint transactionId = iOperation->FileEngine()
	          ->RequestConnectionStateL(KMountStronglyConnected,
                                        iOperation);
        
    // transactionId = 0 means syncronous non-cancellable operation  
    if (transactionId > 0) 
        {
        iOperation->iRequestingConnection = EFalse;    
    	iOperation->iTransactionId = transactionId;
        }
    
    }
   
// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TRequestConnectionState::CompleteL
// This function should be able to do what CRsfwVolumeTable::MountL normally does
// after fileEngine->RequestConnectionStateL() has returned
// (with non-error return code)
// ----------------------------------------------------------------------------
//     
CRsfwRfeStateMachine::TState*
CRsfwMountStateMachine::TRequestConnectionState::CompleteL()
    {  
    DEBUGSTRING16(("CRsfwMountStateMachine::TRequestConnectionState::CompleteL"));
        
    if (iOperation->iRequestingConnection)
        {
        iOperation->iRequestingConnection = EFalse;
        }
       
    if (!(iOperation->iMountState == KMountStateDormant)) 
        {
        // Attach volume to the table
        iOperation->Volumes()->iVolumes[iOperation->iVolumeId] =
            iOperation->iVolume;
        }
        
    // Attach volume to the request
    // (such that the request can notify File Engine of operation completion)
    iOperation->Request()->iVolume = iOperation->iVolume;

    iOperation->FileEngine()->EnteredConnectionStateL(KMountStronglyConnected, EFalse);

    // publish the new connection state (P&S notification)
    iOperation->Volumes()->PublishConnectionStatus(iOperation->iVolume);
         
    // Remember the volume id for the next root setup
    iOperation->Volumes()->iLastVolumeId = iOperation->iVolumeId;
   
    // remember why the connection attempt failed
    iOperation->iConnectingError = KErrNone;
 
   // dismiss the connecting dialog
     return new CRsfwMountStateMachine::TDismissConnectionWaitNoteState(
            iOperation);  
   
    }


// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TRequestConnectionState::ErrorL
// ----------------------------------------------------------------------------
// 
CRsfwRfeStateMachine::TState*
CRsfwMountStateMachine::TRequestConnectionState::ErrorL(TInt aCode)
    {
    DEBUGSTRING16(("CRsfwMountStateMachine::TRequestConnectionState error=%d", aCode));

    if (iOperation->iRequestingConnection)
        {
        iOperation->iRequestingConnection = EFalse;
        }
    
    // remember why the connection attempt failed
    iOperation->iConnectingError = aCode;
    
    if ((aCode == KErrNone) || (aCode == KErrCancel)) 
        {
        // immediately return 
        return iOperation->CompleteRequestL(aCode);
        }

    // else remove wait note dialog first
     return new CRsfwMountStateMachine::TDismissConnectionWaitNoteState(
            iOperation);  
    }


// *************

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::
// TDismissConnectionWaitNoteState::TDismissConnectionWaitNoteState
// ----------------------------------------------------------------------------
// 
CRsfwMountStateMachine::
TDismissConnectionWaitNoteState::TDismissConnectionWaitNoteState(CRsfwMountStateMachine* aParent)
    : iOperation(aParent)
        {
        }

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TDismissConnectionWaitNoteState::EnterL
// ----------------------------------------------------------------------------
// 
void CRsfwMountStateMachine::TDismissConnectionWaitNoteState::EnterL() 
    {
    DEBUGSTRING16(("CRsfwMountStateMachine::TDismissConnectionWaitNoteState::EnterL"));	  
    iOperation->DeleteWaitNoteL(EFalse);    
    }

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TDismissConnectionWaitNoteState::CompleteL
// ----------------------------------------------------------------------------
// 
CRsfwRfeStateMachine::TState*
CRsfwMountStateMachine::TDismissConnectionWaitNoteState::CompleteL()
    {
    DEBUGSTRING16(("CRsfwMountStateMachine::TDismissConnectionWaitNoteState::CompleteL"));
    switch (iOperation->iConnectingError) 
        {
        case KErrNone:
           return iOperation->CompleteRequestL(KErrNone);
        case KErrCancel:
           return iOperation->CompleteRequestL(KErrCancel);
        case KErrAccessDenied:
           return new CRsfwMountStateMachine::TGetAuthCredentials(iOperation);
        case KErrNoMemory:
            iOperation->Volumes()->WaitNoteManager()
                ->ShowOutOfMemoryNoteL();
            return iOperation->CompleteRequestL(KErrNoMemory);
        case KErrNotFound:
        case KErrPathNotFound:
            iOperation->Volumes()->WaitNoteManager()
                ->ShowAddressNotFoundErrorL(iOperation->iFriendlyName);
            return iOperation->CompleteRequestL(iOperation->iConnectingError);
        case KRsfwErrNoNetworkCoverage:
            iOperation->Volumes()->WaitNoteManager()
	            ->ShowNoNetworkCoverageNoteL();
	        return iOperation->CompleteRequestL(KRsfwErrNoNetworkCoverage);    
        case KRsfwErrOfflineNotPossible:
            iOperation->Volumes()->WaitNoteManager()
	            ->ShowOfflineNotPossibleNoteL(); 
	        return iOperation->CompleteRequestL(KRsfwErrOfflineNotPossible);
        default:
           // ask user whether to retry
            return new CRsfwMountStateMachine::TUnavailableRetry(iOperation);
        }
    }
    
//-----------------------------------------------------------------------------
// CRsfwMountStateMachine::TDismissConnectionWaitNoteState::ErrorL
// ----------------------------------------------------------------------------
// 
CRsfwRfeStateMachine::TState*
CRsfwMountStateMachine::TDismissConnectionWaitNoteState::ErrorL(TInt /*aCode*/)
    {
    DEBUGSTRING16(("CRsfwMountStateMachine::TDismissConnectionWaitNoteState::ErrorL"));		
    // dismissing the dialog failed
    // no code for this case
    return CompleteL();
    }

// *************

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TGetAuthCredentials::TGetAuthCredentials
// ----------------------------------------------------------------------------
// 
CRsfwMountStateMachine::
TGetAuthCredentials::TGetAuthCredentials(CRsfwMountStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TGetAuthCredentials::EnterL
// ----------------------------------------------------------------------------
// 
void CRsfwMountStateMachine::TGetAuthCredentials::EnterL()
    {
    DEBUGSTRING16(("CRsfwMountStateMachine::TGetAuthCredentials::EnterL"));
    iAuthRequest.iMethod = TRsfwNotPluginRequest::EAuthenticationDlg;
    iAuthRequest.iDriveName = iOperation->iFriendlyName;
    iAuthRequest.iUserName = iOperation->iMountConfig.iUserName;
    iAuthRequest.iPassword = iOperation->iMountConfig.iPassword;
           
    iOperation->Volumes()->WaitNoteManager()->SetAuthenticationDialogL(iAuthRequest);
    iOperation->Volumes()->WaitNoteManager()
            ->StartWaitNoteL(ERemoteOpAuthDialog, iOperation);    
    }

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TGetAuthCredentials::CompleteL
// ----------------------------------------------------------------------------
//     
CRsfwRfeStateMachine::TState*
CRsfwMountStateMachine::TGetAuthCredentials::CompleteL()
    {
    DEBUGSTRING16(("CRsfwMountStateMachine::TGetAuthCredentials::CompleteL"));
    // re-set username and password and try connecting again
    iOperation->iMountConfig.iUserName = iAuthRequest.iUserName;
    iOperation->iMountConfig.iPassword = iAuthRequest.iPassword;   
     
    return new CRsfwMountStateMachine::TRequestConnectionState(iOperation);  
    }
    
// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TGetAuthCredentials::ErrorL
// ----------------------------------------------------------------------------
// 
CRsfwRfeStateMachine::TState*
CRsfwMountStateMachine::TGetAuthCredentials::ErrorL(TInt /*aCode*/)
    {    
    DEBUGSTRING16(("CRsfwMountStateMachine::TGetAuthCredentials::ErrorL"));
    return iOperation->CompleteRequestL(KErrAccessDenied);
    }
    
// **************


// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TUnavailableRetry::TGetAuthCredentials
// ----------------------------------------------------------------------------
// 
CRsfwMountStateMachine::
TUnavailableRetry::TUnavailableRetry(CRsfwMountStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TUnavailableRetry::EnterL
// ----------------------------------------------------------------------------
// 
void CRsfwMountStateMachine::TUnavailableRetry::EnterL()
    {
    DEBUGSTRING16(("CRsfwMountStateMachine::TUnavailableRetry::EnterL"));
    iRetryRequest.iMethod = TRsfwNotPluginRequest::EUnavailableRetryDlg;
    iRetryRequest.iDriveName = iOperation->iFriendlyName;
    
    iOperation->Volumes()->WaitNoteManager()->SetGlobalNoteRequestL(iRetryRequest);
    iOperation->Volumes()->WaitNoteManager()
            ->StartWaitNoteL(ERemoteUnavailableRetry, iOperation);         
    }

// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TUnavailableRetry::CompleteL
// ----------------------------------------------------------------------------
//     
CRsfwRfeStateMachine::TState*
CRsfwMountStateMachine::TUnavailableRetry::CompleteL()
    {
    DEBUGSTRING16(("CRsfwMountStateMachine::TUnavailableRetry::CompleteL"));	
    // retry
    return new CRsfwMountStateMachine::TRequestConnectionState(iOperation);  
    }
    
// ----------------------------------------------------------------------------
// CRsfwMountStateMachine::TUnavailableRetry::ErrorL
// ----------------------------------------------------------------------------
// 
CRsfwRfeStateMachine::TState*
CRsfwMountStateMachine::TUnavailableRetry::ErrorL(TInt aCode)
    {
    DEBUGSTRING16(("CRsfwMountStateMachine::TUnavailableRetry::ErrorL"));
    if (aCode == KErrCancel) 
        {
        // user cancelled the a dialog
        return iOperation->CompleteRequestL(KErrCancel);
        }
    else 
        {
        return iOperation->CompleteRequestL(aCode);
        }
    }
