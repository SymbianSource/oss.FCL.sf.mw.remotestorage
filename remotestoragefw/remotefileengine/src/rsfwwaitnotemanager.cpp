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


// INCLUDE FILES
#include <pathinfo.h>
#include <CAknMemorySelectionDialog.h>
#include <RemoteFileEngine.rsg>
#include <aknnotpi.rsg>
#include <AknGlobalConfirmationQuery.h>
#include <StringLoader.h>
#include <CDirectoryLocalizer.h>
#include <AknGlobalNote.h>
#include <bautils.h>

#include "rsfwwaitnotemanager.h"
#include "rsfwrfeserver.h"
#include "sysutil.h"
#include "mdebug.h"


// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::CRsfwWaitNoteManager
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CRsfwWaitNoteManager::CRsfwWaitNoteManager() :
    CActive( CActive::EPriorityUserInput )
    {
    
    }

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::ConstructL()
    {
    DEBUGSTRING(("CRsfwWaitNoteManager::ConstructL"));
    CActiveScheduler::Add( this );
    iOpState = ERemoteWaitNoteStateOk;
    
    // read wait note txt:s from the localization file
    TFileName resourceFile;
    resourceFile.Copy(KResourceFile);
    BaflUtils::NearestLanguageFile(CRsfwRfeServer::Env()->iFs,resourceFile);
    
    DEBUGSTRING16(("opening resource file '%S'", &resourceFile));   
    iResourceFile.OpenL(CRsfwRfeServer::Env()->iFs, resourceFile);   
    iResourceFile.ConfirmSignatureL();
    DEBUGSTRING(("resource file ok")); 
    
    // pre-read the most common strings  	
   	HBufC8* warningNoteLabel = iResourceFile.AllocReadL(R_WAIT_NOTE_DISC_WARNING);
    CleanupStack::PushL(warningNoteLabel);
    iResourceReader.SetBuffer(warningNoteLabel);
    iNoteTxt = iResourceReader.ReadHBufCL();
    CleanupStack::PopAndDestroy(warningNoteLabel);

	iAvkonNoteId = 0;
   
    }

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::NewL()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
EXPORT_C CRsfwWaitNoteManager* CRsfwWaitNoteManager::NewL()
    {
    CRsfwWaitNoteManager* self = new ( ELeave ) CRsfwWaitNoteManager();

    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop();

    return self;
    }

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------
//
EXPORT_C CRsfwWaitNoteManager::~CRsfwWaitNoteManager()
    {
    Deque();
    iResourceFile.Close();
    
    
    if (iNoteTxt) 
    	{
    	delete iNoteTxt;
    	}
    if (iAuthRequest) 
        {
        delete iAuthRequest;
        }
    
    if (iGlobalNoteRequest) 
        {
        delete iGlobalNoteRequest;
        }

     if (iSaveToRequest) 
        {
        delete iSaveToRequest;
        iSaveToRequest = NULL;
        }

    iNotifier.Close();		
    
    }
    
// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::StartWaitNoteL
// Start to display a wait note.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CRsfwWaitNoteManager::StartWaitNoteL( 
	TRemoteOperationType aOpType,
	CRsfwWaitNoteStateMachine*  aOperation)
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::StartWaitNoteL"));
   
    // It is possible that when this function is called there already is 
    // a wait note. In this case we allow the new operation and its wait note
    // to override the ongoing concurrent operation's wait note. 
    // However, we want to protect some dialogs. 
    // These are 
    // 1) Wait note when uploading a file 
    // The reason for this is that the wait note is in fact the only indication
    // the user gets about the status of writing a file to the server.
    // 2) User query when uploading failed and we need to give time to read 
    // the dialogs about selecting a new location from a local media
    if (!((iOpState == ERemoteWaitNoteStateInProgress) && (iOpType == ERemoteSaveToLocal)))
    	{ 	    
 		iOperation = aOperation;
    
    	// cancel whatever is there currently
    	CancelWaitNoteL(iNoteId);
    	
    	DEBUGSTRING16(("CRsfwWaitNoteManager::CancelWaitNoteL returned"));
    	
    	// don't make asynchronous request when there has been already any!
    	if(!IsActive())
            {
        	if (aOpType <= ERemoteSaveToLocal) 
        	    {
        	    DEBUGSTRING16(("calling iNotifier.Connect()"));
        	    User::LeaveIfError(iNotifier.Connect());
        	    DEBUGSTRING16(("called iNotifier.Connect()"));
        	    }
      	    
        	switch( aOpType )
        		{  
        		// these "basic request" pass the method and drive friendly name
        		// drive friendly name is not needed for wait notes, only for retryrequest
        		// so there could be even more simple IPC struct without it
        		// (on the other hand, current param struct would easily allow showing the friendly name
        		// with the wait notes)
        		case ERemoteOpConnecting:
        	    case ERemoteOpDirDownloading:
        	    case ERemoteUnavailableRetry: 
        		    {
        		    iNotifier.StartNotifierAndGetResponse(iStatus, KRsfwNotifierPluginUID,
                        *iGlobalNoteRequest, *iGlobalNoteRequest);
        		    }
        		    break;
        	    case ERemoteWarnDisconnect:
        	        {
        	        CAknGlobalConfirmationQuery* iQuery = CAknGlobalConfirmationQuery::NewL();
                    iQuery->ShowConfirmationQueryL(iStatus, *iNoteTxt);
         	        break;							
        	        }
        	    case ERemoteOpAuthDialog:
        	        {
        	        DEBUGSTRING16(("calling ERemoteOpAuthDialog/ERemoteUnavailableRetry"));
        	        // same struct used for input and output params.
        	        iNotifier.StartNotifierAndGetResponse(iStatus, KRsfwNotifierPluginUID,
                        *iAuthRequest, *iAuthRequest);
        	        break;
        	        }

        		case ERemoteSaveToLocal:
        	    	DEBUGSTRING16(("calling ERemoteOpAuthDialog/ERemoteSaveToLocal"));
        		    iNotifier.StartNotifierAndGetResponse(iStatus, KRsfwNotifierPluginUID,
                        *iSaveToRequest, *iSaveToRequest);
        		    break;
        		}
    
    	    DEBUGSTRING16(("CRsfwWaitNoteManager calling SetActive()"));
    	    SetActive();
    	    
    	
            DEBUGSTRING16(("CRsfwWaitNoteManager::StartWaitNoteL function returns"));
       	    iOpType = aOpType;
            iOpState = ERemoteWaitNoteStateInProgress;
            return ++iNoteId;
    	    }
        else
            {
            DEBUGSTRING16(("CRsfwWaitNoteManager::StartWaitNoteL attempt to make request when object is active"));
            DEBUGSTRING16(("CRsfwWaitNoteManager::StartWaitNoteL function returns 0"));
    	    return 0;  // caller didn't "get" the note
            }
        }
    else
        {
        DEBUGSTRING16(("CRsfwWaitNoteManager::StartWaitNoteL function returns 0"));
        return 0;   // caller didn't "get" the note
        }
    }


// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::CancelWaitNote
// Cancel a wait note.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::CancelWaitNoteL(TInt aNoteId)
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::CancelWaitNoteL, iOpState = %d, iOpType = %d", iOpState, iOpType));		
    // Cancel a wait note if there is one and it is the same one that
    // the caller thinks it is removing...
    if ( (iOpState == ERemoteWaitNoteStateInProgress ) && (aNoteId == iNoteId))
        {
           
        if (iOpType <= ERemoteSaveToLocal) 
            { 
            // notifier plugin dialogs
            User::LeaveIfError(iNotifier.CancelNotifier(KRsfwNotifierPluginUID));
            iNotifier.Close();
            }
       
            
        // Dismiss qlobal query 
        if ( iQuery )
            {
            iQuery->CancelConfirmationQuery();
            delete iQuery;
            iQuery = NULL;
            }    
            

        
        iOpState = ERemoteWaitNoteStateOk;
        }
    
    }         
    
// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::RunL
// Handles an active object’s request completion event.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::RunL()
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::RunL, status=%d, optype=%d", iStatus.Int(), iOpType));
    CancelWaitNoteL(iNoteId);
    
    TInt errorCode = iStatus.Int();
    
    switch (iOpType) 
        {
        case ERemoteWarnDisconnect:
            if (iStatus == EAknSoftkeyYes)
                {
                errorCode = KErrNone;
                }
            else 
                {
                errorCode = KErrCancel;
                }
            break;
       case ERemoteOpAuthDialog:
            iAuthCredentials->iUserName = (*iAuthRequest)().iUserName;
            iAuthCredentials->iPassword = (*iAuthRequest)().iPassword;  
            delete iAuthRequest;
            iAuthRequest = NULL;
            break;
       case ERemoteUnavailableRetry:
       case ERemoteOpConnecting:
       case ERemoteOpDirDownloading:
            delete iGlobalNoteRequest;
            iGlobalNoteRequest = NULL; 
            break;
       case ERemoteSaveToLocal:
            iSaveParams->iFileName = (*iSaveToRequest)().iFileName;
            delete iSaveToRequest;
            iSaveToRequest = NULL;
            break;
        }
        
    if (iOperation) // is some operation waiting for info about this wait note?
        {
       if (((iOpType == ERemoteOpDirDownloading) || (iOpType == ERemoteOpConnecting))
            && (errorCode == KErrCancel))
                {
                 DEBUGSTRING16(("calling iOperation->CancelTransaction()"));
                // we received from the dialog information that user wants to cancel
                // call CancelTransaction on the access protocol module
                // this then must cancel our pending transaction
                // so we finally get to the ErrorL() state of the state machine
                iOperation->CancelTransaction();
                }
 
       else 
            {
            DEBUGSTRING16(("calling iOperation->HandleRemoteAccessResponse()"));
            iOperation->HandleRemoteAccessResponse(0, errorCode);
            }
        }
               
    }
    
// ----------------------------------------------------------------------------
// CRsfwWaitNoteManager::RunError
// ----------------------------------------------------------------------------
//
TInt CRsfwWaitNoteManager::RunError(TInt /*aError*/)
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::RunError"));		
    return KErrNone;
	}    
    
// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::DoCancel
// -----------------------------------------------------------------------------
//  
void CRsfwWaitNoteManager::DoCancel() 
	{	
	}  

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::ShowAddressNotFoundErrorL
// -----------------------------------------------------------------------------
// 	
void CRsfwWaitNoteManager::ShowAddressNotFoundErrorL(
    const TDesC& aFriendlyName) 
    {	
    DEBUGSTRING16(("CRsfwWaitNoteManager::ShowAddressNotFoundErrorL (drive '%S')", &aFriendlyName));	
	RResourceFile resourceFile;
	TResourceReader resourceReader; 
	
	// get the name of the localized resource file
	TFileName resourceFileName;
    resourceFileName.Copy(KResourceFile);
	BaflUtils::NearestLanguageFile(CRsfwRfeServer::Env()->iFs,resourceFileName);
	
	DEBUGSTRING16(("opening resource file '%S'", &resourceFileName)); 
	// read localized string from resource file
    resourceFile.OpenL(CRsfwRfeServer::Env()->iFs, resourceFileName);
    CleanupClosePushL(resourceFile);
    resourceFile.ConfirmSignatureL();
    DEBUGSTRING(("resource file ok")); 
    HBufC8* noteBuffer = resourceFile.AllocReadLC(R_NOTE_ADDRESS_NOT_AVAILABLE);
    resourceReader.SetBuffer(noteBuffer);
    HBufC* noteText = resourceReader.ReadHBufCL();
    CleanupStack::PushL(noteText);
    
    // S60 .loc formatting
    HBufC* formattedText = HBufC::NewLC(noteText->Length() + aFriendlyName.Length());  
    TPtr fprt = formattedText->Des();  
    StringLoader::Format(fprt, *noteText , -1, aFriendlyName);
    
    // error dialog
    CAknGlobalNote* errorDialog = CAknGlobalNote::NewLC();
    errorDialog->ShowNoteL(EAknGlobalErrorNote, *formattedText);
    CleanupStack::PopAndDestroy(5); // resourceFile, noteBuffer, noteText, formattedText, errordialog
	}


// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::SetAuthenticationDialogL
// -----------------------------------------------------------------------------
// 
void CRsfwWaitNoteManager::SetAuthenticationDialogL(TRsfwAuthenticationDlgRequest& aAuthRequest) 
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::SetAuthenticationDialogL"));
    if (iAuthRequest) 
        {
        delete iAuthRequest;
        iAuthRequest = NULL;
        }
    iAuthRequest = new (ELeave) TRsfwAuthParamsPckg(); 
    
    (*iAuthRequest)() = aAuthRequest;
    iAuthCredentials = &aAuthRequest;
    }

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::SetGlobalNoteRequestL
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::SetGlobalNoteRequestL(TRsfwNotPluginRequest& aRequestStruct)
    {
  	DEBUGSTRING16(("CRsfwWaitNoteManager::SetGlobalNoteRequestL"));
    if (iGlobalNoteRequest) 
        {
        delete iGlobalNoteRequest;
        iGlobalNoteRequest = NULL;
        }
    
    iGlobalNoteRequest = new (ELeave) TRsfwRetryParamsPckg();
    (*iGlobalNoteRequest)() = aRequestStruct;
    }

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::SetSaveToDialogRequestL
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::SetSaveToDialogRequestL(TRsfwSaveToDlgRequest& aSaveToRequest) 
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::SetSaveToDialogRequestL"));	
    if (iSaveToRequest) 
        {
        delete iSaveToRequest;
        iSaveToRequest = NULL;
        }
    
    iSaveToRequest = new (ELeave) TRsfwSaveToParamsPckg();
    
    (*iSaveToRequest)() = aSaveToRequest;
    iSaveParams = &aSaveToRequest;
    }

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::ShowFileSavedToDialogL
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::ShowFileSavedToDialogL(const TDesC& aPath )
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::ShowFileSavedToDialogL"));	
    HBufC8* textbuffer = iResourceFile.AllocReadL(R_CONFIRM_FILE_SAVED_TO);
    CleanupStack::PushL(textbuffer);
    iResourceReader.SetBuffer(textbuffer);
    HBufC* text = iResourceReader.ReadHBufCL();
    CleanupStack::PushL(text);
    HBufC* formattedText = NULL;
    
    // extract the path
    TParse parser;
    parser.Set(aPath, NULL, NULL);
    // try to get localized path
    CDirectoryLocalizer* localizer = CDirectoryLocalizer::NewL();
    CleanupStack::PushL(localizer);
    // S60 localizer requires also drive letter,
    // i.e. matches are like C:\\Data\\Images
    localizer->SetFullPath(parser.DriveAndPath());
    TPtrC localizedName = localizer->LocalizedName();
    
    if (localizedName != KNullDesC) 
        {
        formattedText = HBufC::NewMaxLC(text->Length() +
    								localizedName.Length());
    	
        TPtr formatPtr(formattedText->Des());
        StringLoader::Format(formatPtr, *text, -1, localizedName); 
        }
    else 
        {
        formattedText = HBufC::NewMaxLC(text->Length() +
    								aPath.Length());
    	
        TPtr formatPtr(formattedText->Des());
        StringLoader::Format(formatPtr, *text, -1, parser.Path());       
        }
    
   																				
    CAknGlobalNote* dlg = CAknGlobalNote::NewLC();
    dlg->ShowNoteL(EAknGlobalInformationNote, *formattedText );
    CleanupStack::PopAndDestroy(5); // textbuffer, text, localizer, formattedtext, dlg
    }

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::ShowFailedSaveNoteL
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::ShowFailedSaveNoteL() 
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::ShowFailedSaveNoteL"));
    ShowGlobalInformationNoteL(R_SAVING_FAILED);
    }

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::ShowFailedSaveNoteL
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::ShowNoNetworkCoverageNoteL() 
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::ShowNoNetworkCoverageNoteL"));
    ShowGlobalInformationNoteL(R_NO_NETWORK_COVERAGE);
    }

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::ShowOfflineNotPossibleNoteL
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::ShowOfflineNotPossibleNoteL() 
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::ShowOfflineNotPossibleNoteL")); 	
    ShowGlobalInformationNoteL(R_OFFLINE_NOT_POSSIBLE);
    }

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::ShowOutOfMemoryNoteL
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::ShowOutOfMemoryNoteL() 
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::ShowOutOfMemoryNoteL")); 	
    ShowGlobalInformationNoteL(R_RAM_OUT_OF_MEMORY);
    }

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::ShowGlobalInformationNoteL
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::ShowGlobalInformationNoteL(TInt aResourceId) 
    {
    HBufC8* textbuffer = iResourceFile.AllocReadL(aResourceId);
    CleanupStack::PushL(textbuffer);
    iResourceReader.SetBuffer(textbuffer);
    HBufC* text = iResourceReader.ReadHBufCL();
    CleanupStack::PushL(text);
    CAknGlobalNote* dlg = CAknGlobalNote::NewLC();
    dlg->ShowNoteL(EAknGlobalInformationNote, *text );
    CleanupStack::PopAndDestroy(3); // textBuffer, text, dlg
    }

// -----------------------------------------------------------------------------
// CRsfwWaitNoteManager::ResetOperation
// -----------------------------------------------------------------------------
//
void CRsfwWaitNoteManager::ResetOperation()
    {
    DEBUGSTRING16(("CRsfwWaitNoteManager::ResetOperation")); 	 	
    iOperation = NULL;
    }

// -----------------------------------------------------------------------------

//  End of File 
