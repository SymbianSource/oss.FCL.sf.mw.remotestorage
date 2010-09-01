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

#ifndef C_RSFWNOTPLUGIN_H
#define C_RSFWNOTPLUGIN_H

#include <e32def.h>
#include <e32base.h>
#include <eiknotapi.h> // MEikSrvNotifierBase
#include <MAknMemorySelectionObserver.h>
#include "rsfwauthenticationdlgrequest.h"
#include "rsfwsavetodlgrequest.h"
#include "rsfwnotpluginrequest.h"

class CAknDialog;


IMPORT_C CArrayPtr<MEikSrvNotifierBase2>* NotifierArray();

/**
 *  Remote Storage FW plugin to the notifier server.
 *
 *  This plugin implements the custom global dialogs needed by 
 *  Remote Storage Framework.
 *
 *  @lib rsfwnotplugin.dll
 *  @since S60 v3.2
 */
class CRsfwNotPluginDlg : public CActive, 
                          public MEikSrvNotifierBase2, 
                          public MAknMemorySelectionObserver,
                          public MProgressDialogCallback
    {
public: 

    	static CRsfwNotPluginDlg* NewL();

    	~CRsfwNotPluginDlg();

		// for base class CActive

        /**
         * From CActive
         * Handles an active object's request completion event.
         *
         * @since S60 v3.2
         */
	    void RunL();
	    
        /**
         * From CActive
         * Handles a leave occurring in the request completion event handler RunL().
         *
         * @since S60 v3.2
         */
    	TInt RunError(TInt aError);
    	
        /**
         * From CActive
         * Implements cancellation of an outstanding request.
         *
         * @since S60 v3.2
         */    	
         void DoCancel();

        // from base class MEikSrvNotifierBase2

       /**
        * From .MEikSrvNotifierBase2
        * Called when all resources allocated by notifiers should be freed.
        *
        * @since S60 v3.2
        * 
        */
    	virtual void Release();
    
    
        /**
        * From MEikSrvNotifierBase2
        * Called when a notifier is first loaded to allow any initial construction that is required.
        *
        * @since S60 v3.2
        * 
        */
        virtual MEikSrvNotifierBase2::TNotifierInfo RegisterL();
    	
    
        /**
        * From MEikSrvNotifierBase2
        * Return the priority a notifier takes and the channels it acts on.  The return value may be varied
     	* at run-time.
        *
        * @since S60 v3.2
     	*/
		virtual MEikSrvNotifierBase2::TNotifierInfo Info() const;
		
    
        /**
        * From MEikSrvNotifierBase2
        * Start the notifier with data aBuffer and return an initial response.
        *
        * @since S60 v3.2
        * @param aBuffer Data that can be passed from the client-side. The format and 
	    *                meaning of any data is implementation dependent.
     	*/
    	virtual TPtrC8 StartL(const TDesC8& aBuffer);
    
    
        /**
        * From MEikSrvNotifierBase2
     	* Start the notifier with data aBuffer.  aMessage should be completed when the notifier is deactivated.
     	* May be called multiple times if more than one client starts the notifier.  The notifier is immediately
     	* responsible for completing aMessage.
        *
        * @since S60 v3.2
        * @param aBuffer Data that can be passed from the client-side. The format and 
    	*                meaning of any data is implementation dependent.
	    * @param aReplySlot Identifies which message argument to use for the reply.
        *                   This message argument will refer to a modifiable descriptor, a TDes8 type, 
	    *                   into which data can be returned. The format and meaning of any returned data 
	    *                   is implementation dependent.
	    @param aMessage Encapsulates a client request
        */
		virtual void StartL(const TDesC8& aBuffer, TInt aReplySlot, const RMessagePtr2& aMessage);
		
		
        /**
        * From MEikSrvNotifierBase2
        * The notifier has been deactivated so resources can be freed and outstanding messages completed.
        *
        * @since S60 v3.2
     	*/
    	virtual void Cancel();


        /**
        * From MEikSrvNotifierBase2
        * Update a currently active notifier with data aBuffer.
        *
        * @since S60 v3.2
        * @param aBuffer Data that can be passed from the client-side. The format and 
	    *                meaning of any data is implementation dependent.
        * @return A pointer descriptor representing data that may be returned. The format 
	    *         and meaning of any data is implementation dependent.
     	*/
    	virtual TPtrC8 UpdateL(const TDesC8& aBuffer);
    	
        // from base class MAknMemorySelectionObserver
     
        /**
         * From MAknMemorySelectionObserver
         * Logic to decide whether the inputs given in the dialog are ok
         *
         * @since S60 v3.2
         * @param aMemory the selected memory
        */
    	TBool OkToExitL( CAknMemorySelectionDialog::TMemory aMemory );
    	   
    	// from base class MProgressDialogCallback	
    	
    	 /**
         * From MProgressDialogCallback
         * Callback we receive when a non-modal wait dialog is dismissed
         *
         * @since S60 v3.2
         * @param aButtonId the button user pressed
        */
        void DialogDismissedL( TInt aButtonId );
        
	private: 
		
		  /**
        * C++ default constructor.
        */
    	CRsfwNotPluginDlg();
    	
    	/**
        * By default Symbian 2nd phase constructor is private.
        */
    	void ConstructL();

		// New functions

 		// Helpers
	    void HandleAsyncRequestL();
	    
	    // show dialogs
	    TInt ShowAuthenticationDialogL();			       
        TBool ShowUnavailableRetryNoteL();						       
        TBool ShowSaveToDlgL();	
        void ShowWaitNoteL();	
    					    
    	/**
        * Returns the type of given item index in CFileManagerItemProperties bitmask
        * @param aFullPath	Full path to item which type is needed..
        * @return CFileManagerItemProperties bitmask
        */
		TUint32 FileTypeL( const TDesC& aFullPath ) const;				    
        
        void Cleanup();
        
        TBool GetValidNameL(TDesC& aPath, TDes& aName);

        void ShowDiskFullNoteL( TBool aInternal );
        
        void CancelL();

    	/**
        * Temporarily disables 'app key', so that user cannot switch or close
        * the app when global dialog is being displayed
        */
        void BlockAppSwitching();
        void UnblockAppSwitching();

	private: // Data
	    TInt iMethod; // See TRsfwNotPluginRequest::TRsfwNotPluginMethod
        RMessagePtr2 	iMessage;
    	TInt 			iReplySlot;
    	MEikSrvNotifierBase2::TNotifierInfo  iInfo;
		
        // for all dialogs
        TBuf<KRsfwMaxDrivenameLength>  iDriveName;
        // for authentication dialog
        HBufC* iUserName;
        HBufC* iPassword;
    	
    	// param structs
    	TRsfwAuthParamsPckg* iAuthRequest;
    	TRsfwSaveToParamsPckg* iSaveToRequest;
    	
		// Valid during showing a dialog
    	CAknDialog* iDialog; 
    	CAknWaitDialog* iWaitDialog;
    	TBool iCancelled;
    	
    	// for save as dialog
    	HBufC*  iFileName;
    	TInt iFileSize;
    	TBuf<KRsfwMaxDriveletterLength> iCacheDrive;
    	CAknMemorySelectionDialog* iMemDialog;
    	HBufC* iCurrentRootPath;
    	RFs iFs;
    	
    	// 
    	TBool iDialogDismissedCalled;
    	TBool iAppSwitchingBlocked;  // is 'app key' disabled
   };

#endif