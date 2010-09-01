/*
* Copyright (c) 2002-2005 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  The control container (view) of the settings list view
*
*/


#ifndef CRSFWGSPLUGINDRIVESETTINGSCONTAINER_H
#define CRSFWGSPLUGINDRIVESETTINGSCONTAINER_H

// INCLUDES
#include    <akncheckboxsettingpage.h>
//#include    "rsfwgsplugin.h"


// FORWARD DECLARATIONS
//class CGSListBoxItemTextArray;
class CRsfwGsPluginSettingsList;
class CRsfwGsSettingsData;
class CRsfwMountMan;
class CAknView;
class CRsfwMountEntry;


// CLASS DECLARATION

/**
*  CRsfwGsPluginDriveSettingsContainer container class
*  container class for Remote drive settings view
*
*  @lib rsfwgsplugin.dll
*  @since Series 60 3.1
*/
class CRsfwGsPluginDriveSettingsContainer : public CCoeControl, 
                                            MEikListBoxObserver,
                                            MCoeForegroundObserver
                                            
    {
    public: // Constructors and destructor

         /**
        * Constructor.
        */
        CRsfwGsPluginDriveSettingsContainer(CAknView* aView);


        /**
        * By default Symbian 2nd phase constructor is private.
        * @param aRect gives the correct TRect for construction
        * @param aModel for the model
        */
        void ConstructL( const TRect& aRect, 
                         CRsfwMountMan* aMountMan );


        /**
        * Destructor.
        */
        ~CRsfwGsPluginDriveSettingsContainer();

    public: // From CCoeControl
    
        /**
        * See CCoeControl.
        */
        TInt CountComponentControls() const;
        
        /**
        * See CCoeControl.
        */
        CCoeControl* ComponentControl( TInt aIndex ) const;
        
        /**
        * See CCoeControl.
        */
        TKeyResponse OfferKeyEventL( 
            const TKeyEvent& aKeyEvent, TEventCode aType );
            
        /**
        * See CCoeControl.
        */
        void SizeChanged();
        
        /**
        * See CCoeControl.
        */
        void Draw(const TRect& aRect) const;
       
        /**
        * See CCoeControl
        */
        void HandleResourceChange( TInt aType );

        /**
         * Gets help context
         */
        void GetHelpContext( TCoeHelpContext& aContext ) const;

        /**
         * See CCoeControl
         */
        void FocusChanged(TDrawNow aDrawNow);
        
    public: // MCoeForegroundObserver

        /** Handles the drive list view coming to the foreground. */
        void HandleGainingForeground();
        /** Handles the drive list view going into the background. */
        void HandleLosingForeground();
     
        
    public:
    
        /**
        * Edit currently focused item on settings list. called from UI
        */
        void EditCurrentItemL();
        
        /**
        * See MEikListBoxObserver
        */
        void HandleListBoxEventL(CEikListBox* aListBox, TListBoxEvent aEventType);
        
        /**
        * Prepare a Remote drive with given name for editing
        * @param aRemoteDriveName reference to the remote drive to be edited
        */
        void PrepareRemoteDriveForEditingL(const TDesC& aRemoteDriveName);
        
        /**
        * Prepare the a new remote drive with default values with given name
        * @param aRemoteDrive name reference to the new remote drive name
        */
        void PrepareRemoteDriveNewDefaultL(TDesC& aRemoteDriveName);
        
              
        /**
        * Called by the UI when back button is pressed, to perform needed steps
        * @return ETrue if settings list is allowed to close
        */
        TBool IsExitProcessingOKL(); 
        
        /**
        * Save current drive settings
        */        
        void SaveSettingsL();
        
        /*
        * Return the name of current set in iData
        * @return TDes reference to current set
        */
        TDes& GetCurrentRemoteDriveName();

      
        void HandleResourceChangeManual(TInt aType);
      
    private: // Most of these methods perform operations using iData
    
        /**
        * Sets the title pane text with given discriptor
        * @param aTitleText text to be shown on title pane
        */
        void SetTitlePaneTextL( const TDesC& aTitleText ) const;
        
        /**
        * Finds out whether compulsory items are filled
        * @return ETrue if compulsory items are filled
        */
        TBool AreCompulsoryItemsFilled();
        
        /**
        * Display a query dialog to user that compulsory settings are not
        * filled, and if user wants to delete the settings
        * @return ETrue if user wants to delete the settings
        */
        TBool DisplayDeleteOrDontSaveDialogL();
        
        /**
        * Update a remote drive if it already exist or create it if it doesnt exist
        * @param aShowDialog if false then possible error notes won't be shown
        * @return ETrue if save procedure goes ok
        */
        TBool SaveOrCreateAndSaveRemoteDriveL(TBool aShowDialog);
        
        /**
        * Deletes the remote drive if it exist in the Central Repository
        */
        void DeleteRemoteDriveIfExistL();
        
        /**
        * Load Remote drive names with trap, useful when list doesnt have anything
        * @param Reference to setting ids
        * @return CDesCArray remote drives
        */
      	CDesCArray* LoadRemoteDriveNamesL();  
   
        /**
        * See if the remote drive setting under edit was changed
        * @param Reference to the current mount conf 
        * @return ETrue if something has been changed
        */   	
      	TBool DataChanged(const CRsfwMountEntry* aCurrentData);   
      	
      	/**
        * Check whether some other drive (some other drive letter) already uses this name
        * @param aRemoteDriveNamereference the remote drive name
        * @param aDriveLetter the drive letter
        * @return ETrue if remote drive exist with given name
        */
        TBool IsRemoteDriveNameConflictL(TDesC& aRemoteDriveName, const TDesC& aDriveLetter);

        /**
        * Returns ETrue if the address (URL) for a remote drive is valid
        * Calls RsfwMountUtils API
        * @since S60 3.2
        * @param aFriendlyName remote drive friendly name
        */
        TBool IsDriveAddressValidL(const TDesC& aDriveAddress);
            
    private: // data
        
        // CRsfwGsPluginSettingsList owned
      	CRsfwGsPluginSettingsList* iSettingList;        
  
      	// Pointer to settings data owned
      	CRsfwGsSettingsData* iData;
      	
      	  // Pointer to the application view, not owned
     	 CAknView* iView; // not owned
      	    	
    	// model, not owned	
      	CRsfwMountMan*           iMountMan; 
    };


#endif  // CRSFWGSPLUGINDRIVESETTINGSCONTAINER_H

// End of File
