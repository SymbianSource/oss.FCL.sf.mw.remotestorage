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


// INCLUDE FILES
#include    <aknappui.h> 
#include    <eikspane.h>
#include    <StringLoader.h>
#include    <aknlists.h>
#include 	<AknIconArray.h> 
#include	<eikclbd.h>
#include    <rsfwgspluginrsc.rsg>
#include 	<avkon.mbg>
#include	<AknQueryDialog.h>
#include 	<rsfwgsplugin.mbg>
#include    <aknnotewrappers.h>

#include   	"rsfwgslocalviewids.h"
#include    "rsfwgsplugindrivelistcontainer.h"
#include    "rsfwgsplugin.h"
#include	"rsfwmountman.h"
#include    "rsfwcontrol.h"
#include	"rsfwmountentry.h"

 _LIT(KTabulator, "\t");
 _LIT(KConnectedIcon, "\t0");

// ============================ MEMBER FUNCTIONS ===============================

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::CRsfwGsPluginDriveListContainer()
// ---------------------------------------------------------------------------
//
CRsfwGsPluginDriveListContainer::CRsfwGsPluginDriveListContainer(CRsfwGsPlugin* aView) : iView(aView)
    {
    
    }
    

// -----------------------------------------------------------------------------
// GSSettListRemoteDrivesListContainer::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::ConstructL(
    const TRect& aRect,
   	CRsfwMountMan* aMountMan)
    {
	iMountMan = aMountMan;

    CEikStatusPane* sp = static_cast<CAknAppUi*> 
        ( iEikonEnv->EikAppUi() )->StatusPane();
    iTitlePane = static_cast<CAknTitlePane*> 
        ( sp->ControlL( TUid::Uid( EEikStatusPaneUidTitle ) ) );

    // Set title
    StringLoader::Load ( iTitle, R_STR_REMOTE_DRIVES_VIEW_TITLE );
    iTitlePane->SetTextL(iTitle);// FromResourceL( rReader );
    CreateWindowL(); // Makes the control a window-owning control

    // Main List creation and initialization
    iMainList = new(ELeave) CAknSingleStyleListBox();
    iMainList->SetContainerWindowL(*this);
    iMainList->ConstructL(this, EAknListBoxLoopScrolling);

    // Main list scroll bar issues
    iMainList->CreateScrollBarFrameL(EFalse);
    iMainList->ScrollBarFrame()->SetScrollBarVisibilityL(
    CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);

	 // Empty text processing
	 _LIT (KStringHeader, "%S\n%S");
    HBufC* emptyText = iEikonEnv->AllocReadResourceLC(R_GS_RSFW_NO_REMOTE_DRIVES);  
    HBufC* emptyText2 = iEikonEnv->AllocReadResourceLC(R_GS_RSFW_NO_REMOTE_DRIVES_EXPLANATORY); 
  	HBufC* myString = HBufC::NewLC(emptyText->Length() + emptyText2->Length() + 5);
  	TPtr emptyStringPtr = myString->Des();
  	emptyStringPtr.Format(KStringHeader, emptyText, emptyText2);
    
    iMainList->SetListBoxObserver(this);
    iMainList->View()->SetListEmptyTextL(*myString);
    CleanupStack::PopAndDestroy( 3 ); //emptyText, emptyText2, myString
	
	CArrayPtrFlat<CGulIcon>* iconArray = new(ELeave) CAknIconArray(4);
	
	CGulIcon* icon2;
	icon2 = AknsUtils::CreateGulIconL(
        AknsUtils::SkinInstance(), 
        KAknsIIDQgnIndiConnectionOnAdd, 
        KGSPluginBitmapFile,
        EMbmRsfwgspluginQgn_indi_connection_on_add,
        EMbmRsfwgspluginQgn_indi_connection_on_add_mask);

	CleanupStack::PushL(icon2);
	iconArray->AppendL(icon2);
	CleanupStack::Pop(icon2);  
	
	CGulIcon* icon3;
	icon3 = AknsUtils::CreateGulIconL(
        AknsUtils::SkinInstance(), 
        KAknsIIDQgnIndiMarkedAdd, 
        AknIconUtils::AvkonIconFileName(),
        EMbmAvkonQgn_indi_marked_add,
        EMbmAvkonQgn_indi_marked_add_mask);

	CleanupStack::PushL(icon3);
	iconArray->AppendL(icon3);
	CleanupStack::Pop(icon3);  
	
	(static_cast<CAknSingleStyleListBox *>(iMainList))->
	    ItemDrawer()->ColumnData()->SetIconArray(iconArray);
 
    SetupListL();
    
    // Start receiving P&S notifier about the changes in drive connection states
    // if the key is not found, will leave with an error code
    // just ignore for now...
    TRAP_IGNORE(iDriveConnectObserver = CRsfwGsPropertyWatch::NewL(this));
        
    SetRect( aRect );
    ActivateL();
    
    iCoeEnv->AddForegroundObserverL( *this );
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::~CRsfwGsPluginDriveListContainer()
// Destructor
//  
// ---------------------------------------------------------------------------
//
CRsfwGsPluginDriveListContainer::~CRsfwGsPluginDriveListContainer()
    {
    if (iMainList)
        {
         delete iMainList;
        }
   
   if (iDriveConnectObserver)
        {
         delete iDriveConnectObserver;
        }   
    iCoeEnv->RemoveForegroundObserver( *this );     
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::SizeChanged()
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::SizeChanged()
    {
    iMainList->SetRect(Rect());
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::CountComponentControls() const
// ---------------------------------------------------------------------------
//
TInt CRsfwGsPluginDriveListContainer::CountComponentControls() const
    {
    return 1;
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::ComponentControl() const
// ---------------------------------------------------------------------------
//
CCoeControl* CRsfwGsPluginDriveListContainer::ComponentControl( TInt aIndex ) const
    {
    switch ( aIndex )
        {
        case 0:
            return iMainList;
        default:
            return NULL;
        }
    }
    

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::EditCurrentItemL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::EditCurrentItemL()
    { 
    HBufC* currentDriveName = GetCurrentRemoteDriveNameLC();
    if (!(iView->IsDriveConnectedL())) 
        {       
        TPtrC drivePtr = currentDriveName->Ptr();
        iCurrentItemIndex=iMainList->CurrentItemIndex();
        TPtrC namePtr = drivePtr.Left(currentDriveName->Length());
        iView->LoadSettingsViewL(EEditExisting, namePtr, ETrue); 
          
        }
    else 
        {
        HBufC* myDisplayMessage = NULL;
        if ( currentDriveName->Length() )
            {
            myDisplayMessage = StringLoader::LoadLC( R_STR_RSFW_ERROR_EDIT_CONNECTED, *currentDriveName );
            }
        else
            {
            myDisplayMessage = StringLoader::LoadLC( R_STR_RSFW_ERROR_EDIT_CONNECTED );
            }
        CAknErrorNote* errorNote = new CAknErrorNote(EFalse);
        errorNote->ExecuteLD(*myDisplayMessage);
        CleanupStack::PopAndDestroy(myDisplayMessage);
        }
    CleanupStack::PopAndDestroy(currentDriveName);    
    }
    


// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::GetCurrentRemoteDriveNameLC()
// ---------------------------------------------------------------------------
//
HBufC* CRsfwGsPluginDriveListContainer::GetCurrentRemoteDriveNameLC()
    {
    TPtrC16 remoteDriveName, remoteDriveNameClipped, remoteDriveNamePtr;
    TInt currentItemIndex = iMainList->CurrentItemIndex();
    if (currentItemIndex == KErrNotFound)
	    {
	    User::Leave(KErrNotFound);
	    }
    remoteDriveName.Set(iRemoteDriveListArray->MdcaPoint(currentItemIndex));
    // remove '\t' from the beginning 
    remoteDriveNameClipped.Set(remoteDriveName.Mid(1)); 
   	// remove the trailing '\tn'if exists
    TInt posit = remoteDriveNameClipped.Find(KTabulator);
    if (posit != KErrNotFound) 
        {
        remoteDriveNamePtr.Set(remoteDriveNameClipped.Left(posit));
        }
    else 
        {
        remoteDriveNamePtr.Set(remoteDriveNameClipped);
        }

    return remoteDriveNamePtr.AllocLC();    
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::GetRemoteDriveNameLC(TInt aIndex)
// ---------------------------------------------------------------------------
//
TPtrC CRsfwGsPluginDriveListContainer::GetRemoteDriveNameL(TInt aIndex)
    {
	TPtrC16 remoteDriveName, remoteDriveNameClipped, remoteDriveNamePtr;
    remoteDriveName.Set(iRemoteDriveListArray->MdcaPoint(aIndex));
    // remove '\t' from the beginning 
    remoteDriveNameClipped.Set(remoteDriveName.Mid(1)); 
   	// remove the trailing '\tn'
    TInt posit = remoteDriveNameClipped.Find(KTabulator);
    if (posit != KErrNotFound) 
        {
        remoteDriveNamePtr.Set(remoteDriveNameClipped.Left(posit));
        }
    else 
        {
        remoteDriveNamePtr.Set(remoteDriveNameClipped);
        }

    return remoteDriveNamePtr;    
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::OfferKeyEventL()
// ---------------------------------------------------------------------------
//
TKeyResponse CRsfwGsPluginDriveListContainer::OfferKeyEventL( const TKeyEvent& aKeyEvent, 
                                                            TEventCode aType )
    {
    if (iMainList)
        {
        // if cancel key is pressed and list is not empty, invoke deletion
        if ((aKeyEvent.iCode == EKeyBackspace ) && (aType == EEventKey) )
            {
            if(!IsListEmpty())
                {
                CRsfwGsPlugin* iTempView = static_cast<CRsfwGsPlugin*> (iView);
                iTempView->DeleteRemoteDriveL();
                }  
            return EKeyWasConsumed;
            }
        else 
            {
            TKeyResponse returnValue = iMainList->OfferKeyEventL (aKeyEvent, aType);
            if ((aKeyEvent.iCode == EKeyUpArrow || aKeyEvent.iCode == EKeyDownArrow ) 
                        && (aType == EEventKey))
                {
                // Cba needs to be updated any time the user goes through the drive list
                // as MSK "Edit" is dependent on connection state
                // Note that we had to call iMainList->OfferKeyEventL() first 
                // to make sure that the correct drive is highlighted
                iCurrentItemIndex = iMainList->CurrentItemIndex();
                iView->UpdateCbaL();            
                }
            return returnValue;
            }
        }
    return EKeyWasNotConsumed;    
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::HandleListBoxEventL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::HandleListBoxEventL(CEikListBox* /*aListBox*/, TListBoxEvent aListBoxEvent)
    {
    // if the Select Key has been pressed
    if ((aListBoxEvent == MEikListBoxObserver::EEventEnterKeyPressed) ||
    (aListBoxEvent == MEikListBoxObserver::EEventItemSingleClicked))
        {
        EditCurrentItemL();           
        }
    }
      
    
  
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::SetupListL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::SetupListL()
    {
    CTextListBoxModel* model = iMainList->Model();
    model->SetOwnershipType(ELbmOwnsItemArray);
    
    iRemoteDriveListArray = static_cast<CDesCArray*>(model->ItemTextArray());
    LoadRemoteDriveListArrayL();
    }    
    


// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::LoadSettingsListArrayL()
// ---------------------------------------------------------------------------
//
TInt CRsfwGsPluginDriveListContainer::LoadRemoteDriveListArrayL()
    {
    iTitlePane->SetTextL(iTitle);
    
    CDesCArray* remoteDriveList = NULL;
    remoteDriveList = GetRemoteDriveNamesL();
    CleanupStack::PushL(remoteDriveList);
    TInt remoteDriveListCount = remoteDriveList->Count();
    iRemoteDriveListArray->Reset();
    for (TInt i = 0; i< remoteDriveListCount; i++)
        {
        TBuf <KMaxFriendlyNameLength+5> string; // maximum name + \t + \tn
   		string.Append(KTabulator);
        string.Append(remoteDriveList->MdcaPoint(i));
        if ( IsDriveConnectedL(remoteDriveList->MdcaPoint(i))) 
       		{
       		string.Append(KConnectedIcon);
        	}
        iRemoteDriveListArray->AppendL(string);	
 
        }
    CleanupStack::PopAndDestroy(remoteDriveList); 
    iMainList->HandleItemRemovalL();
    iMainList->HandleItemAdditionL();
    return remoteDriveListCount;
    }


    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::IsListEmpty()
// ---------------------------------------------------------------------------
//
TBool CRsfwGsPluginDriveListContainer::IsListEmpty()
    {
    if (iRemoteDriveListArray->Count())
        return EFalse;
    return ETrue;    
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::IsLastItem()
// ---------------------------------------------------------------------------
//
TBool CRsfwGsPluginDriveListContainer::IsLastItem()
    {
    if (iRemoteDriveListArray->Count() == 1)
        return ETrue;
    return EFalse;    
    }    
    
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::DeleteCurrentRemoteDriveL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::DeleteCurrentRemoteDriveL()
    {
    TBool deletingLast = EFalse;
    if (iMainList->CurrentItemIndex() == iMainList->BottomItemIndex()) 
        {
    	deletingLast = ETrue;
        }
    
    HBufC* currentDriveName = GetCurrentRemoteDriveNameLC();
    TPtrC drivePtr = currentDriveName->Ptr();
    iMountMan->DeleteMountEntryL(drivePtr.Left(currentDriveName->Length()));  
    iRemoteDriveListArray->Delete(iMainList->CurrentItemIndex());
    iMainList->HandleItemRemovalL();
    iMainList->DrawDeferred();
    CleanupStack::PopAndDestroy(currentDriveName);
    
    if (deletingLast) 
        {
    	if (iRemoteDriveListArray->Count() > 0) 
    		{
    		iMainList->SetCurrentItemIndex(iMainList->BottomItemIndex());	
    		}
    	
        }
    }    

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::IsDriveConnectedL()
// ---------------------------------------------------------------------------
//    
TBool CRsfwGsPluginDriveListContainer::IsDriveConnectedL(const TDesC& aName)
    {
    
    TInt err;
    const CRsfwMountEntry* mountEntry;
    TRsfwMountInfo mountInfo;
    TChar driveL;
    mountEntry = mountEntry = iMountMan->MountEntryL(aName);
    if (!mountEntry) 
        {
        User::Leave(KErrNotFound);
        }
    const HBufC* drive = (mountEntry->Item(EMountEntryItemDrive));
    if (drive && drive->Length())
        {
        driveL =(*drive)[0];
        } 
    else 
    	{
    	// getting drive letter failed
    	return EFalse;
    	}
    err = iMountMan->GetMountInfo(driveL, mountInfo);
    if ((err == KErrNone) && (mountInfo.iMountStatus.iConnectionState == KMountStronglyConnected)) 
    	{
    	return ETrue;
    	}
    return EFalse;  
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::SetDriveConnectedStateL()
// ---------------------------------------------------------------------------
// 
void CRsfwGsPluginDriveListContainer::SetDriveConnectedStateL(const TDesC& aName, TBool aConnected)
	{
	TInt i;
	for (i=0; i < iRemoteDriveListArray->Count(); i++) 
		{
		TPtrC driveString =  GetRemoteDriveNameL(i);
		if (driveString.Compare(aName) == 0) 
			{
			// newString has space for the friendly name, tabulator and connected icon
			TBuf<KMaxFriendlyNameLength+5> newString; 
			newString.Append(KTabulator);
		    newString.Append(driveString);	
   			if (aConnected) 
   				{
   				newString.Append(KConnectedIcon);
   				}
   			iRemoteDriveListArray->Delete(i);
    		iRemoteDriveListArray->InsertL(i, newString);	
    		iMainList->DrawDeferred();
    		// Cba needs to be updated as MSK "Edit" is dependent on connection state
    		iView->UpdateCbaL();		
		break;
			}
	
		}
	
	}

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::ConnectCurrentRemoteDriveL()
// ---------------------------------------------------------------------------
//  
void CRsfwGsPluginDriveListContainer::ConnectCurrentRemoteDriveL() 
{  
	User::LeaveIfError(iMountMan->SetMountConnectionStateBlind(GetCurrentRemoteDriveIdL(), 
	                                    KMountStronglyConnected));
}

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::DisconnectCurrentRemoteDriveL()
// ---------------------------------------------------------------------------
//  
void CRsfwGsPluginDriveListContainer::DisconnectCurrentRemoteDriveL() 
{
	User::LeaveIfError(iMountMan->SetMountConnectionState(GetCurrentRemoteDriveIdL(), 
	                                    KMountNotConnected));

}

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::GetNextRemoteDriveNameL
// ---------------------------------------------------------------------------
//
TDesC& CRsfwGsPluginDriveListContainer::GetNextRemoteDriveNameL()
    {
    // Always return "New drive" as the default starting name
    // in case there is already a drive with the name "new name"
    // user is not allowed to save the new drive before changing the name.
    StringLoader::Load(iSettingNewName, R_STR_REMOTE_DRIVE_NEW_NAME);
    return iSettingNewName; 
    }
    
    
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::GetCurrentRemoteDriveIdL
// ---------------------------------------------------------------------------
//
TChar CRsfwGsPluginDriveListContainer::GetCurrentRemoteDriveIdL()
    {
    const CRsfwMountEntry* mountEntry;
   	HBufC* currentDriveName = GetCurrentRemoteDriveNameLC();
   	TPtrC drivePtr = currentDriveName->Ptr();
 	mountEntry = iMountMan->MountEntryL(drivePtr.Left(currentDriveName->Length()));
 	if (!mountEntry) 
 	    {
 	    User::Leave(KErrNotFound);
 	    }
   	CleanupStack::PopAndDestroy(currentDriveName);
   	const HBufC* drive = (mountEntry->Item(EMountEntryItemDrive));
    if (drive && drive->Length())
        {
        return (*drive)[0];
        }
    
    // should not come here....
    // return the drive letter of the default drive
    RFs fs;
	User::LeaveIfError(fs.Connect());
	TChar defaultd;
	fs.DriveToChar(EDriveC, defaultd);
	fs.Close();
	return defaultd;
    }    
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::GetRemoteDriveNamesL
// ---------------------------------------------------------------------------
//
CDesCArray* CRsfwGsPluginDriveListContainer::GetRemoteDriveNamesL()
    {
    CDesCArray* myArray = new (ELeave) CDesC16ArraySeg(4);
    iMountMan->GetMountNamesL(myArray);
    return myArray;
    }
          
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::RemoteDriveCount
// ---------------------------------------------------------------------------
//
TInt CRsfwGsPluginDriveListContainer::RemoteDriveCount()
    {
   	return iRemoteDriveListArray->Count();
    }
          
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::HandleResourceChange()
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::HandleResourceChange( TInt aType )
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
        CRsfwGsPlugin* iTempView = static_cast<CRsfwGsPlugin*> (iView);    

        iTempView->HandleResourceChangeManual(aType);      
        }
    CCoeControl::HandleResourceChange( aType );
    }
  
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::HandleResourceChangeManual()
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::HandleResourceChangeManual(TInt aType)
    {
    if (aType != KAknsMessageSkinChange) 
        {
        TRect mainPaneRect;
        AknLayoutUtils::LayoutMetricsRect( AknLayoutUtils::EMainPane,
                                       mainPaneRect);
        SetRect( mainPaneRect );
        }
    DrawDeferred();
    iMainList->HandleResourceChange(aType);
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::FocusChanged
// Set focus on the selected listbox. For animated skins feature.
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::FocusChanged(TDrawNow /*aDrawNow*/)
    {
    if(iMainList)
        {
        iMainList->SetFocus( IsFocused() );
        }
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::GetHelpContext
// This function is called when Help application is launched.
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::GetHelpContext( TCoeHelpContext& aContext ) const
    {
    aContext.iMajor = KGSRsfwPluginUID;
    }    

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::HandleGainingForeground
// ---------------------------------------------------------------------------
//    
void CRsfwGsPluginDriveListContainer::HandleGainingForeground() 
    {
    TRAP_IGNORE(HandleGainingForegroundL());
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::HandleGainingForegroundL
// Called from HandleGainingForeground in order to trap potential leaves.
// ---------------------------------------------------------------------------
//    
void CRsfwGsPluginDriveListContainer::HandleGainingForegroundL() 
    {
    if (iView->CurrentContainer() == this) 
        {
        // update the drive list
        LoadRemoteDriveListArrayL();
        SetFocus();
        iView->ProcessDeletingDialog();
        iView->UpdateCbaL();
        }
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::HandleLosingForeground
// ---------------------------------------------------------------------------
//        
void CRsfwGsPluginDriveListContainer::HandleLosingForeground() 
    {
    
    }   
    
// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::SetFocus
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::SetFocus()
    { 
    TInt remoteDriveCount = iRemoteDriveListArray->Count();
    
    if (remoteDriveCount > 0) 
        {
        if (iCurrentItemIndex >= remoteDriveCount) 
            {
            // set to the beginning of the list
            iCurrentItemIndex = 0;
            }
        
        iMainList->SetCurrentItemIndex(iCurrentItemIndex);
        }
    }

// ---------------------------------------------------------------------------
// CRsfwGsPluginDriveListContainer::SetFocusL
// finds the given name on the drive list and sets focus accordingly
// ---------------------------------------------------------------------------
//
void CRsfwGsPluginDriveListContainer::SetFocusL(const TDes& aDriveName)
    {
    CDesCArray* remoteDriveList = NULL;
    remoteDriveList = GetRemoteDriveNamesL();
    CleanupStack::PushL(remoteDriveList);
    
    for ( TInt i = 0; i < remoteDriveList->Count(); i++ )
        { 
        if (aDriveName.Compare(remoteDriveList->MdcaPoint(i)) == 0)
            {
            iMainList->SetCurrentItemIndex(i);
            CleanupStack::PopAndDestroy(remoteDriveList); 
            return;
            }
        }
    CleanupStack::PopAndDestroy(remoteDriveList);
        
    // if drive name not found on the list, use default behaviour
    SetFocus();
    }
    
        
//  End of File  
