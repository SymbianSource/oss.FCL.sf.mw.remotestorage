/*
* Copyright (c) 2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  States that use a global wait dialog
*
*/

  
#include "rsfwwaitnotestatemachine.h"
#include "rsfwvolumetable.h"
#include "rsfwwaitnotemanager.h"
#include "rsfwfileengine.h"
#include "rsfwvolume.h"
#include "mdebug.h"


// ----------------------------------------------------------------------------
// CRsfwWaitNoteStateMachine::CancelTransaction
// ----------------------------------------------------------------------------
//
void CRsfwWaitNoteStateMachine::CancelTransaction() 
	{	
	//if ((iTransactionId > 0) && FileEngine()) 
	if (FileEngine()) 
		{
		if ((iTransactionId > 0))
		    {
	        // with cancelled global wait notes the operation is
            // completed via access protocol Cancel
            // that is transaction is cancelled and the operation state machine
            // receives KErrCancel callback
		    FileEngine()->CancelTransaction(iTransactionId);
		    }
		else
		    {
		    HandleRemoteAccessResponse(0, KErrCancel);
		    }
    	}
    }

// ----------------------------------------------------------------------------
// CRsfwWaitNoteStateMachine::ShowWaitNoteL
// ----------------------------------------------------------------------------
//
void CRsfwWaitNoteStateMachine::ShowWaitNoteL(TRemoteOperationType aOperationType)
    {
    if (FileEngine() && FileEngine()->Volume())
        {
        switch (aOperationType) 
            {
             case ERemoteOpConnecting:
                iGlobalWaitNoteRequest.iMethod = TRsfwNotPluginRequest::EConnectingDlg;
                break;    
             case ERemoteOpDirDownloading:  
                iGlobalWaitNoteRequest.iMethod = TRsfwNotPluginRequest::EFetchingDlg;
                break; 
             case ERemoteUnavailableRetry:
                iGlobalWaitNoteRequest.iMethod = TRsfwNotPluginRequest::EUnavailableRetryDlg;
                break;
             }
            Volumes()->WaitNoteManager()->SetGlobalNoteRequestL(iGlobalWaitNoteRequest);
            iNoteId = Volumes()->WaitNoteManager()
                        ->StartWaitNoteL(aOperationType, this);       
        }
   else
        {// show note if Uri info not available
        iNoteId = Volumes()->WaitNoteManager()->StartWaitNoteL(aOperationType, this);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwWaitNoteStateMachine::DeleteWaitNoteL
// ----------------------------------------------------------------------------
//
void CRsfwWaitNoteStateMachine::DeleteWaitNoteL(TBool aCancelOpWait)
    {
    DEBUGSTRING16(("CRsfwWaitNoteStateMachine::DeleteWaitNoteL"));	
    if (aCancelOpWait) 
        {
         // let the wait note manager know that we are not expecting any event anymore
        Volumes()->WaitNoteManager()->ResetOperation(); 
        }
    if (iNoteId > 0) 
    	{
    	Volumes()->WaitNoteManager()->CancelWaitNoteL(iNoteId);
    	iNoteId = 0;	
    	}
    }

// ----------------------------------------------------------------------------
// CRsfwWaitNoteStateMachine::ErrorOnStateEntry
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState* CRsfwWaitNoteStateMachine::ErrorOnStateEntry(TInt aError)
    {
    TRAP_IGNORE(DeleteWaitNoteL(ETrue));
    return CRsfwRfeStateMachine::ErrorOnStateEntry(aError);
    }

// ----------------------------------------------------------------------------
// CRsfwWaitNoteStateMachine::ErrorOnStateExit
// ----------------------------------------------------------------------------
//
CRsfwRfeStateMachine::TState* CRsfwWaitNoteStateMachine::ErrorOnStateExit(TInt aError)
    {
    TRAP_IGNORE(DeleteWaitNoteL(ETrue));
    return CRsfwRfeStateMachine::ErrorOnStateExit(aError);
    }    
    
// ----------------------------------------------------------------------------
// CRsfwWaitNoteStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
//     
CRsfwRfeStateMachine::TState* CRsfwWaitNoteStateMachine::CompleteRequestL(
    TInt aError) 
    {   
    CompleteAndDestroyState()->SetErrorCode(aError);
    DeleteWaitNoteL(ETrue);
    return CompleteAndDestroyState(); 
    }
 


