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
* Description:  Rsfw GS plugin, UI(CAknView) interface header
*
*/



#ifndef CRSFWGSPLUGIN_H
#define CRSFWGSPLUGIN_H

// INCLUDES
#include <ConeResLoader.h>
#include <gsplugininterface.h>
#include <aknnotewrappers.h>
#include "rsfwgsplugin.hrh"
#include "rsfwgsremotedrivesend.h"

// CONSTANTS

// the drive letter is required in these
// resource file
_LIT( KRsfwGsPluginResourceFileName, "Z:\\resource\\rsfwgspluginrsc.rsc" );
// icons
_LIT(KGSPluginBitmapFile,       "Z:\\resource\\apps\\rsfwgsplugin.mif");

// FORWARD DECLARATIONS
class CRsfwGsPluginDriveListContainer;
class CRsfwGsPluginDriveSettingsContainer;
class CRsfwMountMan;

/**
*  CRsfwGsPlugin view class (CAknView).
*
* This is RSFW GS plugin.
*/
class CRsfwGsPlugin : public CGSPluginInterface
    {
    
    public: // Constructors and destructor
        
        /**
        * Symbian OS two-phased constructor
        * @return 
        */
        static CRsfwGsPlugin* NewL( TAny* aInitParams );
    
        /**
        * Destructor.
        */
        ~CRsfwGsPlugin();
        
        /**
        * Load the SettingsView with specified mount entry and type of loading
        * The types are EEditExisting, ENewDefault, ENewFromExisting
        */
        void LoadSettingsViewL(TRsfwSettingsViewType aType, TDesC& aMountName, TBool aAddToViewStack);

        /**
        * Loads the Main View where list of remote drives is visible
        */
        void LoadMainViewL();
        
		/**
        * Help launcher method.
        * @param aContext The help context
        */
        void LaunchHelpL( const TDesC& aContext );
        
    public: // From CAknView
        
        /**
        * This function is used for identifying the plugin
        */
        TUid Id() const;
        
        /**
        * See CAknView
        */
        void HandleViewRectChange();

        /**
        * See CAknView
        */
        void DoActivateL( const TVwsViewId& aPrevViewId,
                          TUid aCustomMessageId,
                          const TDesC8& aCustomMessage );
        /**
        * See CAknView
        */
        void DoDeactivate();

        /**
        * See CAknView
        */
        void HandleCommandL( TInt aCommand );
        
  public:
        
        /**
        * Is the current remote drive connected?
        * Fetches the information from RFE
        */
        TBool IsDriveConnectedL();
        
        /**
        * Update softkeys
        */
        void UpdateCbaL();
        
        /**
        * This function deletes the current remote drive.
        */
        void DeleteRemoteDriveL();
        
        /**
        * Return the currently active container
        */   
        CCoeControl* CurrentContainer();
        
        /**
        * Process the existed deleting dialog
        */   
        void ProcessDeletingDialog();

    protected:

        /**
        * C++ default constructor.
        */
        CRsfwGsPlugin( );

        /**
        * Symbian OS default constructor.
        */
        void ConstructL();
        
    private:
    
        /**
        * See base classes
        */
        void DynInitMenuPaneL(TInt aResourceId,CEikMenuPane* aMenuPane);
        
       
        /**
        * Connect he current remote drive
        */
        void ConnectRemoteDriveL();

        /**
        * Disconnect he current remote drive
        */
        void DisconnectRemoteDriveL();
        
        
        /**
        * Sends a link to the current remote drive as a smart message
        */
        void SendLinkL();
        
        
        /**
        * Process "Send As"
        */
        void DoSendAsL();
        
 
    public: // From CGSPluginInterface
    
        /**
        * See CGSPluginInterface
        */
        void GetCaptionL( TDes& aCaption ) const;
        
        /**
        * See CGSPluginInterface
        */
        CGulIcon* CreateIconL( const TUid aIconType );
    
    public:    
        
        void HandleResourceChangeManual(TInt aType);
        
    protected: //Data        
        
        // Reference to application UI - not owned.
        CAknViewAppUi* iAppUi;
        
        // RConeResourceLoader
        RConeResourceLoader iResources;
        
        // Previous View ID
        TVwsViewId iPrevViewId; // Previous view.
        
    private:
    
        // Pointer to the main list container - owned
        CRsfwGsPluginDriveListContainer* iMainListContainer;
        
        // Pointer to the setting list container - owned.
        CRsfwGsPluginDriveSettingsContainer* iSettingListContainer;
        
        // Pointer to the model - owned
        CRsfwMountMan*  iMountMan;
        
        // Pointer to the current container - not owned.
        CCoeControl* iCurrentContainer;   
        
        // for sending remote drive links - owned
        CRsfwGsRemoteDriveSend*	iSender;
        
        // Pointer to the possible deleting dialog
        CAknQueryDialog* iDialog;
        
        // The deleting id of remote drive
        TChar iDeletingId;
    };
  


#endif // CRSFWGSPLUGIN_H

// End of File
