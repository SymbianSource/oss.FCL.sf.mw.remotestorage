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
* Description:  Rsfw GS plugin, Setting List class implementation
*
*/


// INCLUDE FILES
#include <akntitle.h>
#include <ApSettingsHandlerUI.h>
#include <ApUtils.h>
#include <commdb.h>
#include <eikspane.h>
#include <akntextsettingpage.h>
#include <ConnectionUiUtilities.h>

#include "rsfwgspluginsettinglist.h"
#include "rsfwgssettingsdata.h"
#include "mdebug.h"

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::NewL(CRsfwGsSettingsData &aData)
// -----------------------------------------------------------------------------
// 
CRsfwGsPluginSettingsList *CRsfwGsPluginSettingsList::NewL(CRsfwGsSettingsData &aData)
    {
    CRsfwGsPluginSettingsList* self = CRsfwGsPluginSettingsList::NewLC(aData);
    CleanupStack::Pop(self);
    return self;
    }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::NewLC(CRsfwGsSettingsData &aData)
// -----------------------------------------------------------------------------
// 
CRsfwGsPluginSettingsList *CRsfwGsPluginSettingsList::NewLC(CRsfwGsSettingsData &aData)
    {
    CRsfwGsPluginSettingsList* self = new (ELeave) CRsfwGsPluginSettingsList(aData);
    CleanupStack::PushL(self);
    return self;
    }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::CRsfwGsPluginSettingsList(CRsfwGsSettingsData &aData)
// -----------------------------------------------------------------------------
// 
CRsfwGsPluginSettingsList::CRsfwGsPluginSettingsList(CRsfwGsSettingsData &aData) : 
    CAknSettingItemList(),
    iSettingsData(aData)  
    {
    }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::~CRsfwGsPluginSettingsList()
// -----------------------------------------------------------------------------
// 
CRsfwGsPluginSettingsList::~CRsfwGsPluginSettingsList()
  {
#if defined __WINS__ 
  iDlgSrv.Close();
#endif
  }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::SizeChanged()
// -----------------------------------------------------------------------------
// 
void CRsfwGsPluginSettingsList::SizeChanged()
    {
    // if size changes, make sure component takes whole available space
    if (ListBox()) 
        {
        ListBox()->SetRect(Rect());
        }
    }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::EditCurrentItemL()
// -----------------------------------------------------------------------------
// 
void CRsfwGsPluginSettingsList::EditCurrentItemL()
    {
    // invoke EditItemL on the current item
    
    TInt index = ListBox()->CurrentItemIndex();
	EditItemL(index,ETrue); // invoked from menu
    }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::EditItemL (TInt aIndex, TBool aCalledFromMenu)
// -----------------------------------------------------------------------------
// 
void CRsfwGsPluginSettingsList::EditItemL (TInt aIndex, TBool aCalledFromMenu)
    {
    if (aIndex == EAccessPointIndex)
      {
      EditAccessPointL();
      }
    else
        {
      	TInt cflags = (*SettingItemArray())[aIndex]->SettingPageFlags();
      	// allow user to exit address field without typing anything,
      	// if he e.g. realizes that he does not remember the address
        if ((aIndex == EUserIDIndex) || (aIndex == EServerAddressIndex))
       		{
        	cflags |= CAknTextSettingPage::EZeroLengthAllowed;
       	 	}
    	else
       		{
            //coverity[logical_vs_bitwise]
        	cflags &= (!CAknTextSettingPage::EZeroLengthAllowed);
        	}
        (*SettingItemArray())[aIndex]->SetSettingPageFlags(cflags);	
         CAknSettingItemList::EditItemL(aIndex, aCalledFromMenu);
        (*SettingItemArray())[aIndex]->StoreL();
        }
    SaveSettingL(aIndex);
    }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::SaveSettingL(TInt aIndex)
// -----------------------------------------------------------------------------
// 
void CRsfwGsPluginSettingsList::SaveSettingL(TInt aIndex)
    {
    
    switch (aIndex)
        {
        case ESettingNameIndex:
            SetTitlePaneTextL(iSettingsData.iSettingName);         
            break;
        case EServerAddressIndex:
            break;
        
        case EAccessPointIndex:
            if (iSettingsData.iAccessPoint > -1) // if Valid AP number
                {
                (iSettingsData.iAccessPointDes).Num(iSettingsData.iAccessPoint);
                GetAccessPointNameL(iSettingsData.iAccessPoint, 
                                                    iSettingsData.iAccessPointName);
                }
            break;
        case EUserIDIndex:
            break;
            
        case EPasswordIndex:
            break;
            
        default:
            break;
        }
    LoadSettingsL();  
    DrawNow();  
    }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::CreateSettingItemL (TInt aIdentifier) 
// -----------------------------------------------------------------------------
// 
CAknSettingItem * CRsfwGsPluginSettingsList::CreateSettingItemL (TInt aIdentifier) 
    {
    // method is used to create specific setting item as required at run-time.
    // aIdentifier is used to determine what kind of setting item should be 
    // created

    CAknSettingItem* settingItem = NULL;

    switch (aIdentifier)
        {
        case ESettingItemDriveName:
            settingItem = new (ELeave) CAknTextSettingItem ( aIdentifier,
                                                   iSettingsData.iSettingName);
            break;
       case ESettingItemURL:
            settingItem = new (ELeave) CAknTextSettingItem (
                          aIdentifier, 
                          iSettingsData.iURL);           
            break;    

        case ESettingItemAccessPoint:
            GetAccessPointNameL(iSettingsData.iAccessPoint, iSettingsData.iAccessPointName);
            settingItem = new (ELeave) CAknTextSettingItem (
                          aIdentifier, iSettingsData.iAccessPointName);
            break;

   

        case ESettingItemUserID:
            settingItem = new (ELeave) CAknTextSettingItem (
                          aIdentifier, 
                          iSettingsData.iUserID);
            settingItem->SetEmptyItemTextL(KNullDesC);
            settingItem->LoadL();        
            break;
        case ESettingItemPassword:
            settingItem = new (ELeave) CAknPasswordSettingItem (
                          aIdentifier, 
                          CAknPasswordSettingItem::EAlpha,
                          iSettingsData.iPassword);         
            break;
        default:
            break;
        }
    return settingItem;
    }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::EditAccessPoint()
// -----------------------------------------------------------------------------
// 
void CRsfwGsPluginSettingsList::EditAccessPointL()
    {

    CCommsDatabase* commsDb = CCommsDatabase::NewL( EDatabaseTypeIAP );
    CleanupStack::PushL(commsDb);
    CApUtils* aPUtils = CApUtils::NewLC( *commsDb );

    TUint32 id = 0;
    TRAP_IGNORE(id = aPUtils->WapIdFromIapIdL(iSettingsData.iAccessPoint));
    
    CConnectionUiUtilities* connUiUtils = 
                CConnectionUiUtilities::NewL();            
	CleanupStack::PushL( connUiUtils );
	
    TCuuAlwaysAskResults result;
    // determine how radio button will be initially displayed to the user
    // (depending on what is the current IAP choice)
    iSettingsData.iAccessPoint < 0 ? result = ECuuAlwaysAsk : result = ECuuUserDefined;
	
	if (connUiUtils->AlwaysAskPageL(result)) 
		{
		if (result == ECuuUserDefined) 
			{
			
#if defined __WINS__ 
    		// In wins emulator mode we need to show emulator-lan access point so we
    		// different technique
			TConnectionPrefs prefs;

    		User::LeaveIfError(iDlgSrv.Connect());

   			prefs.iRank = 1;
    		prefs.iDirection = ECommDbConnectionDirectionOutgoing;
    		prefs.iBearerSet = ECommDbBearerCSD | ECommDbBearerWcdma | ECommDbBearerVirtual;
    
    		TUint32 id = iSettingsData.iAccessPoint;

			TRAPD( err, iDlgSrv.IapConnection( id, prefs, iStatus ) );
    		User::LeaveIfError( err );
	
    		User::WaitForRequest( iStatus );
    		iSettingsData.iAccessPoint = id;
    		iDlgSrv.Close();	

#else		
	   	   CApSettingsHandler *ApUi = CApSettingsHandler::NewLC(
                                                        ETrue, 
                                                        EApSettingsSelListIsPopUp,
                                                        EApSettingsSelMenuSelectNormal,
                                                        KEApIspTypeAll,
                                                        EApBearerTypeAllBearers,
                                                        KEApSortNameAscending,
                                                        EIPv4 | EIPv6
                                                        );
			ApUi->RunSettingsL( id, id );
			CleanupStack::PopAndDestroy(ApUi);	
    		iSettingsData.iAccessPoint = aPUtils->IapIdFromWapIdL(id);
#endif
			}
		else if (result == ECuuAlwaysAsk) 
			{
			iSettingsData.iAccessPoint = -1;
			iSettingsData.iAccessPointDes = KNullDesC;
			iSettingsData.iAccessPointName = KNullDesC;
			}
		}
    

    CleanupStack::PopAndDestroy(3); //commsDb, aPUtils, CConnectionUtilities
  
    }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::GetAccessPointNameL(TInt32 aAP, TDes& aAccessPoint)
// -----------------------------------------------------------------------------
// 
void CRsfwGsPluginSettingsList::GetAccessPointNameL(TInt32 aAP, TDes& aAccessPoint)
    {
       // Fetch CommDB data for a matching IAP Id or IAP Name
    CCommsDatabase* commsDb = CCommsDatabase::NewL();
    CleanupStack::PushL(commsDb);
    CCommsDbTableView* table;
    table = commsDb->OpenViewMatchingUintLC(TPtrC(IAP),
                                                TPtrC(COMMDB_ID),
                                                aAP);
        
    TInt err = table->GotoFirstRecord();
    if (err != KErrNone)
        {
        CleanupStack::PopAndDestroy(2, commsDb); // table, commsDb
        return;
        }

    // Read name for IAP information
    table->ReadTextL(TPtrC(COMMDB_NAME), aAccessPoint);
    CleanupStack::PopAndDestroy(2, commsDb); // table, commsDb
    }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::SetTitlePaneTextL( const TDesC& aTitleText ) const
// -----------------------------------------------------------------------------
// 
void CRsfwGsPluginSettingsList::SetTitlePaneTextL( const TDesC& aTitleText ) const
    {
    DEBUGSTRING(("CRsfwGsPluginSettingsList::SetTitlePaneTextL"));
    CAknTitlePane* title = static_cast< CAknTitlePane* >
        ( iEikonEnv->AppUiFactory()->StatusPane()->ControlL(
                                        TUid::Uid( EEikStatusPaneUidTitle ) ) );
    if ( !title )
        {
        User::Leave( KErrNotSupported );
        }

    title->SetTextL( aTitleText );
    }

// -----------------------------------------------------------------------------
// CRsfwGsPluginSettingsList::ResetItemIndex()
// -----------------------------------------------------------------------------
// 
void CRsfwGsPluginSettingsList::ResetItemIndex()
    {
    ListBox()->SetCurrentItemIndex(NULL);   
    }

// End of File
