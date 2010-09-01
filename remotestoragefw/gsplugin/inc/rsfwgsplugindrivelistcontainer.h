/*
* Copyright (c) 2003 - 2005 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  The control container (view) of the remote drives list view
*
*/


#ifndef CRSFWGSPLUGINDRIVELISTCONTAINER_H
#define CRSFWGSPLUGINDRIVELISTCONTAINER_H

// INCLUDES
// System includes
#include <eikclb.h>
#include <akntitle.h>

#include "rsfwgsplugin.hrh"
#include "rsfwgspropertywatch.h"

// FORWARD DECLARATIONS

class CRsfwGsPlugin;
class CRsfwMountMan;
class CEikButtonGroupContainer;
class CAknColumnListBox;

// CLASS DECLARATION

/**
*  CRsfwGsPluginDriveListContainer container class
*  @since 3.1
*  container class for Remote Drives view
*/
class CRsfwGsPluginDriveListContainer : public CCoeControl, 
                                               MEikListBoxObserver,
                                               MCoeForegroundObserver
    {
    public:  // Constructors and destructor
    
    	/**
        * Constructor
        */
        CRsfwGsPluginDriveListContainer(CRsfwGsPlugin* aView);
        
        
         /**
        * Symbian OS default constructor.
        *
        * @param aRect gives the correct TRect for construction.
        */
        void ConstructL( const TRect& aRect, CRsfwMountMan* aMountMan );

        /**
        * Destructor.
        */
        ~CRsfwGsPluginDriveListContainer();    
 
  public: // From CCoeControl
        
        /**
        * See CCoeControl
        */
        TInt CountComponentControls() const;
        
        /**
        * See CCoeControl
        */
        CCoeControl* ComponentControl( TInt aIndex ) const;

        /**
        * See CCoeControl
        */
        TKeyResponse OfferKeyEventL( 
            const TKeyEvent& aKeyEvent, TEventCode aType );

        /**
        * See CCoeControl
        */
        void SizeChanged();         

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

        
 public: // own methods
        
        /**
        * Invokes editing on current item, in response to UI Edit command
        */
        void EditCurrentItemL();
        
        /**
        * See MEikListBoxObserver
        */
        void HandleListBoxEventL(CEikListBox* aListBox, TListBoxEvent aEventType);
        
        /**
        * Finds whether main list is empty
        * @return ETrue if main list is empty.
        */
        TBool IsListEmpty();
        
        /**
        * Finds whether the current item is the last item in main list
        * @return ETrue if current item is last in main list.
        */
        TBool IsLastItem();
        
        /**
        * Delete the currently focused drive. Also deletes it from CentRep
        */
        void DeleteCurrentRemoteDriveL();
        
        /**
        * Is drive connected?
        * Fetches the information from RFE
        * @param the Friendly name of the drive
        */
        TBool IsDriveConnectedL(const TDesC& aName);
        
        /**
        * Sets drive in the list to 'connected' or 'disconnected'
        * (modifies the icon)
        * @param the Friendly name of the drive
        */
        void SetDriveConnectedStateL(const TDesC& aName, TBool aConnected);
        
        /**
        * Connects the currently chose remote drive
        */
        void ConnectCurrentRemoteDriveL();
                 
        /**
        * Disconnects the remote drive
        */
        void DisconnectCurrentRemoteDriveL();
        
        /**
        * Loads remote drives from Central Repository
        * @return TInt the number of remote drives
        */
        TInt LoadRemoteDriveListArrayL();
        
        /**
        * Suggests next new remote drive name, this usually called by UI when
        * user create a new remote drive.
        * @return TDesC& new remote drive name
        */
        TDesC& GetNextRemoteDriveNameL();
        
        /**
        * Get the name of currently focused drive. Usually called by UI.
        * The pointer to buffer remains on heap, which is deleted by caller.
        * @return HBufC* pointer to currently focused remote drive name
        */
        HBufC* GetCurrentRemoteDriveNameLC();
        
         /**
        * Get the name of remote drive at index aIndex. Usually called by UI
        * @return TPtrC16 Pointer to a buffer owned by the list array
        */
        TPtrC GetRemoteDriveNameL(TInt aIndex);
        
         /**
        * Sets the model
        */   
        void SetModel(CRsfwMountMan* iMountMan);
      
         /**
        * Returns the number of remote drives in the list
        */  		
		TInt RemoteDriveCount();
		
        /**
        * Sets the focus
        */
        void SetFocus();

        /**
        * Sets the focus to given drive
        */
        void SetFocusL(const TDes& aDriveName);       

        /**
        * Get the ID of the current remote drive. 
        * ID is the drive letter under which the drive is mounted
        * @return TInt ID of the current remote drive
        */
       TChar GetCurrentRemoteDriveIdL();

		
		void HandleResourceChangeManual(TInt aType);

    private: // own methods
    
        /**
        * Perform the initial setup of the main list. Called by Constructor
        */
      void SetupListL();
          
      
        /**
        * Get Remote drive names 
        * @return CDesCArray remote drives
        */
      CDesCArray* GetRemoteDriveNamesL();

        /**
        * Handles the drive list view coming to the foreground.
        * Leaving variant
        */
      void HandleGainingForegroundL();

    public:  // data
      
      // model, not owned	
      CRsfwMountMan*           iMountMan;
      
           // for setting focus
      TInt iCurrentItemIndex;
    
    private: // data
    
    // Pointer to the main list, owned
      CAknColumnListBox* iMainList;

      // Pointer to the application view, not owned
      CRsfwGsPlugin* iView; // not owned
      
      // The remote drive names list array, not owned
      CDesCArray* iRemoteDriveListArray;
      
      // Buffer for holding remote drive setting name
      TBuf<KMaxFriendlyNameLength>  iSettingNewName;
      
      // Title for menu pane
      TBuf<KMaxMenuPaneTitleLength>  iTitle;
      
      // Pointer to title pane, not owned
      CAknTitlePane* iTitlePane;
      
      // P&S notifier about changes in drive connection state
      CRsfwGsPropertyWatch* iDriveConnectObserver;
      
 
    
    };

#endif //CRSFWGSPLUGINDRIVELISTCONTAINER_H
            
// End of File
