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
* Description:  RsfwPlugin Implementation
*
*/


// Includes
#include <StringLoader.h>
#include <rsfwgspluginrsc.rsg> 
#include <gsfwviewuids.h> // for KUidGS
#include <aknnotewrappers.h>
#include <hlplch.h>
#include <utf.h>
#include <rsfwgsplugin.mbg>
#include <featmgr.h>
#include <csxhelp/cp.hlp.hrh>
#include <rsfwmountman.h>
#include <rsfwmountentry.h>

#include "rsfwgsplugin.h"
#include "rsfwinterface.h"
#include "rsfwgsplugindrivelistcontainer.h"
#include "rsfwgsremotedrivesettingscontainer.h"
#include "rsfwgslocalviewids.h"


#define KUidGeneralSettings   0x100058EC

// ========================= MEMBER FUNCTIONS ================================

// ---------------------------------------------------------------------------
// CRsfwGsPlugin::CRsfwGsPlugin()
// Constructor
//
// ---------------------------------------------------------------------------
//
CRsfwGsPlugin::CRsfwGsPlugin( )
    : iAppUi( CAknView::AppUi() ), iResources( *CCoeEnv::Static() )
    {
    
    }

// ---------------------------------------------------------------------------
// CRsfwGsPlugin::~CRsfwGsPlugin()
// Destructor
//
// ---------------------------------------------------------------------------
//
CRsfwGsPlugin::~CRsfwGsPlugin()
    {    
    FeatureManager::UnInitializeLib();
    
    if (iCurrentContainer && iAppUi)
        {
        iAppUi->RemoveFromViewStack( *this, iCurrentContainer );
        }
    delete iMainListContainer;
    delete iSettingListContainer;
    delete iMountMan;
    delete iSender;
    iResources.Close();
    /** Nice to know when the plugin is cleaned up */
    #ifdef _DEBUG
    RDebug::Print( _L( "[CRsfwGsPlugin] ~CRsfwGsPlugin()" ) );
    #endif

    }

// ---------------------------------------------------------------------------
// CRsfwGsPlugin::ConstructL()
// Symbian OS two-phased constructor
// ---------------------------------------------------------------------------
//
void CRsfwGsPlugin::ConstructL()
    {
    // To know if the plugin is loaded (for debugging)
    #ifdef _DEBUG
    RDebug::Print(_L("[CRsfwGsPlugin] ConstructL()" ));
    RDebug::Print( _L( "[CRsfwGsPlugin] Loading resource from :" ) );
    RDebug::Print( KRsfwGsPluginResourceFileName );
    #endif
    
   	iMountMan = CRsfwMountMan::NewL(0, NULL);   
   	
   	iSender = CRsfwGsRemoteDriveSend::NewL(EGSCmdAppSendLink);
    
    TFileName fileName( KRsfwGsPluginResourceFileName );
    iResources.OpenL( fileName ); 
    BaseConstructL( R_GS_RSFW_MAIN_VIEW );
    	
    FeatureManager::InitializeLibL();		
   	
    }

// ---------------------------------------------------------------------------
// CRsfwGsPlugin::NewL()
// Static constructor
// ---------------------------------------------------------------------------
//
CRsfwGsPlugin* CRsfwGsPlugin::NewL( TAny* /*aInitParams*/ )
    {
    CRsfwGsPlugin* self = new(ELeave) CRsfwGsPlugin( );
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }


// ---------------------------------------------------------------------------
// CRsfwGsPlugin::Id() const
// ---------------------------------------------------------------------------
//
TUid CRsfwGsPlugin::Id() const
    {
    return KGSRsfwPluginUID;
    }

// ---------------------------------------------------------------------------
// CRsfwGsPlugin::HandleClientRectChange()
// ---------------------------------------------------------------------------
//
void CRsfwGsPlugin::HandleViewRectChange()
    {
    if ( iMainListContainer->IsVisible() )
        {
        iMainListContainer->SetRect( ClientRect() );
        }
        
    if ( iSettingListContainer->IsVisible() )
        {
        iSettingListContainer->SetRect( ClientRect() );
        }
    }

// ---------------------------------------------------------------------------
// CRsfwGsPlugin::DoActivateL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPlugin::DoActivateL( const TVwsViewId& aPrevViewId,
                                  TUid aCustomMessageId,
                                  const TDesC8& aCustomMessage )
    {
    iPrevViewId = aPrevViewId;
    if (iCurrentContainer)
        {
        iAppUi->RemoveFromViewStack( *this, iCurrentContainer );
        iCurrentContainer = NULL;
        }
        

    if( iMainListContainer )
        {
        delete iMainListContainer;
        iMainListContainer=NULL;
        }

    if( iSettingListContainer )
        {
        delete iSettingListContainer;
        iSettingListContainer = NULL;
        }
            
    iMainListContainer = new( ELeave ) CRsfwGsPluginDriveListContainer(this);
    iMainListContainer->SetMopParent(this);
    TRAPD( error, iMainListContainer->ConstructL( ClientRect(), iMountMan ) );
    if (error)
        {
        delete iMainListContainer;
        iMainListContainer = NULL;
        User::Leave( error );
        }
        
    iSettingListContainer = new( ELeave ) CRsfwGsPluginDriveSettingsContainer(this);
    iSettingListContainer->SetMopParent(this);
    TRAPD( error1, iSettingListContainer->ConstructL( ClientRect(), iMountMan));
    if (error1)
        {
        delete iSettingListContainer;
        iSettingListContainer = NULL;
        User::Leave( error1 );
        }

  if (aCustomMessageId == KRemoteDriveSettingsViewId)  
    	{
    	TBuf<KMaxFriendlyNameLength> remoteDrive;
    	CnvUtfConverter::ConvertToUnicodeFromUtf8(remoteDrive, aCustomMessage);
    	iSettingListContainer->MakeVisible(ETrue);
    	if (remoteDrive.Length() > 0) 
    	    {
    	    LoadSettingsViewL(EEditExisting, remoteDrive, EFalse); 
    	    }
    	 else 
    	    {
    	    LoadSettingsViewL(ENewDefault, 
                    iMainListContainer->GetNextRemoteDriveNameL(), EFalse);
    	    }
    	iCurrentContainer = iSettingListContainer;
    	} 
    else 
    	{
    	iSettingListContainer->MakeVisible(EFalse);
    	iCurrentContainer = iMainListContainer;
    	}

    UpdateCbaL();
    iAppUi->AddToViewStackL( *this, iCurrentContainer ); 
    }


// ---------------------------------------------------------------------------
// CRsfwGsPlugin::DoDeactivate()
// ---------------------------------------------------------------------------
//
void CRsfwGsPlugin::DoDeactivate()
    {
    // try to save settings if in settings list container
    if ( iCurrentContainer && iSettingListContainer
        && iCurrentContainer == iSettingListContainer )
        {
        TRAP_IGNORE(iSettingListContainer->SaveSettingsL());
        }
        
    if ( iCurrentContainer )
        {
        iAppUi->RemoveFromViewStack( *this, iCurrentContainer );
        }
    iCurrentContainer = NULL;

    if( iMainListContainer )
        {
        delete iMainListContainer;
        iMainListContainer = NULL;
        }
        
    if( iSettingListContainer )
        {
        delete iSettingListContainer;
        iSettingListContainer = NULL;
        }        
    }



// ---------------------------------------------------------------------------
// CRsfwGsPlugin::HandleCommandL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPlugin::HandleCommandL( TInt aCommand )
    {
  	switch ( aCommand )
        {
        case EAknSoftkeyBack:
            if (iCurrentContainer == iSettingListContainer)
                {
                if (iSettingListContainer->IsExitProcessingOKL()) 
                	{
                	// if we are called from GS, go back to the main view
                	// otherwise we are called e.g. from FileManager =>
                	// go back to the previous view
                	if (iPrevViewId.iAppUid == KUidGS)
                		{
                		LoadMainViewL();
                		// set focus on the newly created/modified drive
                		// we have to pass the name cause in case a drive has just been created
                		// we don't know its position on the list
                		iMainListContainer->SetFocusL(iSettingListContainer->GetCurrentRemoteDriveName());
                		}
                	else 
                		{
                		iAppUi->ActivateLocalViewL( iPrevViewId.iViewUid );	
                		}
                	
                	}              
                }
            else  
          	  {
               iAppUi->ActivateLocalViewL( iPrevViewId.iViewUid );
           	  }
                
            break;
        case EGSCmdAppEdit:
            if (!(iMainListContainer->IsListEmpty())) 
                {
                iMainListContainer->EditCurrentItemL();
                }
        	break;
        case EGSCmdAppChange:
            iSettingListContainer->EditCurrentItemL();                       
            break;   
        case EGSCmdAppNew:
        	// Number of remote drives is limited to 9, so that drive
        	// letters are also available for other technologies.
        	if (iMainListContainer->RemoteDriveCount() < KMaxRemoteDrives) 
        	{
      			LoadSettingsViewL(ENewDefault, 
                    iMainListContainer->GetNextRemoteDriveNameL(), ETrue);
        	}
        	else 
        	{
                HBufC* myDisplayMessage = NULL;
                myDisplayMessage = StringLoader::LoadLC(R_STR_RSFW_ERROR_MAX_DRIVES);
                CAknErrorNote* errorNote = new CAknErrorNote(EFalse);
                errorNote->ExecuteLD(*myDisplayMessage);
                CleanupStack::PopAndDestroy(myDisplayMessage);
        	}
            break;
        case EGSCmdAppDelete:
            DeleteRemoteDriveL();
            break;
       	case EGSCmdAppConnect:
       		ConnectRemoteDriveL();
       		break;
       	case EGSCmdAppDisconnect:
       		DisconnectRemoteDriveL();
       		break;	
        case EAknCmdHelp:
        	{
            if (iCurrentContainer == iSettingListContainer)
                {
                LaunchHelpL(KRD_HLP_REMOTE_DRIVE_CONFIG);
                }
               
            else 
                {
                LaunchHelpL(KRD_HLP_REMOTE_DRIVES);
                }
            
        	}
        	break;
        case EGSCmdAppSendLink:
        	{
        	SendLinkL();
        	}
        	break;
        default:
            iAppUi->HandleCommandL( aCommand );
            break;
        }
    }
    

// ---------------------------------------------------------------------------
// CRsfwGsPlugin::GetCaptionL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPlugin::GetCaptionL( TDes& aCaption ) const
    {
    StringLoader::Load( aCaption, R_GS_REMOTE_DRIVES_VIEW_CAPTION );
    }

// ---------------------------------------------------------------------------
// CRsfwGsPlugin::CreateIconL()
// ---------------------------------------------------------------------------
//
 CGulIcon* CRsfwGsPlugin::CreateIconL( const TUid aIconType )
 	{
 	 CGulIcon* icon;
       
    if( aIconType == KGSIconTypeLbxItem )
        {
        icon = AknsUtils::CreateGulIconL(
        AknsUtils::SkinInstance(), 
        KAknsIIDQgnPropSetConnRemotedrive, 
        KGSPluginBitmapFile,
        EMbmRsfwgspluginQgn_prop_set_conn_remotedrive,
        EMbmRsfwgspluginQgn_prop_set_conn_remotedrive_mask);
        }
     else
        {
        icon = CGSPluginInterface::CreateIconL( aIconType );
        }
 	return icon;
 	}


// ---------------------------------------------------------------------------
// CRsfwGsPlugin::LoadSettingsViewL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPlugin::LoadSettingsViewL(TRsfwSettingsViewType aType, TDesC& aRemoteDrive, TBool aAddToViewStack)
    {
    switch(aType)
        {
        case EEditExisting:
            iSettingListContainer->PrepareRemoteDriveForEditingL(aRemoteDrive);
            break;
        case ENewDefault:
            iSettingListContainer->PrepareRemoteDriveNewDefaultL(aRemoteDrive);
            break;
        default:
            break;
        }
    if (iCurrentContainer)
        iAppUi->RemoveFromViewStack( *this, iCurrentContainer );
    iCurrentContainer = iSettingListContainer;
    if (aAddToViewStack) 
        {
        iAppUi->AddToViewStackL( *this, iCurrentContainer );
        }
    iMainListContainer->MakeVisible(EFalse);
    iSettingListContainer->MakeVisible(ETrue);
    UpdateCbaL();
    }


// ---------------------------------------------------------------------------
// CRsfwGsPlugin::LoadMainViewL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPlugin::LoadMainViewL()
    {
    iMainListContainer->LoadRemoteDriveListArrayL();
    if (iCurrentContainer)
        iAppUi->RemoveFromViewStack( *this, iCurrentContainer );
    iCurrentContainer = iMainListContainer;
    iAppUi->AddToViewStackL( *this, iCurrentContainer );
   
    iMainListContainer->SetFocus();
   
    iSettingListContainer->MakeVisible(EFalse);  
    UpdateCbaL();
    iMainListContainer->MakeVisible(ETrue);
    Cba()->DrawDeferred();  
    }
    

    
// ---------------------------------------------------------------------------
// CRsfwGsPlugin::DynInitMenuPaneL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPlugin::DynInitMenuPaneL( TInt aResourceId, 
                                      CEikMenuPane* aMenuPane )
    {
    if ( aResourceId == R_GS_REMOTE_DRIVES_MENU )
        {
        if (iCurrentContainer == iMainListContainer)
       		{
        	aMenuPane->SetItemDimmed(EGSCmdAppChange, ETrue);
        	if (iMainListContainer->IsListEmpty())
            	{
				aMenuPane->SetItemDimmed( EGSCmdAppEdit, ETrue );
            	aMenuPane->SetItemDimmed( EGSCmdAppDelete, ETrue ); 
            	aMenuPane->SetItemDimmed( EGSCmdAppConnect, ETrue );
            	aMenuPane->SetItemDimmed( EGSCmdAppDisconnect, ETrue );  
            	aMenuPane->SetItemDimmed( EGSCmdAppSendLink, ETrue );
            	}
            else 
                {
                TBool isDriveConnected = IsDriveConnectedL();
                aMenuPane->SetItemDimmed( EGSCmdAppDelete, isDriveConnected ); 
                aMenuPane->SetItemDimmed( EGSCmdAppConnect, isDriveConnected );  
                aMenuPane->SetItemDimmed( EGSCmdAppEdit, isDriveConnected );  
            	aMenuPane->SetItemDimmed( EGSCmdAppDisconnect, !isDriveConnected );  	
                }
       		}
        else 
        	{
        	aMenuPane->SetItemDimmed( EGSCmdAppNew, ETrue );
    		aMenuPane->SetItemDimmed( EGSCmdAppEdit, ETrue );
            aMenuPane->SetItemDimmed( EGSCmdAppDelete, ETrue ); 
            aMenuPane->SetItemDimmed( EGSCmdAppConnect, ETrue );
            aMenuPane->SetItemDimmed( EGSCmdAppDisconnect, ETrue );  
            aMenuPane->SetItemDimmed( EGSCmdAppSendLink, ETrue );
        	}
        
        }
        
     if (aResourceId == R_GS_MENU_ITEM_EXIT) 
        {
       if (!FeatureManager::FeatureSupported( KFeatureIdHelp )) 
            {
            aMenuPane->SetItemDimmed( EAknCmdHelp , ETrue );
            }
        }       
    }


// ---------------------------------------------------------------------------
// CRsfwGsPlugin::DeleteRemoteDriveL()
// ---------------------------------------------------------------------------
//
void CRsfwGsPlugin::DeleteRemoteDriveL()
    {
  
    HBufC* myFormatMessage = NULL;
    HBufC* here = iMainListContainer->GetCurrentRemoteDriveNameLC();
    TChar driveId = iMainListContainer->GetCurrentRemoteDriveIdL();
    
    if (!iMainListContainer->IsDriveConnectedL(*here)) 
        {
        if ( here->Length() )
            {
            myFormatMessage = StringLoader::LoadLC( R_STR_RSFW_CONF_DELETE, *here );
            }
        else
            {
            myFormatMessage = StringLoader::LoadLC( R_STR_RSFW_CONF_DELETE );
            }
    
        CAknQueryDialog* query = CAknQueryDialog::NewL(CAknQueryDialog::EConfirmationTone);
        
        iDialog = query;
        iDeletingId = iMainListContainer->GetCurrentRemoteDriveIdL();
   
        if ( query->ExecuteLD( R_CONFIRMATION_QUERY, *myFormatMessage ) )
            {
            if (iMainListContainer->RemoteDriveCount() > 0) 
                {
                TChar currentdriveId = iMainListContainer->GetCurrentRemoteDriveIdL();
                if (driveId == currentdriveId) 
                    {
                    iMainListContainer->DeleteCurrentRemoteDriveL();
                    }
                }
            }
        iDialog = NULL;
        
        CleanupStack::PopAndDestroy(myFormatMessage); 
    
        UpdateCbaL();  

        }
    CleanupStack::PopAndDestroy(here);
    }
    
// ---------------------------------------------------------------------------
// CRsfwGsPlugin::ProcessDeletingDialog()
// ---------------------------------------------------------------------------
//
void CRsfwGsPlugin::ProcessDeletingDialog()
    {
	  if ( (iMainListContainer->RemoteDriveCount() == 0) || 
            (iDeletingId != iMainListContainer->GetCurrentRemoteDriveIdL()) )
        {
        if (iDialog)
            {
            delete iDialog;
			      iDialog = NULL;
			      }
		    }
	  }
    
// ---------------------------------------------------------
// CRsfwGsPlugin::LaunchHelpL()
// ---------------------------------------------------------
//
void CRsfwGsPlugin::LaunchHelpL( const TDesC& aContext )
	{
	//make context array
    //granurality 1 is ok cos there is added just one item
    CArrayFix< TCoeHelpContext >* cntx = new( ELeave ) CArrayFixFlat< TCoeHelpContext >( 1 );
    CleanupStack::PushL( cntx );

    cntx->AppendL( TCoeHelpContext( TUid::Uid(KUidGeneralSettings), aContext ) );
    CleanupStack::Pop( cntx );

    //and launch help - takes ownership of context array
    HlpLauncher::LaunchHelpApplicationL( iEikonEnv->WsSession(), cntx);
	}    
	
// ---------------------------------------------------------
// CRsfwGsPlugin::HandleResourceChangeManual
// ---------------------------------------------------------
//
void CRsfwGsPlugin::HandleResourceChangeManual(TInt aType)
    {
    if (iCurrentContainer==iSettingListContainer)
        iMainListContainer->HandleResourceChangeManual(aType);
    else if (iCurrentContainer==iMainListContainer)
        iSettingListContainer->HandleResourceChangeManual(aType);    
    }  
    
// ---------------------------------------------------------------------------
// CRsfwGsPlugin::IsDriveConnected()
// ---------------------------------------------------------------------------
//    
TBool CRsfwGsPlugin::IsDriveConnectedL()
    {
    TBool connected;
	HBufC* currentDriveName = iMainListContainer->GetCurrentRemoteDriveNameLC();
	if (!currentDriveName) 
	    {
	    User::Leave(KErrNotFound);
	    }
    TPtrC drivePtr = currentDriveName->Ptr();
    connected = iMainListContainer->IsDriveConnectedL(drivePtr.Left(currentDriveName->Length()));
    CleanupStack::PopAndDestroy(currentDriveName);
    return connected;  
    }

// ---------------------------------------------------------------------------
// CRsfwGsPlugin::ConnectRemoteDrive()
// ---------------------------------------------------------------------------
//    
void CRsfwGsPlugin::ConnectRemoteDriveL()
    {
    iMainListContainer->ConnectCurrentRemoteDriveL();  
    }    
    
// ---------------------------------------------------------------------------
// CRsfwGsPlugin::DisconnectRemoteDrive()
// ---------------------------------------------------------------------------
//    
void CRsfwGsPlugin::DisconnectRemoteDriveL()
    {
    iMainListContainer->DisconnectCurrentRemoteDriveL();  
    }    
   
   

// ----------------------------------------------------
//  CRsfwGsPlugin::DoSendAsL
// ----------------------------------------------------
//
void CRsfwGsPlugin::DoSendAsL()
    {
    // read the source setting
    const CRsfwMountEntry* mountEntry = NULL;
    HBufC* currentDriveName = iMainListContainer->GetCurrentRemoteDriveNameLC();
    TPtrC drivePtr = currentDriveName->Ptr();
    mountEntry = iMountMan->MountEntryL(
    						drivePtr.Left(currentDriveName->Length()));
    CleanupStack::PopAndDestroy(currentDriveName);
    iSender->SendL( *mountEntry );
    }
   
   
void CRsfwGsPlugin::SendLinkL( )
	{
	HBufC* driveBuf = iMainListContainer->GetCurrentRemoteDriveNameLC();	
	iSender->DisplaySendCascadeMenuL();

    // make sure that the mountentry was not deleted
    if (driveBuf) 
        {	
        const CRsfwMountEntry* mountentry = iMountMan->MountEntryL(*driveBuf);
	    CleanupStack::PopAndDestroy(driveBuf);
	    if (mountentry) 
	        {
	        if (iSender->CanSend()) 
		        {
		        DoSendAsL();
		        }   
	        }
        }

	}
	
void CRsfwGsPlugin::UpdateCbaL() 
    {
    CEikButtonGroupContainer* cbaGroup = Cba();
    
    if (iCurrentContainer == iMainListContainer) 
        {
        cbaGroup->SetCommandSetL(R_RSFW_SOFTKEYS_OPTIONS_BACK_EDIT);
        if (iMainListContainer->IsListEmpty())
            {
            cbaGroup->MakeCommandVisible(EGSCmdAppEdit, EFalse);
            }
        else if (IsDriveConnectedL()) 
            {
            cbaGroup->MakeCommandVisible(EGSCmdAppEdit, EFalse);
            }
        else
            {
            cbaGroup->MakeCommandVisible(EGSCmdAppEdit, ETrue);
            }
        }
    else if (iCurrentContainer == iSettingListContainer) 
        {
        cbaGroup->SetCommandSetL(R_RSFW_SOFTKEYS_OPTIONS_BACK_CHANGE);
        // it is possible that the MSK command was dimmed 
        cbaGroup->MakeCommandVisible(EGSCmdAppChange, ETrue);
        }
    
    Cba()->DrawDeferred();    
    }
  
CCoeControl* CRsfwGsPlugin::CurrentContainer() 
    {
    return iCurrentContainer;
    }  
    
// End of file

