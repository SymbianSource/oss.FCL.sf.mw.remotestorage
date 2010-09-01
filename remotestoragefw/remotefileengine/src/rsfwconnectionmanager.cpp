/*
* Copyright (c) 2002-2004 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Connection manager
 *
*/


// Copyright (C) 2002-2004 Nokia

// INCLUDE FILES
#include <commdbconnpref.h>
#include <es_enum.h>
#include "rsfwconnectionmanager.h"
#include "rsfwcommon.h"
#include "mdebug.h"

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::NewL
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwConnectionManager* CRsfwConnectionManager::NewL(
    MRsfwConnectionObserver* aConnectionObserver)
    {
    DEBUGSTRING(("CRsfwConnectionManager::NewL"));
    CRsfwConnectionManager* self = new (ELeave) CRsfwConnectionManager();
    CleanupStack::PushL(self);
    self->ConstructL(aConnectionObserver);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::CRsfwConnectionManager
// ----------------------------------------------------------------------------
//
CRsfwConnectionManager::CRsfwConnectionManager()
: CActive( EPriorityStandard )
    {
    }

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwConnectionManager::ConstructL(
    MRsfwConnectionObserver* aConnectionObserver)
    {
    DEBUGSTRING(("CRsfwConnectionManager::ConstructL"));
    iConnectionObserver = aConnectionObserver;
    // Connect to the socket server
    User::LeaveIfError(iSocketServ.Connect());
    // Add this to active scheduler
    CActiveScheduler::Add( this );
    
    iSuspensionTimer = CPeriodic::NewL(CActive::EPriorityLow);
    }

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::~CRsfwConnectionManager
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwConnectionManager::~CRsfwConnectionManager()
    {
     DEBUGSTRING(("CRsfwConnectionManager::~CRsfwConnectionManager"));
    Cancel();
    iConnection.Close();
    iSocketServ.Close();
    iIaps.Close();
    
    StopSuspensionTimer();
    delete iSuspensionTimer;    
    }

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::UseIapL
// ----------------------------------------------------------------------------
//
EXPORT_C void CRsfwConnectionManager::UseIapL(const TDesC& aIap)
    {
    // aIap may contain an IAP name or an IAP Id (or '?'/'*')
    DEBUGSTRING(("IAP: %S", &aIap));

    // Determine IAP selection policy
    // By default, ask the user
    iIapSelection = ERsfwIapSelectionAskUser;
    if (aIap.CompareF(KIapDefaultPreferences) == 0)
        {
        // Use static CommDB preferences
        iIapSelection = ERsfwIapSelectionUseDefaultPreferences;
        }
    else if (aIap.CompareF(KIapAskUser) == 0)
        {
        // Ask the user
        }
    else
        {
        // Build a table of acceptable IAPs.
        // Now the table only contains one entry
        TIapInfo iapInfo;
        iapInfo.iId = 0;
        iapInfo.iName.SetLength(0);
        // try to retrieve ID or name based on aIap
        TLex iapLex(aIap);
        TInt err = iapLex.Val(iapInfo.iId, EDecimal);
        if (err != KErrNone)
            {
            // The IAP name was given
            iapInfo.iName.Copy(aIap);
            }
        if (LoadIapInfoL(iapInfo) == KErrNone)
            {
            iIaps.Append(iapInfo);
            iIapSelection = ERsfwIapSelectionExplicit;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::GetConnection
// ----------------------------------------------------------------------------
//
EXPORT_C TInt CRsfwConnectionManager::GetConnection(RSocketServ*& aSocketServ,
                                                    RConnection*& aConnection)
    {
    DEBUGSTRING(("Get connection"));
    TInt err = iConnection.Open(iSocketServ);
    if (err == KErrNone)
        {
        TUint32 iapId = 0;
        TCommDbDialogPref dialogPreference = ECommDbDialogPrefDoNotPrompt;
        switch (iIapSelection)
            {
        case ERsfwIapSelectionAskUser:
            iapId = 0;
            dialogPreference = ECommDbDialogPrefPrompt;
            break;

        case ERsfwIapSelectionUseDefaultPreferences:
            break;

        case ERsfwIapSelectionExplicit:
            iapId = iIaps[0].iId;
            dialogPreference = ECommDbDialogPrefDoNotPrompt;
            break;
            
        default:
            break;
            }

        err = StartConnection(iapId, dialogPreference);
        if (err == KErrNone)
            {
            aSocketServ = &iSocketServ;
            aConnection = &iConnection;
            }
        }            
    DEBUGSTRING(("Get connection returning %d", err));    
    return err;
    }

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::LoadIapInfoL
// ----------------------------------------------------------------------------
//
TInt CRsfwConnectionManager::LoadIapInfoL(TIapInfo& aIapInfo)
    {
    DEBUGSTRING(("CRsfwConnectionManager::LoadIapInfoL"));
    // Fetch CommDB data for a matching IAP Id or IAP Name
    CCommsDatabase* commsDb = CCommsDatabase::NewL();
    CleanupStack::PushL(commsDb);
    CCommsDbTableView* table;
    if (aIapInfo.iId)
        {
        table = commsDb->OpenViewMatchingUintLC(TPtrC(IAP),
                                                TPtrC(COMMDB_ID),
                                                aIapInfo.iId);
        }
    else
        {
        table = commsDb->OpenViewMatchingTextLC(TPtrC(IAP),
                                                TPtrC(COMMDB_NAME),
                                                aIapInfo.iName);
        }
    TInt err = table->GotoFirstRecord();
    if (err != KErrNone)
        {
        DEBUGSTRING16(("Could not find IAP '%S' (id=%d)!",
                       &aIapInfo.iName,
                       aIapInfo.iId));
        CleanupStack::PopAndDestroy(2, commsDb); // table, commsDb
        return KErrNotFound;
        }

    // Read IAP information
    table->ReadUintL(TPtrC(COMMDB_ID), aIapInfo.iId);
    table->ReadTextL(TPtrC(COMMDB_NAME), aIapInfo.iName);
    table->ReadTextL(TPtrC(IAP_BEARER_TYPE), aIapInfo.iBearerType);
    TBuf<KCommsDbSvrMaxColumnNameLength> serviceType;
    table->ReadTextL(TPtrC(IAP_SERVICE_TYPE), serviceType);
    TUint32 service;
    table->ReadUintL(TPtrC(IAP_SERVICE), service);
    // Find out the network
    TUint32 networkId;
    table->ReadUintL(TPtrC(IAP_NETWORK), networkId);
    CleanupStack::PopAndDestroy(table); // table

    table = commsDb->OpenViewMatchingUintLC(TPtrC(NETWORK),
                                            TPtrC(COMMDB_ID),
                                            networkId);
    err = table->GotoFirstRecord();
    if (err == KErrNone)
        {
        table->ReadTextL(TPtrC(COMMDB_NAME), aIapInfo.iNetworkName);
        }
    else
        {
        DEBUGSTRING(("Could not find network for the IAP!"));
        }
    CleanupStack::PopAndDestroy(table); // table

    aIapInfo.iServiceName.Zero();
    aIapInfo.iSsId.Zero();
      
    CleanupStack::PopAndDestroy(commsDb); // commsDb

       
    aIapInfo.iBearerQuality = ERsfwConnectionQualityStrong;
       
    DEBUGSTRING16(("found IAP %S: id=%d, servicetype=%S, service=%d, network=%S, servicename=%S, ssid=%S, bearer=%S, quality=%d",
                   &aIapInfo.iName,
                   aIapInfo.iId,
                   &serviceType,
                   service,
                   &aIapInfo.iNetworkName,
                   &aIapInfo.iServiceName,
                   &aIapInfo.iSsId,
                   &aIapInfo.iBearerType,
                   aIapInfo.iBearerQuality));

    return KErrNone;
    }

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::StartConnection
// ----------------------------------------------------------------------------
//
TInt CRsfwConnectionManager::StartConnection(TUint32 aIapId,
                                             TCommDbDialogPref aDialogPreference)
    {
    DEBUGSTRING(("CRsfwConnectionManager::StartConnection with id %d", &aIapId));
    TCommDbConnPref connectionPref;
    connectionPref.SetIapId(aIapId);
    connectionPref.SetDialogPreference(aDialogPreference);
    connectionPref.SetDirection(ECommDbConnectionDirectionOutgoing);
    DEBUGSTRING(("Starting connection to IAP %d with pref %d",
                 aIapId,
                 aDialogPreference));
    TInt err = iConnection.Start(connectionPref);
    DEBUGSTRING(("Connection starting returned %d", err));
    // start observing the connection, any events will occur in RunL() function
    if ( !IsActive() )
        {
        iConnection.ProgressNotification(iProgress, iStatus);
        SetActive();
        }                   
    else
        {
        DEBUGSTRING(("StartConnection called twice!"));
        Cancel();
                
        DEBUGSTRING(("StartConnection Cancel!"));

        iConnection.ProgressNotification(iProgress, iStatus);
        DEBUGSTRING(("CRsfwConnectionManager iConnection.ProgressNotification!"));

        SetActive();
       
        }
    return err;
    }

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::HandleDisconnectionEventL
// ----------------------------------------------------------------------------
//
void CRsfwConnectionManager::HandleDisconnectionEventL()
    {
    DEBUGSTRING(("CRsfwConnectionManager::HandleDisconnectionEventL"));
    if (iConnectionObserver)
        {
        iConnectionObserver->HandleConnectionEventL(
            ERsfwConnectionObserverEventConnectionDisconnected,
            NULL);
        }    
    }
    
// ----------------------------------------------------------------------------
// CRsfwConnectionManager::StartSuspensionTimer
// ----------------------------------------------------------------------------
//
void CRsfwConnectionManager::StartSuspensionTimer()
    {
    DEBUGSTRING(("CRsfwConnectionManager::StartSuspensionTimer"));
    if (iSuspensionTimer)
        {
        const TInt KRsfwGPRSSuspensionTimeout = 60 * 1000000; // 60 sec
        
        DEBUGSTRING(("GPRS suspension timer started (%d us)",
                     KRsfwGPRSSuspensionTimeout));
        iSuspensionTimer->Cancel();
        TCallBack callBack(CRsfwConnectionManager::SuspensionTimerExpiredL, this);
        iSuspensionTimer->Start(KRsfwGPRSSuspensionTimeout,
                                KRsfwGPRSSuspensionTimeout,
                                callBack);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::StopSuspensionTimer
// ----------------------------------------------------------------------------
//
void CRsfwConnectionManager::StopSuspensionTimer()
    {
    DEBUGSTRING(("CRsfwConnectionManager::StopSuspensionTimer"));
    if (iSuspensionTimer)
        {
        DEBUGSTRING(("GPRS suspension timer stopped"));
        iSuspensionTimer->Cancel();
        }
    }
    
// ----------------------------------------------------------------------------
// CRsfwConnectionManager::SuspensionTimerExpired
// ----------------------------------------------------------------------------
//
TInt CRsfwConnectionManager::SuspensionTimerExpiredL(TAny* aArg)
    {
    DEBUGSTRING(("GPRS suspension timer expired"));
    CRsfwConnectionManager* connMan = static_cast<CRsfwConnectionManager*>(aArg);
    connMan->StopSuspensionTimer();
    connMan->HandleDisconnectionEventL();
    return KErrNone;
    }

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::RunL
// ----------------------------------------------------------------------------
//
void CRsfwConnectionManager::RunL()
    {
    TInt status = iStatus.Int();
    DEBUGSTRING(("CRsfwConnectionManager::RunL %d", &status));
    TInt stage = iProgress().iStage;
    TInt error = iProgress().iError;
    DEBUGSTRING(("ConnectionManager::RunL - status: %d, stage: %d, error: %d", status, stage, error));
    
    if ( error == KErrConnectionTerminated || error == KErrDisconnected 
        || stage == KLinkLayerClosed || stage == KConnectionClosed )
        {
        // KErrDisconnected occurs if WLAN goes out of range
        // KErrConnectionTerminated occurs if user cancels connection from "Active connections" menu
        // stage values KLinkLayerClosed & KConnectionClosed should be generated by GPRS 
        // (however GPRS usually generates KDataTransferTemporarilyBlocked event)
        HandleDisconnectionEventL();
        }
    else if ( stage == KDataTransferTemporarilyBlocked )
        {
        // KDataTransferTemporarilyBlocked means GPRS 'suspend' event
        // start timer, when it expires we will disconnect
        StartSuspensionTimer();
        }
    else if ( stage == KLinkLayerOpen )
        {
        // KLinkLayerOpen may mean GPRS 'resume' event
        StopSuspensionTimer();
        }
    else 
        {
        // ignore the event
        }
    
    // request new events if necessary
    iConnection.ProgressNotification(iProgress, iStatus, KConnProgressDefault);
    SetActive();
	}

// ----------------------------------------------------------------------------
// CRsfwConnectionManager::RunError
// ----------------------------------------------------------------------------
//
#ifdef _DEBUG
TInt CRsfwConnectionManager::RunError(TInt aError)
#else
TInt CRsfwConnectionManager::RunError(TInt /*aError*/)
#endif
    {
    DEBUGSTRING(("ConnectionManager::RunErrorL - error: %d", aError));
    return KErrNone;
	}
	
// ----------------------------------------------------------------------------
// CRsfwConnectionManager::DoCancel
// ----------------------------------------------------------------------------
//
void CRsfwConnectionManager::DoCancel()
    {
    DEBUGSTRING(("CRsfwConnectionManager::::DoCancel()"));
    //cancel request issued by ProgressNotification()
    iConnection.CancelProgressNotification();
    }
// End of File
