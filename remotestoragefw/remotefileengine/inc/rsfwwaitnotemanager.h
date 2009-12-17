/*
* Copyright (c) 2005 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Global wait notes used in Remote File Engine
*
*/


#ifndef C_RSFWWAITNOTEMANAGER_H
#define C_RSFWWAITNOTEMANAGER_H

// INCLUDES
#include <e32base.h>
#include <barsc.h>
#include <barsread.h>
#include "rsfwwaitnotestatemachine.h"
#include "rsfwauthenticationdlgrequest.h"
#include "rsfwsavetodlgrequest.h"

// CONSTANTS
_LIT(KResourceFile, "Z:\\resource\\remotefileengine.rsc");

// FORWARD DECLARATIONS
class TRsfwAuthenticationDlgRequest;
class TRsfwAuthenticationDlgResponse;
class CAknGlobalConfirmationQuery;

// CLASS DECLARATION

/**
*  CRsfwWaitNoteManager class
*
*  Wait Note Manager class for Remote File Engine
*/
class CRsfwWaitNoteManager : public CActive                                       
    {
 public: // Constructors and destructor

        /**
        * Symbian OS two-phased constructor
        * @return Pointer to this component.
        */
        IMPORT_C static CRsfwWaitNoteManager* NewL();

        /**
        * C++ default destructor.
        */
        virtual ~CRsfwWaitNoteManager();

 private:

        /**
        * C++ default constructor.
        */
        CRsfwWaitNoteManager();

        /**
        * Symbian OS default constructor.
        */
        void ConstructL();

        void ShowGlobalInformationNoteL(TInt aResourceId);

 public: // New functions

        /**
        * Start to display wait note. 
        * Note: A new wait note can only be activated when the previous one is 
        *       dismissed.
        * @since 3.1
        * @param aOpType Wait Note operation type.
        */
        TInt StartWaitNoteL( TRemoteOperationType aOpType, 
        					 CRsfwWaitNoteStateMachine*  aOperation );


        /**
        * Server or path not found, when connecting 
        */
        void ShowAddressNotFoundErrorL(const TDesC& aFriendlyName);

		void SetAuthenticationDialogL(TRsfwAuthenticationDlgRequest& aAuthRequest);
		
		void SetGlobalNoteRequestL(TRsfwNotPluginRequest& aRequestStruct);
		
        void SetSaveToDialogRequestL(TRsfwSaveToDlgRequest& aSaveRequest);

        void ShowFileSavedToDialogL(const TDesC& aValue);
        
        void ShowFailedSaveNoteL();
 
        void ShowNoNetworkCoverageNoteL();
        
        void ShowOfflineNotPossibleNoteL();

        void ShowOutOfMemoryNoteL();
       
        // inform the wait note manager that no operation is waiting for 
        // it to trigger its state anymore
        void ResetOperation();
       
        /**
        * Cancel wait note.
        * Note: Please make sure the system is still able to handle key press 
        *       events during an operation.
        * @since 3.1
        */
        void CancelWaitNoteL(TInt aNoteId);
        
   
private:    // Functions from base classes

        /**
        * Handles an active object’s request completion event.
        */
        void RunL();  

 		/**
    	*Implements cancellation of an outstanding request.
		*/
		void DoCancel();

 		/**
    	* Called in case RunL() leaves
		*/
        TInt RunError(TInt aError);
        
private:    // Data

	    // Current operation type
        TRemoteOperationType	iOpType;
        
         // Operation state
        TRemoteWaitNoteStates	iOpState;
        
         // sequental id for the curren note
        // this is unique unlike CAknGlobalNote's id
        TInt					iNoteId;
                
        // standard global confirmation query
        CAknGlobalConfirmationQuery* 		iQuery;
        
         // note Id of the active CAknGlobalNote
        TInt 					iAvkonNoteId;
        
        // custom global notes server 
        RNotifier iNotifier;
        
        // IPC parameters struct for authentication dialog
        TRsfwAuthParamsPckg* iAuthRequest;
       
        // pointer to the mount state machines auth info
        // so that new username and/or passwd can be written
        TRsfwAuthenticationDlgRequest* iAuthCredentials;
                
        // IPC parameters struct for wait notes and the retry note
        TRsfwRetryParamsPckg *iGlobalNoteRequest;
        
        // IPC parameters struct for saveto dialog
        TRsfwSaveToParamsPckg* iSaveToRequest;
        
        // pointer to state machine's save to params
        TRsfwSaveToDlgRequest* iSaveParams;
               
        // buffer for the disconnect warning note txt
        HBufC*					iNoteTxt;
        
        // resource file reading
        RResourceFile iResourceFile;
        TResourceReader iResourceReader; 
        
		CRsfwWaitNoteStateMachine* iOperation;
    };

#endif  // REMOTEWAITNOTEMANAGER_H

// End of File