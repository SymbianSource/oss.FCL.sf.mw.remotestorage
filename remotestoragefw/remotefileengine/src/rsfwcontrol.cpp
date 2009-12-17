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
* Description:  Client side implementation of Remote Storage FW API 
 *              : for control functions such as mounting and unmounting.
 *
*/


// INCLUDE FILES
#include "rsfwcommon.h"
#include "rsfwcontrol.h"


// CONSTANTS

#ifdef __WINS__
const TUint KServerMinHeapSize =   0x1000;  //  4K
const TUint KServerMaxHeapSize = 0x100000;  // 64K
#endif

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// RRsfwControl::RRsfwControl
// C++ default constructor can NOT contain any code, that
// might leave.
// ----------------------------------------------------------------------------
//
EXPORT_C RRsfwControl::RRsfwControl() : RSessionBase()
    {
    }

// ----------------------------------------------------------------------------
// RRRsfwControl::Connect
// Connects to the framework by starting the server if neccessary and creating
// a session.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwControl::Connect()
    {
    const TInt KTryCount = 3;
    
    TInt err;
    TBool retry;
    TInt i = KTryCount;
    do
        {
        err = StartServer(KRfeServerName);
        if (err == KErrNone)
            {
            err = CreateSession(KRfeServerName,
                                Version(),
                                KDefaultMessageSlots);
            }
        retry = ((err == KErrNotFound) || (err == KErrServerTerminated));
        } while (retry && (++i <= KTryCount));

    return err;
    }

// ----------------------------------------------------------------------------
// RRsfwControl::Version
// Returns the version of Remote File Engine
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TVersion RRsfwControl::Version()
    {
    return(TVersion(KRfeMajorVersionNumber,
                    KRfeMinorVersionNumber,
                    KRfeBuildVersionNumber));
    }
    
// ----------------------------------------------------------------------------
// RRsfwControl::Mount
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwControl::Mount(TInt aDriveLetter)
    {
    TIpcArgs args;
    args.Set(0, aDriveLetter);
    return SendRequest(EMountByDriveLetter, args);
    }

// ----------------------------------------------------------------------------
// RRsfwControl::Mount
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwControl::Mount(const TRsfwMountConfig& aMountConfig)
    {
    TPckg<TRsfwMountConfig> pckgMountConfig(aMountConfig);
    return SendRequest(EMount, TIpcArgs(&pckgMountConfig));
    }

// ----------------------------------------------------------------------------
// RRsfwControl::Mount
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwControl::MountBlind(TInt aDriveLetter)
    {
    iArgs.Set(0, aDriveLetter);
    return Send(EMountByDriveLetter, iArgs);
    }

// ----------------------------------------------------------------------------
// RRsfwControl::Mount
// ----------------------------------------------------------------------------
//
EXPORT_C void RRsfwControl::Mount(const TRsfwMountConfig& aMountConfig,
                                  TRequestStatus& aStatus)
    {
    aStatus = KRequestPending;
    iPckgBufMountConfig = aMountConfig;
    SendRequest(EMount, TIpcArgs(&iPckgBufMountConfig), aStatus);
    }
    
// ----------------------------------------------------------------------------
// RRsfwControl::DismountByVolumeId
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwControl::DismountByVolumeId(TInt aVolumeId)
    {
    return SendRequest(EDismountByVolumeId, TIpcArgs(aVolumeId));
    }

// ----------------------------------------------------------------------------
// RRsfwControl::DismountByDriveLetter
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwControl::DismountByDriveLetter(TChar aDriveLetter)
    {
    aDriveLetter.UpperCase();
    return SendRequest(EDismountByDriveLetter, TIpcArgs(aDriveLetter));
    }


// ----------------------------------------------------------------------------
// RRsfwControl::GetMountInfo
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwControl::GetMountInfo(const TChar& aDriveLetter,
                                         TRsfwMountInfo& aMountInfo)
    {
    TPckg<TRsfwMountInfo> pckgMountInfo(aMountInfo);
    SendRequest(EGetMountInfo, TIpcArgs(aDriveLetter, &pckgMountInfo));
    TInt err;
    if (aMountInfo.iMountConfig.iUri.Length())
        {
        err = KErrNone;
        }
    else
        {
        err = KErrNotFound;
        }
    return err;
    }

// ----------------------------------------------------------------------------
// RRsfwControl::SetMountConnectionState
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwControl::SetMountConnectionState(const TChar& aDriveLetter,
                                                    TUint aState)
    {
    // send a blind request
    return Send(ESetMountConnectionState,
                       TIpcArgs(aDriveLetter,
                                aState));
    }

// ----------------------------------------------------------------------------
// RRsfwControl::RefreshDirectoryL
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwControl::RefreshDirectory(const TDesC& aPath)
    {
    return(SendRequest(EDirRefresh, TIpcArgs(&aPath)));
    }

// ----------------------------------------------------------------------------
// RRsfwControl::CancelAllRemoteTransfers
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwControl::CancelRemoteTransfer(const TDesC& aFile)
    {
    // server must be running in order to successfully cancel anything
    return(SendReceive(ECancelAll, TIpcArgs(&aFile)));
    }


// ----------------------------------------------------------------------------
// RRsfwControl::StartServer
// Starts the Remote File Engine if it is not running, uses semaphore to 
// synchronize startup.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
TInt RRsfwControl::StartServer(const TDesC& aServerName)
    {
    TInt err;

    TFindServer findRfe(aServerName);
    TFullName name;

    err = findRfe.Next(name);
    if (err == KErrNone)
        {
        // Server already running
        return KErrNone;
        }

    RSemaphore semaphore;       
    err = semaphore.CreateGlobal(KRfeSemaphoreName, 0);
    if (err != KErrNone)
        {
        return err;
        }

    err = CreateServerProcess(aServerName);
    if (err != KErrNone)
        {
        semaphore.Close();  
        return  err;
        }
    semaphore.Wait();
    semaphore.Close();       

    return  KErrNone;
    }

// ----------------------------------------------------------------------------
// RRRsfwControl::CreateServerProcess
// Starts the Remote File Engine using name to find the binary
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
TInt RRsfwControl::CreateServerProcess(const TDesC& aServerName)
    {
    TInt err;
    
    // just load anything that matches with the name
    const TUidType serverUid(KNullUid, KNullUid, KNullUid);

    RProcess server;

    _LIT(KStartCommand, "");
    err = server.Create(aServerName, KStartCommand, serverUid);
    if (err != KErrNone)
        {
        return err;
        }
    server.Resume();
    server.Close();

    return  KErrNone;
    }



// ----------------------------------------------------------------------------
// RRsfwControl::SendRequest
// ----------------------------------------------------------------------------
//    
TInt RRsfwControl::SendRequest(TInt aOpCode, TIpcArgs aArgs)
    {
    TInt err = SendReceive(aOpCode, aArgs);
    if (err == KErrServerTerminated)
        {
        // Close handle before opening new, otherwise client leaks old handle and panics CONE 36
        Close(); 
        
        // try to restart the server
        err = Connect();
        if (err == KErrNone) 
            {
            err = SendReceive(aOpCode, aArgs);
            }   
        }
    return err;
    }

// ----------------------------------------------------------------------------
// RRsfwControl::SendRequest
// ----------------------------------------------------------------------------
//   
void RRsfwControl::SendRequest(TInt aOpCode,
                               TIpcArgs aArgs,
                               TRequestStatus& aStatus)
    {
    SendReceive(aOpCode, aArgs, aStatus);
    }

//  End of File 
