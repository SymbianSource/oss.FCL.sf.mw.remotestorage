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
* Description:  RSFW notifier server plugin
*
*/


#include <AknNotifyStd.h>
#include <eikenv.h>
#include <AknQueryDialog.h>
#include <AknGlobalNote.h>
#include <ConeResLoader.h>
#include <rsfwnotplugindlg.rsg>
#include <StringLoader.h>
#include <CAknMemorySelectionDialog.h>
#include <CAknFileSelectionDialog.h>
#include <sysutil.h>
#include <bautils.h>
#include <AknWaitDialog.h>
#include <uikon/eiksrvui.h> // CEikServAppUi

#include "rsfwnotplugindlg.h"
#include "rsfwnotpluginnamedialog.h"
#include "mdebug.h"

#include <implementationproxy.h> 

_LIT(KResourceFile, "z:\\resource\\RsfwNotPluginDlg.RSC"); // emulator


void CreateNotifierL(CArrayPtr<MEikSrvNotifierBase2>* aNotifiers)
    {
    MEikSrvNotifierBase2* notifier;
    notifier = CRsfwNotPluginDlg::NewL();
    CleanupStack::PushL(notifier);
    aNotifiers->AppendL(notifier);
    CleanupStack::Pop(notifier);
    }

EXPORT_C CArrayPtr<MEikSrvNotifierBase2>* NotifierArray()
    {
    CArrayPtrFlat<MEikSrvNotifierBase2>* notifiers
        = new CArrayPtrFlat<MEikSrvNotifierBase2>(1);
    if (notifiers)
        {
        TRAPD(err, CreateNotifierL(notifiers));
        if(err)
            {
            notifiers->ResetAndDestroy();
            delete notifiers;
            notifiers = NULL;
            }
        }
    return(notifiers);
    }



CRsfwNotPluginDlg* CRsfwNotPluginDlg::NewL()
    {
    CRsfwNotPluginDlg* dlg = new(ELeave)CRsfwNotPluginDlg();
    CleanupStack::PushL(dlg);
    dlg->ConstructL();
    CleanupStack::Pop(dlg);
    return dlg;
    }

CRsfwNotPluginDlg::~CRsfwNotPluginDlg()
    {
    Cleanup();
    iFs.Close(); 
    if (iWaitDialog) 
        {
        delete iWaitDialog;
        }
    // just in case make sure the app key is unblocked
    UnblockAppSwitching();
    }
    
void CRsfwNotPluginDlg::Cleanup()
    {
    
    iMethod = TRsfwNotPluginRequest::ENoMethod;
    iCancelled = EFalse;  
    iReplySlot = NULL;  
 
    
    if (iUserName)
        {
        delete iUserName;
        iUserName = NULL;
        }
     
    if (iPassword)
        {
        delete iPassword;
        iPassword = NULL;
        }
    
    if (iCurrentRootPath)  
        {
        delete iCurrentRootPath;
        iCurrentRootPath = NULL;
        }
    
    
    if (iMemDialog) 
        {
        delete iMemDialog;
        iMemDialog = NULL;
        }
        
    if (iFileName) 
        {
        delete iFileName;
        iFileName = NULL;
        }
        
    if (iAuthRequest)
        {
        delete iAuthRequest;
        iAuthRequest = NULL;
        }
        
    if (iSaveToRequest) 
        {
        delete iSaveToRequest;
        iSaveToRequest = NULL;
        }
        
    if (iWaitDialog) 
        {
        delete iWaitDialog;
        iWaitDialog = NULL;
        }
       
    }

CRsfwNotPluginDlg::CRsfwNotPluginDlg() : CActive(EPriorityStandard),
    iMethod(TRsfwNotPluginRequest::ENoMethod)
    {
    CActiveScheduler::Add(this);
    }

void CRsfwNotPluginDlg::ConstructL()
    {
    User::LeaveIfError(iFs.Connect());
    iAppSwitchingBlocked = EFalse;
    }

void CRsfwNotPluginDlg::RunL()
    {
    if(iMethod == TRsfwNotPluginRequest::ENoMethod)
        return; // Notifier canceled or request signaled by other means

    HandleAsyncRequestL(); // show dialog
    }

TInt CRsfwNotPluginDlg::RunError(TInt aError)
    {
    DEBUGSTRING16(("CRsfwNotPluginDlg::RunError, error=%d",aError));
    UnblockAppSwitching();
    iMessage.Complete(aError);
    Cleanup();
    return KErrNone;
    }

void CRsfwNotPluginDlg::DoCancel()
    {
    }

/**
 * Called when all resources allocated by notifiers shoudl be freed.
 */
void CRsfwNotPluginDlg::Release()
    {
    delete this;
    }

/**
 * Called when a notifier is first loaded to allow any initial construction that is required.
 */
MEikSrvNotifierBase2::TNotifierInfo CRsfwNotPluginDlg::RegisterL()
    {
    iInfo.iUid = KRsfwNotifierPluginUID;
    // Because there are some longer wait notes here, we use low priority 
    // (lower than ENotifierPriorityVLow). This allows e.g. VPN credentials dialog on
    // top of the "Connecting..." wait note.
    iInfo.iChannel = EAknNotifierChannelProgressDialog; 
    iInfo.iPriority = ENotifierPriorityVLow; 
    return iInfo;
    }

/**
 * Return the priority a notifier takes and the channels it acts on.  The return value may be varied
 * at run-time.
 */
MEikSrvNotifierBase2::TNotifierInfo CRsfwNotPluginDlg::Info() const
    {
    return iInfo;
    }

/**
 * Start the notifier with data aBuffer and return an initial response.
 */
TPtrC8 CRsfwNotPluginDlg::StartL(const TDesC8& /*aBuffer*/)
    {
    //Create and launch confirmation dialog using static function.
    //The function returns True when the OK button is pressed.
    return TPtrC8(NULL, 0);
    }

void CRsfwNotPluginDlg::HandleAsyncRequestL()
    {
    // Load resource file
    CEikonEnv* eikEnv = CEikonEnv::Static();
    TFileName filename(KResourceFile); 
    RConeResourceLoader resLoader(*eikEnv);
    User::LeaveIfError(resLoader.Open(filename));
    CleanupClosePushL(resLoader);
	TInt result;
    TBool okpressed = EFalse;
    switch(iMethod)
        {
        
        case TRsfwNotPluginRequest::EAuthenticationDlg:
            DEBUGSTRING16(("CRsfwNotPluginDlg::HandleAsyncRequestL EAuthenticationDlg"));
            okpressed = ShowAuthenticationDialogL();
            DEBUGSTRING16(("CRsfwNotPluginDlg::::ShowAuthenticationDialogL returned %d", okpressed));
            break;  
        case TRsfwNotPluginRequest::EUnavailableRetryDlg:
           DEBUGSTRING16(("CRsfwNotPluginDlg::HandleAsyncRequestL EUnavailableRetryDlg"));
            okpressed  = ShowUnavailableRetryNoteL();  
           break;
        case TRsfwNotPluginRequest::ESaveToDlg:
            DEBUGSTRING16(("CRsfwNotPluginDlg::HandleAsyncRequestL ESaveToDlg"));
            okpressed = ShowSaveToDlgL();
            break;
        case TRsfwNotPluginRequest::EConnectingDlg:
        case TRsfwNotPluginRequest::EFetchingDlg:
            ShowWaitNoteL();
            break;
        default:
            break;
            
        }

// some message dialogs will be completed here, 
// others from the DialogDismissedL callback
if (iMethod  <=  TRsfwNotPluginRequest::EUnavailableRetryDlg) 
    {
    if(iCancelled)
        {
        iMessage.Complete(KErrCancel);
        }
    else
        {
        if(okpressed) 
            {
            if (iMethod == TRsfwNotPluginRequest::EAuthenticationDlg) 
                {
                iMessage.WriteL(iReplySlot, *iAuthRequest);
                }
            else if (iMethod == TRsfwNotPluginRequest::ESaveToDlg) 
                {
                iMessage.WriteL(iReplySlot, *iSaveToRequest);
                }
            
            result = KErrNone;
            }
        else 
            {
            result = KErrCancel;
            }
        iMessage.Complete(result);
        }
    }

    CleanupStack::PopAndDestroy(&resLoader);

    }

/**
 * Start the notifier with data aBuffer.  aMessage should be completed when the notifier is deactivated.
 * May be called multiple times if more than one client starts the notifier.  The notifier is immediately
 * responsible for completing aMessage.
 */

void CRsfwNotPluginDlg::StartL(const TDesC8& aBuffer, TInt aReplySlot, const RMessagePtr2& aMessage)
    {
    if(iMethod != TRsfwNotPluginRequest::ENoMethod)
        {
        aMessage.Complete(KErrInUse);
        return;
        }

    Cleanup();
    
     // Read incoming parameters
    TRsfwNotPluginRequest params;
    TPckgC<TRsfwNotPluginRequest> pckg( params );
    pckg.Set( aBuffer );
    iMethod = pckg().iMethod;
    iDriveName = pckg().iDriveName;
   
    
    if (iMethod == TRsfwNotPluginRequest::EAuthenticationDlg) 
        {
        TRsfwAuthenticationDlgRequest authparams;
        TPckgC<TRsfwAuthenticationDlgRequest> authpckg( authparams );
        authpckg.Set( aBuffer );
            
        // read parameters
        iUserName = HBufC::NewL(KRsfwMaxUsernameLength);
        TPtr username = iUserName->Des();
        username.Append(authpckg().iUserName);
        iPassword = HBufC::NewL(KRsfwMaxPasswordLength);
        TPtr psswd= iPassword->Des();
        psswd.Append(authpckg().iPassword);           
        }
    else if (iMethod == TRsfwNotPluginRequest::ESaveToDlg)
        {
        TRsfwSaveToDlgRequest saveToparams;
        TPckgC<TRsfwSaveToDlgRequest> savepckg( saveToparams );
        savepckg.Set( aBuffer );
            
        // read parameters
        iFileName = HBufC::NewL(KMaxFileName);
        TPtr filename = iFileName->Des();
        filename.Append(savepckg().iFileName);
        iCacheDrive = savepckg().iCacheDrive;
        TLex lex(savepckg().iFileSize);
        lex.Val(iFileSize);
        }
   
   
    iMessage = aMessage; 
    iReplySlot = aReplySlot;
    iStatus = KRequestPending;
    TRequestStatus* status = &iStatus;
    SetActive();
    User::RequestComplete(status, KErrNone);

    }


TBool CRsfwNotPluginDlg::ShowAuthenticationDialogL()
    {
    DEBUGSTRING16(("CRsfwNotPluginDlg::::ShowAuthenticationDialogL"));
    	  	
    TBool returnValue = EFalse;
	HBufC* firstprompt = NULL;
	HBufC* secondprompt = NULL;
    firstprompt = StringLoader::LoadLC( R_RD_QUERY_USERNAME, iDriveName );
	DEBUGSTRING16(("loaded firstprompt"));
	secondprompt = StringLoader::LoadLC( R_RD_QUERY_PASSWORD);	
		
	DEBUGSTRING16(("loaded secondprompt"));
	iAuthRequest = new (ELeave) TRsfwAuthParamsPckg();
	TPtr username = iUserName->Des();
    TPtr password = iPassword->Des();
    CAknMultiLineDataQueryDialog* dlg = CAknMultiLineDataQueryDialog::NewL(username,password);
    CleanupStack::PushL(dlg);
    DEBUGSTRING16(("created dialog"));
    dlg->SetPromptL(*firstprompt, *secondprompt);
    DEBUGSTRING16(("set prompts"));
    CleanupStack::Pop(dlg);
    dlg->SetMaxLengthOfFirstEditor(KRsfwMaxUsernameLength);
    dlg->SetMaxLengthOfFirstEditor(KRsfwMaxPasswordLength);
    iDialog = dlg;
    
    BlockAppSwitching();
    TBool okButtonSelected = dlg->ExecuteLD(R_CUSTOM_USERNAMEPASSWD_DIALOG);
    UnblockAppSwitching();

    
    if (okButtonSelected) 
        {
        returnValue = ETrue;
        (*iAuthRequest)().iUserName = username;
        (*iAuthRequest)().iPassword = password;
        }
    
    CleanupStack::PopAndDestroy(2, firstprompt); // secondprompt, firstprompt
    return returnValue;
    
    }
    
TBool CRsfwNotPluginDlg::ShowSaveToDlgL()
    { 
    
    TPtr filename = iFileName->Des();  
  
    CDesCArrayFlat* strings = new (ELeave) CDesCArrayFlat( 2 );
    CleanupStack::PushL( strings );
    strings->AppendL( filename );
    strings->AppendL( iDriveName );    
    
    TBool dialogCancelled = EFalse;
    TBool fileSelected = EFalse;
    iCurrentRootPath = HBufC::NewL(KMaxPath);
    TPtr rootptr = iCurrentRootPath->Des();
    TPtr folderptr(NULL, 0);
    
    TInt saveSelection;
    iSaveToRequest = new (ELeave) TRsfwSaveToParamsPckg();
    CAknQueryDialog* note = CAknQueryDialog::NewL();
    CleanupStack::PushL(note);
    HBufC* saveprompt = StringLoader::LoadLC( R_RD_FILE_SAVE_FAIL, *strings);
    note->SetPromptL(*saveprompt);
    CleanupStack::PopAndDestroy(saveprompt);
    CleanupStack::Pop(note);

    BlockAppSwitching();            
    saveSelection = note->ExecuteLD(R_CUSTOM_SAVE_QUERY);   
    UnblockAppSwitching();

    CleanupStack::PopAndDestroy(strings); // strings
   
    if (saveSelection == EAknSoftkeySave) 
        {    
        while (!fileSelected && !dialogCancelled)
            {
             CAknMemorySelectionDialog::TMemory selectedMem = 
                                            CAknMemorySelectionDialog::EPhoneMemory;
            if (iMemDialog) 
                {
                delete iMemDialog;
                iMemDialog = NULL;
                }  
                                          
             iMemDialog = CAknMemorySelectionDialog::NewL(ECFDDialogTypeSave, EFalse);
             CAknCommonDialogsBase::TReturnKey retvalue;
        
             iMemDialog->SetObserver(this);
             retvalue = iMemDialog->ExecuteL(selectedMem, &rootptr, &folderptr); 
             if (retvalue) 
                {
                CAknFileSelectionDialog* filedialog = CAknFileSelectionDialog::NewL(ECFDDialogTypeSave);
                CleanupStack::PushL(filedialog);
                HBufC* dialogtxt = NULL;
                dialogtxt = StringLoader::LoadLC( R_RD_SELECT_DIR_BACK );
                filedialog->SetRightSoftkeyRootFolderL(*dialogtxt);
                fileSelected = filedialog->ExecuteL(rootptr);
                CleanupStack::PopAndDestroy(2, filedialog); // dialogtxt, filedialog
                }
             else 
                {
                dialogCancelled = ETrue;
                }
            }
        
        }
       else
        {
        dialogCancelled = ETrue;
        }
    
    if (!dialogCancelled) 
        {
        dialogCancelled = !GetValidNameL(rootptr, filename);
      
        }
    
    rootptr.Append(filename);    
    
    if (!dialogCancelled) 
        {
        (*iSaveToRequest)().iFileName= rootptr;
        }
               
        
    return !dialogCancelled;
    
    }

void CRsfwNotPluginDlg::ShowWaitNoteL() 
    {
    if (iWaitDialog) 
        {
        delete iWaitDialog;
        iWaitDialog = NULL;
        }
      
      // We set visibilityDelayOff
      // As we show wait dialog only for remote operations
      // we can assumet that the length of the operation is always
     //  over 1.5 seconds..
      iWaitDialog = new( ELeave ) CAknWaitDialog(
            reinterpret_cast< CEikDialog** >( &iWaitDialog ),
            ETrue );

    
    // if user cancels the wait note, this is received via the callbakc.
    iWaitDialog->SetCallback(this);  
            
    switch (iMethod) 
        {
        case TRsfwNotPluginRequest::EConnectingDlg:
            // 'app key' will be unblocked in DialogDismissedL()
            BlockAppSwitching();
            iWaitDialog->ExecuteLD(R_CONNECTING_WAIT_NOTE);
            break;
        case TRsfwNotPluginRequest::EFetchingDlg:
            // 'app key' will be unblocked in DialogDismissedL()
            BlockAppSwitching();
            iWaitDialog->ExecuteLD(R_FETCHING_WAIT_NOTE); 
            break;
        }   
    }
    

TBool CRsfwNotPluginDlg::ShowUnavailableRetryNoteL()
    {
    HBufC* retryprompt = NULL;
    retryprompt = StringLoader::LoadLC( R_RD_DRIVE_UNAVAILABLE, iDriveName );
    
    CAknQueryDialog* note = CAknQueryDialog::NewL();
    CleanupStack::PushL(note);
    note->SetPromptL(*retryprompt);
    CleanupStack::Pop(note);

    BlockAppSwitching();
    TBool retryButtonSelected = note->ExecuteLD(R_CUSTOM_RETRY_QUERY);    
    UnblockAppSwitching();    

    CleanupStack::PopAndDestroy(retryprompt);
    if (retryButtonSelected) 
        {
        return ETrue;
        }
    else 
        {
        return EFalse;
        }
    }



TBool CRsfwNotPluginDlg::OkToExitL( CAknMemorySelectionDialog::TMemory aMemory ) 
    {
    TBool returnValue = EFalse;
    TPtr rootptr = iCurrentRootPath->Des();
    TPtr folderptr(NULL, 0);
    iMemDialog->GetMemories( aMemory, &rootptr, &folderptr);
  
    TDriveUnit selectedDrive(iCurrentRootPath[0]);
    TDriveUnit cacheDrive(iCacheDrive);
    if (selectedDrive == cacheDrive) 
        {
        // just move between one drive
        returnValue = ETrue;
        }
    else if (aMemory == CAknMemorySelectionDialog::EMemoryCard) 
        {
        if (SysUtil::MMCSpaceBelowCriticalLevelL(&iFs, iFileSize)) 
            {
            ShowDiskFullNoteL(EFalse);
            }
        else 
            {
            returnValue = ETrue;
            }
        }
    else if (aMemory == CAknMemorySelectionDialog::EPhoneMemory) 
        {
        if (SysUtil::FFSSpaceBelowCriticalLevelL(&iFs, iFileSize))
            {
            ShowDiskFullNoteL(ETrue);
            }
        else 
            {
            returnValue = ETrue;
            }
        }
     else 
        {
        // only allow memorycard or phone memory 
        returnValue = EFalse;
        }
    return returnValue;
    }

TBool CRsfwNotPluginDlg::GetValidNameL(TDesC& aPath, TDes& aName)
    {
    HBufC* fullPath = HBufC::NewLC(KMaxFileName);
    TPtr pathPtr= fullPath->Des();
    pathPtr.Append(aPath);
    pathPtr.Append(aName);
    TBool renameFile = EFalse;
    TBool userCancelled = EFalse;
    TBool overwriteSelection = EFalse;
  
 
    while (BaflUtils::FileExists(iFs, pathPtr) && !userCancelled && !overwriteSelection) 
        {
        TUint32 fileType( 0 );
        fileType = FileTypeL( pathPtr ); 
        // returns KEntryAttReadOnly if file is read only or open
        CAknQueryDialog* note = CAknQueryDialog::NewL();
        CleanupStack::PushL(note);
        HBufC* queryprompt;
        if (fileType & KEntryAttReadOnly) 
            {
            TBool retValue;
            queryprompt = StringLoader::LoadLC( R_RD_ITEM_RENAME_QUERY, aName);
            note->SetPromptL(*queryprompt);
            CleanupStack::PopAndDestroy(queryprompt);
            CleanupStack::Pop(note);
            retValue = note->ExecuteLD(R_RSFW_PLUGIN_RENAME_QUERY);
            if (retValue) 
                {
                renameFile = ETrue;
                }
            else 
                {
                userCancelled = ETrue;
                }
            }
        else 
            {
            TBool retValue;
            queryprompt = StringLoader::LoadLC( R_RD_ITEM_OVERWRITE_QUERY, aName);
            note->SetPromptL(*queryprompt);
            CleanupStack::PopAndDestroy(queryprompt);
            CleanupStack::Pop(note);
            retValue = note->ExecuteLD(R_RSFW_PLUGIN_OVERWRITE_QUERY);  
            if (!retValue) 
                {
                renameFile = ETrue;
                }
            else 
                {
                overwriteSelection = ETrue;
                }
            }
        
        
        if (renameFile) 
            {
            TBool retval;
            CRsfwNotPluginNameDialog* dlg = 
                CRsfwNotPluginNameDialog::NewL( 
                pathPtr, aName, iFs);
            dlg->SetMaxLength(KMaxFileName - aPath.Length());
            dlg->PrepareLC( R_RSFW_NOT_PLUGIN_FILE_NAME_QUERY );
            retval = dlg->RunLD();
            if (!retval) 
                {
                userCancelled = ETrue;
                }
            else 
                {
                // reset the path after user renamed the file
                CleanupStack::PopAndDestroy(fullPath);
                fullPath = HBufC::NewLC(KMaxFileName);
                pathPtr= fullPath->Des();
                pathPtr.Append(aPath);
                pathPtr.Append(aName);
                }
            }
        
        }
    
    CleanupStack::PopAndDestroy(fullPath);   
        
    if (!userCancelled) 
        {
        return ETrue;
        }
    else 
        {
        return EFalse;
        }
    
 
    }




// ---------------------------------------------------------
// CRsfwNotPluginDlg::ShowDiskFullNoteL
// Show an out of disk note.
// ---------------------------------------------------------
//
void CRsfwNotPluginDlg::ShowDiskFullNoteL( TBool aInternal )
    {

    HBufC* message = NULL;

    if ( aInternal )
        {
        message = StringLoader::LoadLC( R_RSFWPLUGIN_NOT_ENOUGH_MEMORY );
        }
    else
        {
        message = StringLoader::LoadLC( R_RSFWPLUGIN_MMC_NOT_ENOUGH_MEMORY );
        }

    TRequestStatus status = KErrNone;
	CAknGlobalNote* note = CAknGlobalNote::NewL();
    CleanupStack::PushL( note );
    note->SetSoftkeys( R_AVKON_SOFTKEYS_OK_EMPTY );
	note->ShowNoteL( status, EAknGlobalErrorNote, *message );
    User::WaitForRequest( status );

    CleanupStack::PopAndDestroy( 2, message ); // note, message
    }


// ---------------------------------------------------------------------------
// The notifier has been deactivated so resources can be freed and outstanding messages completed.
// ---------------------------------------------------------------------------
//
void CRsfwNotPluginDlg::Cancel()
    {
    DEBUGSTRING(("CRsfwNotPluginDlg::Cancel"));        
    TRAP_IGNORE(CancelL()); 
    DEBUGSTRING(("exiting CRsfwNotPluginDlg::Cancel"));
    }

// ---------------------------------------------------------------------------
// Called by Cancel() in order to catch the possible leaves.
// ---------------------------------------------------------------------------
//
void CRsfwNotPluginDlg::CancelL()
    {
    if ((iMethod  >=  TRsfwNotPluginRequest::EConnectingDlg) &&
        iWaitDialog) 
        {
        DEBUGSTRING(("calling ProcessFinishedL()"));
        iDialogDismissedCalled = EFalse;
        iWaitDialog->ProcessFinishedL();
        // iWaitDialog->ProcessFinishedL() should call 
        // dialogdismissed, but for some reason this does not always
        // happen (really looks and feels like a )
        // this extra help should save the day
        if (!iDialogDismissedCalled) 
            {
            DEBUGSTRING(("extra call to ProcessFinishedL()"));
            DialogDismissedL(EAknSoftkeyDone);
            }
        }
    Cleanup();
    }

// ---------------------------------------------------------------------------
// Sets KEntryAttReadOnly if the file is read only, system or open
// ---------------------------------------------------------------------------
//
TUint32 CRsfwNotPluginDlg::FileTypeL( const TDesC& aFullPath ) const
    {
    TUint32 fileType(0);
 // Full check for local and removable drives
    TEntry entry;
    TInt err( iFs.Entry( aFullPath, entry ) );

    // Check if item was deleted outside this component
    if ( err == KErrNotFound || err == KErrPathNotFound )
        {
        User::Leave( err );
        }

    TBool fileOpen( EFalse );
    iFs.IsFileOpen( aFullPath, fileOpen );
    if ( fileOpen || entry.IsReadOnly() || entry.IsSystem() )
        {
        fileType |= KEntryAttReadOnly;
        }
                
    return fileType;            
            
    }
    
// ---------------------------------------------------------------------------
// Update a currently active notifier with data aBuffer.
// ---------------------------------------------------------------------------
//
TPtrC8 CRsfwNotPluginDlg::UpdateL(const TDesC8& /*aBuffer*/)
    {
    return TPtrC8(NULL, 0);
    }


void CRsfwNotPluginDlg::DialogDismissedL( TInt aButtonId )
    {
    DEBUGSTRING(("CRsfwNotPluginDlg::DialogDismissedL"));
    iDialogDismissedCalled = ETrue;
    
    UnblockAppSwitching();
    
    if (aButtonId == EAknSoftkeyCancel) 
        {
        DEBUGSTRING(("Completing dialogrequest with KErrCancel"));
        iMessage.Complete(KErrCancel);
        }
    else if (aButtonId == EAknSoftkeyDone) 
        {
        DEBUGSTRING(("Completing dialogrequest with KErrNone"));
        iMessage.Complete(KErrNone);
        }
    else 
        {
        DEBUGSTRING16(("Completing dialogrequest with %d", aButtonId));
        iMessage.Complete(aButtonId);
        }
    }

// ---------------------------------------------------------------------------
// CRsfwNotPluginDlg::BlockAppSwitching
// Temporarily disables 'app key', so that user cannot switch or close
// the app when global dialog is being displayed
// ---------------------------------------------------------------------------
//
void CRsfwNotPluginDlg::BlockAppSwitching( )
    {
    if ( !iAppSwitchingBlocked )
        {
        ((CEikServAppUi*)(CEikonEnv::Static())->EikAppUi())
       	         ->SuppressAppSwitching(ETrue);
       	iAppSwitchingBlocked = ETrue;
        }
    }

// ---------------------------------------------------------------------------
// CRsfwNotPluginDlg::UnblockAppSwitching
// Enables 'app key' back
// ---------------------------------------------------------------------------
//
void CRsfwNotPluginDlg::UnblockAppSwitching( )
    {
    if ( iAppSwitchingBlocked )
        {
        ((CEikServAppUi*)(CEikonEnv::Static())->EikAppUi())
       	         ->SuppressAppSwitching(EFalse);
       	iAppSwitchingBlocked = EFalse;
        }
    }

// ---------------------------------------------------------------------------
// ECOM interface
// ---------------------------------------------------------------------------
//
const TImplementationProxy ImplementationTable[] =
	{

	IMPLEMENTATION_PROXY_ENTRY(0x101F9772,NotifierArray)

	};

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy) ;
	return ImplementationTable;
	}

