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
* Description:  Rsfw GS plugin Settins list container
*
*/


// INCLUDE FILES
#include    <akntitle.h>
#include    <barsread.h>
#include    <StringLoader.h>
#include    <AknQueryDialog.h>
#include    <aknnotewrappers.h>
#include 	<rsfwgspluginrsc.rsg>
#include    <uri16.h>

#include   	"rsfwgslocalviewids.h"
#include 	"rsfwgsplugin.h"
#include 	"rsfwgsremotedrivesettingscontainer.h"
#include 	"rsfwgspluginsettinglist.h"
#include  	"rsfwgssettingsdata.h"
#include	"rsfwmountman.h"
#include	"rsfwmountentry.h"
#include    "rsfwmountutils.h"
#include	"rsfwcommon.h"
#include    "mdebug.h"


// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::CRsfwGsPluginDriveSettingsContainer()
// ---------------------------------------------------------------------------
//
CRsfwGsPluginDriveSettingsContainer::CRsfwGsPluginDriveSettingsContainer(CAknView* aView) : iView(aView)
    {
    
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::ConstructL(const TRect& aRect)
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveSettingsContainer::ConstructL( const TRect& aRect, 
													CRsfwMountMan* aMountMan)
    {
    #ifdef _DEBUG
    RDebug::Print( _L( "[CRsfwGsPluginDriveSettingsContainer] ConstructL()" ) );
	#endif
	
	iMountMan = aMountMan;

    CEikStatusPane* sp = static_cast<CAknAppUi*> 
        ( iEikonEnv->EikAppUi() )->StatusPane();
    CAknTitlePane* title = static_cast<CAknTitlePane*> 
        ( sp->ControlL( TUid::Uid( EEikStatusPaneUidTitle ) ) );

    // Set the proper title of this list
    TResourceReader rReader;
    iCoeEnv->CreateResourceReaderLC( rReader, R_GS_RSFW_REMOTE_DRIVE_SETTINGS_VIEW_TITLE );
    title->SetFromResourceL( rReader );
    CleanupStack::PopAndDestroy(); //rReader

    CreateWindowL(); // Makes the control a window-owning control

    // construct the data object the settings list will use
    iData = CRsfwGsSettingsData::NewL();
    // construct control and set parent
    iSettingList = CRsfwGsPluginSettingsList::NewL(*iData);
    iSettingList->SetContainerWindowL(*this);

    // CreateResourceReaderLC will allocate a buffer to be used by
    // the TResourceReader. This buffer is pushed onto the cleanup
    // stack - not the TResourceReader itself
    iEikonEnv->CreateResourceReaderLC(rReader, R_SETTINGS);
    iSettingList->ConstructFromResourceL(rReader);

    // Clean up the buffer allocated above, NOT the reader itself.
    // Cannot use expected item overload of PopAndDestroy() as buffer 
    // is inaccessible. 
    CleanupStack::PopAndDestroy();
    iSettingList->ActivateL();
    
    iCoeEnv->AddForegroundObserverL( *this );

    SetRect( aRect );
    ActivateL();
    
       
    #ifdef _DEBUG        
    RDebug::Print( _L( "[CRsfwGsPluginDriveSettingsContainer] Construct done" ) );
	#endif
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::~CRsfwGsPluginDriveSettingsContainer()
// ---------------------------------------------------------------------------
//
CRsfwGsPluginDriveSettingsContainer::~CRsfwGsPluginDriveSettingsContainer()
    {    
    iCoeEnv->RemoveForegroundObserver( *this );
    if (iData)
        delete iData;
    if(iSettingList)  // if setting list has been created
        {
        delete iSettingList;
        iSettingList = NULL;
        }

   // Ecom session is used to make sure that user gives address
    // that corresponds to some installed access plugin
    // (currently via mountils::isaddressvalid)
    // this closes the session (if not in use anymore)
    REComSession::FinalClose();  
    }


// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::SizeChanged()
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveSettingsContainer::SizeChanged()
    {
	iSettingList->SetRect(Rect());
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::CountComponentControls() const
// ---------------------------------------------------------------------------
//
TInt CRsfwGsPluginDriveSettingsContainer::CountComponentControls() const
    {
    return 1;
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::ComponentControl( TInt aIndex ) const
// ---------------------------------------------------------------------------
//
CCoeControl* CRsfwGsPluginDriveSettingsContainer::ComponentControl( TInt aIndex ) const
    {
    switch ( aIndex )
        {
        case 0:
            return iSettingList;
        default:
            return NULL;
        }
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::EditCurrentItemL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveSettingsContainer::EditCurrentItemL()
    {
    iSettingList->EditCurrentItemL();    
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::OfferKeyEventL()
// ---------------------------------------------------------------------------
//
TKeyResponse CRsfwGsPluginDriveSettingsContainer::OfferKeyEventL( 
    const TKeyEvent& aKeyEvent, 
    TEventCode aType )
    {
    if (iSettingList)
        return iSettingList->OfferKeyEventL(aKeyEvent, aType);
    else
        return EKeyWasNotConsumed;
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::Draw(const TRect& aRect) const
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveSettingsContainer::Draw(const TRect& aRect) const
    {
    CWindowGc& gc = SystemGc();
    gc.Clear(aRect);
    }



// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::HandleListBoxEventL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveSettingsContainer::HandleListBoxEventL(CEikListBox* /*aListBox*/, TListBoxEvent aListBoxEvent)
    {
    // if the Select Key has been pressed
    if ((aListBoxEvent == MEikListBoxObserver::EEventEnterKeyPressed) ||
    (aListBoxEvent == MEikListBoxObserver::EEventItemClicked))
        {
        iSettingList->EditCurrentItemL();
        }
    }



// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::PrepareRemoteDriveForEditingL(TDesC& aRemoteDriveName)
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveSettingsContainer::PrepareRemoteDriveForEditingL(const TDesC& aRemoteDriveName)
    {
    DEBUGSTRING(("CRsfwGsPluginDriveSettingsContainer::PrepareRemoteDriveForEditingL enter"));
    const CRsfwMountEntry* mountEntry = NULL;
    
    // read the source setting
    DEBUGSTRING(("read the source setting"));
    mountEntry = iMountMan->MountEntryL(aRemoteDriveName); 
    User::LeaveIfNull((TAny *)mountEntry);
    
    // compulsory fields
    DEBUGSTRING(("compulsory fields"));
    iData->Reset();
    iData->iSettingName = *(mountEntry->Item(EMountEntryItemName));
    iData->iURL =*(mountEntry->Item(EMountEntryItemUri));
   	const HBufC* drive = (mountEntry->Item(EMountEntryItemDrive));
    if (drive && drive->Length())
        {
        iData->iDriveLetter =(*drive)[0];
        }
    // optional fields
    DEBUGSTRING(("optional fields"));
    if (mountEntry->Item(EMountEntryItemUserName))
    {
        DEBUGSTRING(("setting user id"));
    	iData->iUserID = *(mountEntry->Item(EMountEntryItemUserName));
    }
    
    if (mountEntry->Item(EMountEntryItemPassword)) 
    {
        DEBUGSTRING(("setting passwd"));
    	iData->iPassword = *(mountEntry->Item(EMountEntryItemPassword));
    }
    
    if (mountEntry->Item(EMountEntryItemIap))  
    {
        DEBUGSTRING16(("setting access pointDes to %S", (mountEntry->Item(EMountEntryItemIap))));
    	iData->iAccessPointDes = *(mountEntry->Item(EMountEntryItemIap));
    	// if the access point name returned from CenRep was '?', set it to "None" in UI
    	if (iData->iAccessPointDes == KIapAskUser) 
    	{
    		iData->iAccessPointName = KNullDesC;
    	}
    	else 
    	{
    		// try to resolve access point id into name
    		TLex lex(iData->iAccessPointDes);
    		TInt ap, err;
    		err = lex.Val(ap);
    		if (!err) 
    		{
    			iSettingList->GetAccessPointNameL(ap, iData->iAccessPointName);	
    			DEBUGSTRING(("setting access point"));
    			iData->iAccessPoint = ap;
    		}	
    		else
    		{
    			DEBUGSTRING(("setting access point name"));
    			iData->iAccessPointName = KNullDesC;
    		}
    	}
    }
       
    DEBUGSTRING(("iData constructed"));   
    iSettingList->LoadSettingsL();
    SetTitlePaneTextL(iData->iSettingName);
    iSettingList->ResetItemIndex();
    DEBUGSTRING(("CRsfwGsPluginDriveSettingsContainer::PrepareRemoteDriveForEditingL exit"));
    }
    
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::PrepareRemoteDriveNewDefaultL(TDesC& aRemoteDriveName);
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveSettingsContainer::PrepareRemoteDriveNewDefaultL(TDesC& aRemoteDriveName)
    {
    iData->Reset();
    iData->iSettingName = aRemoteDriveName;
    iSettingList->LoadSettingsL();
    iSettingList->ResetItemIndex();
    iSettingList->DrawDeferred();
    SetTitlePaneTextL(iData->iSettingName);
    }


// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::SetTitlePaneTextL( const TDesC& aTitleText ) const
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveSettingsContainer::SetTitlePaneTextL( const TDesC& aTitleText ) const
    {
    CAknTitlePane* title = static_cast< CAknTitlePane* >
        ( iEikonEnv->AppUiFactory()->StatusPane()->ControlL(
                                        TUid::Uid( EEikStatusPaneUidTitle ) ) );
    if ( !title )
        {
        User::Leave( KErrNotSupported );
        }
    title->SetTextL( aTitleText );
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::AreCompulsoryItemsFilled()
// ---------------------------------------------------------------------------
//
TBool CRsfwGsPluginDriveSettingsContainer::AreCompulsoryItemsFilled()
    {
    TUriParser uriParser;
    uriParser.Parse(iData->iURL);
    HBufC* host = HBufC::NewL(KMaxURLLength);
    TPtr hostPtr = host->Des();
    hostPtr = uriParser.Extract(EUriHost);

    if ( ((iData->iSettingName)!=KNullDesC) && (hostPtr!=KNullDesC))
        {
        delete host;
        return ETrue;
        }
    delete host;
    return EFalse;   
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::DisplayDeleteOrDontSaveDialog()
// ---------------------------------------------------------------------------
//
TBool CRsfwGsPluginDriveSettingsContainer::DisplayDeleteOrDontSaveDialogL()
    {
    
    HBufC* myDisplayMessage = NULL;
    myDisplayMessage = StringLoader::LoadLC(R_STR_REMOTE_DRIVE_CONF_COMPULSORY);
    CAknQueryDialog* query = CAknQueryDialog::NewL
                                        (CAknQueryDialog::EConfirmationTone);
    TBool answer = query->ExecuteLD( R_CONFIRMATION_QUERY, *myDisplayMessage );
    CleanupStack::PopAndDestroy(myDisplayMessage);        
    return answer;
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::DeleteremoteDriveIfExistL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveSettingsContainer::DeleteRemoteDriveIfExistL()
    {
    iMountMan->DeleteMountEntryL(iData->iDriveLetter);
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::IsExitProcessingOKL()
// ---------------------------------------------------------------------------
//
TBool CRsfwGsPluginDriveSettingsContainer::IsExitProcessingOKL()
    {
    if (AreCompulsoryItemsFilled())
        {
        if (!(RsfwMountUtils::IsFriendlyNameValid(iData->iSettingName))) 
            {
            HBufC* myDisplayMessage = NULL;
            myDisplayMessage = StringLoader::LoadLC(R_STR_RSFW_ERROR_ILLEGAL_CHARACTERS);
            CAknErrorNote* errorNote = new CAknErrorNote(EFalse);
            errorNote->ExecuteLD(*myDisplayMessage);
            CleanupStack::PopAndDestroy(myDisplayMessage);
            iSettingList->ListBox()->SetCurrentItemIndex(ESettingItemDriveName-1);
            return EFalse;            
            }
        
        // Uri must contain some scheme, like http:// or upnp://    
        if (!IsDriveAddressValidL(iData->iURL)) 
            {
            HBufC* myDisplayMessage = NULL;
            myDisplayMessage = StringLoader::LoadLC(R_STR_RSFW_ERROR_ILLEGAL_ADDRESS);
            CAknNoteDialog* noteDlg = new ( ELeave ) CAknNoteDialog( );
            noteDlg->SetTextL( *myDisplayMessage );
        	noteDlg->ExecuteLD( R_RSFW_INFORMATION_NOTE );	
            CleanupStack::PopAndDestroy(myDisplayMessage);
            iSettingList->ListBox()->SetCurrentItemIndex(ESettingItemURL-1);
            return EFalse;           
            }   
            
        // settings seem to be ok    
        return SaveOrCreateAndSaveRemoteDriveL(ETrue);        
        }
    else 
        {
        if (DisplayDeleteOrDontSaveDialogL())
            {
            DeleteRemoteDriveIfExistL();
            }
        else
            {
            // set focus on the address field as that is the first (and only)
            // unfilled compulsary setting field
             iSettingList->ListBox()->SetCurrentItemIndex(ESettingItemURL-1);
             return EFalse;
            }        
        }
    
    return ETrue;
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::SaveSettingsL
// Attempts to save settings
// ---------------------------------------------------------------------------
void CRsfwGsPluginDriveSettingsContainer::SaveSettingsL()
    {
    if (AreCompulsoryItemsFilled())
        {
        if (!(RsfwMountUtils::IsFriendlyNameValid(iData->iSettingName))) 
            {
            return;            
            }
        // Uri must contain some scheme, like http:// or upnp://    
        if (!IsDriveAddressValidL(iData->iURL)) 
            {
            return;
            }   
        // settings seem to be ok    
        SaveOrCreateAndSaveRemoteDriveL(EFalse);
        }
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::SaveOrCreateAndSaveremoteDriveL()
// ---------------------------------------------------------------------------
//
TBool CRsfwGsPluginDriveSettingsContainer::SaveOrCreateAndSaveRemoteDriveL(TBool aShowDialog)
    {
   // try to get mountentry with this drive letter
   const CRsfwMountEntry* existingMountEntry = iMountMan->MountEntryL(iData->iDriveLetter);
 
   // to keep names unique check if the setting with this name is already exist
   // ask user to change name
   TBool isConflict;
   if (existingMountEntry) 
        {
        isConflict = IsRemoteDriveNameConflictL(iData->iSettingName, *(existingMountEntry->Item(EMountEntryItemDrive)));
        }
   else 
        {
        TBuf<5> bogusDrive;
        bogusDrive.Append(_L("C:\\"));
        isConflict = IsRemoteDriveNameConflictL(iData->iSettingName, bogusDrive);
        }
    
    if (isConflict)
        {
        if (aShowDialog)
            {
            HBufC* myFormatMessage = NULL;	
            if ( iData->iSettingName.Length() )
                {
                myFormatMessage = StringLoader::LoadLC( R_STR_REMOTE_DRIVE_NAME_ALREADY_EXIST, iData->iSettingName);
                }
            else
                {
                myFormatMessage = StringLoader::LoadLC( R_STR_REMOTE_DRIVE_NAME_ALREADY_EXIST );
                }
     		
            CAknErrorNote* note = new CAknErrorNote(EFalse);
            note->SetTimeout( CAknNoteDialog::ELongTimeout  );
            note->ExecuteLD( *myFormatMessage );
            CleanupStack::PopAndDestroy(myFormatMessage);
            // set focus to drive
            iSettingList->ListBox()->SetCurrentItemIndex(ESettingItemDriveName-1); 		    
            }
        return EFalse;       	               
        }    
   	else if (!existingMountEntry || DataChanged(existingMountEntry)) 
        {  	
         
       CRsfwMountEntry* newMountEntry = CRsfwMountEntry::NewLC();
        if (existingMountEntry)
            {
        	// the same drive letter must be set as it is the key
        	newMountEntry->SetItemL(EMountEntryItemDrive, *(existingMountEntry->Item(EMountEntryItemDrive)));
            }
	
 	    newMountEntry->SetItemL(EMountEntryItemName, iData->iSettingName);
 	    newMountEntry->SetItemL(EMountEntryItemUri, iData->iURL);
 	    if ((iData->iUserID)!=KNullDesC)
   			{
      	  	newMountEntry->SetItemL(EMountEntryItemUserName, iData->iUserID);	
        	}
        
     	if ((iData->iPassword)!=KNullDesC)
    	    {
      		newMountEntry->SetItemL(EMountEntryItemPassword, iData->iPassword);	
       		}    
	    if ((iData->iAccessPointDes)!=KNullDesC) 
    	    {
	        newMountEntry->SetItemL(EMountEntryItemIap, iData->iAccessPointDes);
   		    }
   		    else
   		    {
   		    newMountEntry->SetItemL(EMountEntryItemIap, KIapAskUser);	
   		    }
        
        newMountEntry->SetItemL(EMountEntryItemInactivityTimeout, iData->iInActivityTimeout);
        
        // depending on whether this is new mount created or existing mount edited
        // choose proper method from MountMan API
        // pop here from the stack since ownership is passed to the mountman
        CleanupStack::Pop(newMountEntry);
        
        if (!existingMountEntry)
            {
           	iMountMan->AddMountEntryL(newMountEntry);
            }
        else 
            {
            iMountMan->EditMountEntryL(newMountEntry);
            }
        }
    return ETrue;   
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::DataChanged
// ---------------------------------------------------------------------------
//    
TBool CRsfwGsPluginDriveSettingsContainer::DataChanged(const CRsfwMountEntry* aCurrentData)
{
    if (!aCurrentData) 
        {
        return ETrue;
        }

	// compulsary settings
	if (iData->iSettingName != *(aCurrentData->Item(EMountEntryItemName))) 
	{
		return ETrue;
	}
	if (iData->iURL != *(aCurrentData->Item(EMountEntryItemUri))) 
	{
		return ETrue;
	}
	
	// optional settings
	if (!aCurrentData->Item(EMountEntryItemUserName)) 
	{
		if (iData->iUserID != KNullDesC) 
		{
			return ETrue;
		}
	}
	else 
	{
		if (iData->iUserID != *(aCurrentData->Item(EMountEntryItemUserName)))
		{
		return ETrue;
		}	
	}
	
	if (!aCurrentData->Item(EMountEntryItemPassword)) 
	{
		if (iData->iPassword != KNullDesC) 
		{
			return ETrue;
		}
	}
	else 
	{
		if (iData->iPassword != *(aCurrentData->Item(EMountEntryItemPassword)))
		{
			return ETrue;
		}	
	}

	if (!aCurrentData->Item(EMountEntryItemIap)) 
	{
		if (iData->iAccessPointDes != KNullDesC) 
		{
			return ETrue;
		}
	}
	else 
	{
		if (iData->iAccessPointDes != *(aCurrentData->Item(EMountEntryItemIap)))
		{
			return ETrue;
		}	
	}


	return EFalse;
}

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::IsRemoteDriveNameExistL(TDesC& aremoteDriveName)
// 
// ---------------------------------------------------------------------------
//
TBool CRsfwGsPluginDriveSettingsContainer::IsRemoteDriveNameConflictL(
                                               TDesC& aRemoteDriveName,
                                               const TDesC& aDriveLetter)
    {
    TBool ret(EFalse);
    CDesCArray* remoteDriveList = NULL;

    TRAPD(err, remoteDriveList = LoadRemoteDriveNamesL());
    if(err!=KErrNone)    
        {
        return EFalse;
        }
    CleanupStack::PushL(remoteDriveList);

    TInt remoteDriveListCount = remoteDriveList->Count();
    for (TInt i = 0; i< remoteDriveListCount; i++)
        {
        if (!((remoteDriveList->MdcaPoint(i)).CompareF(aRemoteDriveName))) // if equal
            { 
              // check whether the drive letter is the same or not
              const CRsfwMountEntry* existingDrive = 
                    iMountMan->MountEntryL(aRemoteDriveName);
              if ((existingDrive->Item(EMountEntryItemDrive))->Compare(aDriveLetter) == 0 ) 
                {
                ret = EFalse;    
                }
              else 
                {
                ret = ETrue;
                }
              break;
            }
        }
    CleanupStack::PopAndDestroy(remoteDriveList);
    return ret;
    
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::IsDriveAddressValidL
// ---------------------------------------------------------------------------
//
TBool CRsfwGsPluginDriveSettingsContainer::IsDriveAddressValidL(const TDesC& aDriveAddress)
    {
    HBufC8* urlBuffer = HBufC8::NewL(KMaxURLLength);
    TPtr8 bufPtr = urlBuffer->Des();
    bufPtr.Append(aDriveAddress);
    TBool isAddressValid = RsfwMountUtils::IsDriveAddressValid(*urlBuffer);
    delete urlBuffer;
    return isAddressValid;
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::LoadRemoteDriveNamesL
// ---------------------------------------------------------------------------
//
CDesCArray* CRsfwGsPluginDriveSettingsContainer::LoadRemoteDriveNamesL()
    {
    CDesCArray* myArray = new (ELeave) CDesC16ArraySeg(4);
    iMountMan->GetMountNamesL(myArray);
    return myArray;
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::HandleResourceChange()
// ---------------------------------------------------------------------------
void CRsfwGsPluginDriveSettingsContainer::HandleResourceChange( TInt aType )
    {
    if ( aType == KAknsMessageSkinChange ||
         aType == KEikDynamicLayoutVariantSwitch )
        {
        if (aType != KAknsMessageSkinChange) 
            {
            TRect mainPaneRect;
            AknLayoutUtils::LayoutMetricsRect( AknLayoutUtils::EMainPane,
                                           mainPaneRect);
            SetRect( mainPaneRect );
            }
            
        DrawDeferred();    
        CRsfwGsPlugin* tempView = static_cast<CRsfwGsPlugin*> (iView);  
        tempView->HandleResourceChangeManual(aType);
        }
    CCoeControl::HandleResourceChange( aType );
    }


// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::HandleResourceChangeManual()
// ---------------------------------------------------------------------------    
void CRsfwGsPluginDriveSettingsContainer::HandleResourceChangeManual(TInt aType)
    {
    if (aType !=  KAknsMessageSkinChange) 
        {    
        TRect mainPaneRect;
        AknLayoutUtils::LayoutMetricsRect( AknLayoutUtils::EMainPane,
                                       mainPaneRect);
        SetRect( mainPaneRect );  
        } 
    
    DrawDeferred();   	
	iSettingList->HandleResourceChange(aType);
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::FocusChanged
// Set focus on the selected listbox. For animated skins feature.
// ---------------------------------------------------------------------------
void CRsfwGsPluginDriveSettingsContainer::FocusChanged(TDrawNow /*aDrawNow*/)
    {
    if(iSettingList)
        {
        iSettingList->SetFocus( IsFocused() );
        }
    }

// ---------------------------------------------------------
// CRsfwGsPluginDriveSettingsContainer::GetHelpContext
// This function is called when Help application is launched
// ---------------------------------------------------------
//
void CRsfwGsPluginDriveSettingsContainer::GetHelpContext( TCoeHelpContext& aContext ) const
    {
    aContext.iMajor = KGSRsfwPluginUID;
   // aContext.iContext = KRSFW_HLP_EDIT_SET;
    }   
    
    
 TDes& CRsfwGsPluginDriveSettingsContainer::GetCurrentRemoteDriveName()
    {
    return iData->iSettingName;    
    }   
    
 
// from  MCoeForegroundObserver
 
void CRsfwGsPluginDriveSettingsContainer::HandleGainingForeground() 
    {
    
    }  
    
    
void CRsfwGsPluginDriveSettingsContainer::HandleLosingForeground() 
    {
    
    }   
    

// End of File
