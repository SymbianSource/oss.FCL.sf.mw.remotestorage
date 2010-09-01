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


#ifndef CRSFWCONNECTIONMANAGER_H
#define CRSFWCONNECTIONMANAGER_H

// INCLUDES
#include <commdb.h>
#include <rconnmon.h>
#include <es_sock.h>

// FORWARD DECLARATIONS

// DATA TYPES
// Connection event types for MRsfwConnectionObserver
enum TRsfwConnectionObserverEventConnection
    {
    ERsfwConnectionObserverEventConnectionDisconnected = 0,
    ERsfwConnectionObserverEventConnectionWeaklyConnected,
    ERsfwConnectionObserverEventConnectionStronglyConnected
    };

enum TRsfwConnectionManagerConnectionQuality
    {
    ERsfwConnectionQualityNull = 0,
    ERsfwConnectionQualityStrong
    };

// IAP selection algorithms
enum TRsfwConnectionManagerIapSelection
    {
    ERsfwIapSelectionAskUser = 0,            // show the IAP selection menu
    ERsfwIapSelectionUseDefaultPreferences,  // use CommDB preferences
    ERsfwIapSelectionExplicit                // use explicit list
    };

// CLASS DECLARATION
class TIapInfo
    {
public:
    TBuf<KCommsDbSvrMaxColumnNameLength> iName;
    TUint32                              iId;
    TBuf<KCommsDbSvrMaxColumnNameLength> iNetworkName;
    TBuf<KCommsDbSvrMaxColumnNameLength> iServiceName;
    TBuf<KCommsDbSvrMaxColumnNameLength> iSsId;
    TBuf<KCommsDbSvrMaxColumnNameLength> iBearerType;
    TInt                                 iBearerQuality;  // weak/strong
    };

// CLASS DECLARATION
class MRsfwConnectionObserver
    {
public:
    virtual void HandleConnectionEventL(TInt aConnectionEventType,
                                        TAny* aArg) = 0;
    };

// CLASS DECLARATION
// This class manages the link layer for the Rsfw remote access modules.
//
// The primary input is a list of access point names that are assumed
// to be in preference order. However,
// 1) if the list only contains a "*" string,
//    static commdb preference order will be used, or else
// 2) if the list is empty,
//    the user will be prompted for an access point
//
class CRsfwConnectionManager: public CActive
    {
public:
    // Ownership of aIapNames moves to RsfwConnectionManager
    IMPORT_C static CRsfwConnectionManager* NewL(
        MRsfwConnectionObserver* aConnectionObserver);
    IMPORT_C virtual ~CRsfwConnectionManager();
    IMPORT_C void UseIapL(const TDesC& aIap);
    IMPORT_C TInt GetConnection(RSocketServ*& aSocketServ, RConnection*& aConnection);

private:
    CRsfwConnectionManager();
    void ConstructL(MRsfwConnectionObserver* aConnectionObserver);
    TInt LoadIapInfoL(TIapInfo& aIapInfo);
    TInt StartConnection(TUint32 aIapId, TCommDbDialogPref aDialogPreference);
    void HandleDisconnectionEventL();
    
    // functions related to timer for GPRS 'suspend' event
    static TInt SuspensionTimerExpiredL(TAny* aArg);
    void StartSuspensionTimer();
    void StopSuspensionTimer();
    
    
    // from CActive
	void RunL();
	TInt RunError(TInt aError);
	void DoCancel();

private: // Data
    RSocketServ               iSocketServ;
    RConnection               iConnection;
    MRsfwConnectionObserver*  iConnectionObserver;
    TInt                      iIapSelection;        // IAP selection policy
    RArray<TIapInfo>          iIaps;                // allowed IAPs, if any
    TPckgBuf<TNifProgress>    iProgress; 
    CPeriodic*                iSuspensionTimer;     // for GPRS 'suspend' events
    };

#endif // CRSFWCONNECTIONMANAGER_H

// End of File
