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
* Description:  File name query dialog for "save as" dialog
*
*/


#ifndef C_RSFWNOTPLUGINNAMEDIALOG_H
#define C_RSFWNOTPLUGINNAMEDIALOG_H

#include <AknQueryDialog.h>

// CONSTANTS
_LIT(KDot, ".");
_LIT(KNotPluginResourcePath, "rsfwnotplugindlg.rsc");

// CLASS DECLARATION

/**
 * Dialog for "rename file" query in the Notifier plugin
 * This dialog is used to get a new name for a file from the user.
 * It only returns when user has entered a name that is legal in 
 * Symbian file system
 */
class CRsfwNotPluginNameDialog : public CAknTextQueryDialog
    {
    public:  // Constructors and destructor

        /**
         * Two-phased constructor.
         * @param aOldName Old name of the file, this will be the default name
         *                 This contains the path, but the path is not shown to the user
         * @param aNewName On return, the user entered new name of the file.
         *                 Does not contain the path, as it is assumed to remain the same
         * @return Newly created query dialog.
         */
        IMPORT_C static CRsfwNotPluginNameDialog* NewL( const TDesC& aOldName, 
											   TDes& aNewName,
										    	RFs& aFs);

		/**
        * Destructor.
        */
        ~CRsfwNotPluginNameDialog();

    protected: // from CAknTextQueryDialog
        /**
         * @see CAknTextQueryDialog
         */
    	TBool OkToExitL( TInt aButtonId );

	private:
        /**
        * C++ default constructor.
        */
        CRsfwNotPluginNameDialog( TDes& aNewName, RFs& aFs  );

        /**
         * Symbian OS 2nd phase constructor.
         * @param aOldName Old name of the file, this will be the default name
         */
		void ConstructL( const TDesC& aOldName );
	
            
        /**
        * Show simple error note if something is wrong with the name the user chose
        * @param aTextId localized string
        */   
        void ShowSimpleInfoNoteL(
            const TInt aTextId);

    	

    private:    // Data
        /// Own: Old file name
        HBufC* iOldName;
        RFs& iFs;
    };

#endif
