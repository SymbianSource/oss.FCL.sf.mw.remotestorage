/*
* Copyright (c) 2003-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Remote File Engine server
*
*/



// INCLUDE FILES
#include <e32svr.h>
#include <e32math.h>
#include <e32cons.h>

#include <bacline.h>

#include "rsfwvolumetable.h"
#include "rsfwcommon.h"
#include "rsfwinterface.h"
#include "rsfwrfesession.h"
#include "rsfwrfeserver.h"
#include "mdebug.h"
#include "ecom.h"
#include "rsfwmountstore.h"

#include "rsfwconfig.h"


// ----------------------------------------------------------------------------------------
// Server's policy here
// ----------------------------------------------------------------------------------------

//Total number of ranges
const TUint remoteFileEngineRangeCount = 3;

//Definition of the ranges of IPC numbers
const TInt remoteFileEngineRanges[remoteFileEngineRangeCount] = 
        {
        0, // 0 & 1 ; ERfeRequest, EAsynchRequest
        2, // 2 ; The Control API starts from EMount
        12 // 12 ; The Access API starts from ERenameReplace
        }; 

//Policy to implement for each of the above ranges        
const TUint8 remoteFileEngineElementsIndex[remoteFileEngineRangeCount] = 
        {
        CPolicyServer::EAlwaysPass,	//applies to 0th range
        0,  //applies to 1st range
        1  //applies to 2nd range
        };

//Specific capability checks
const static CPolicyServer::TPolicyElement remoteFileEngineElements[] = 
        {
        // action = -1 ===> failing calls happens via CustomFailureActionL
        // File Server is always allowed based on its SID from that function
        //policy "0" for the Control API; fail call if NetworkServices and ReadDeviceData not present
        {_INIT_SECURITY_POLICY_C2(ECapabilityNetworkServices, ECapabilityReadDeviceData), -1},  
        //policy "1"; for the Access API, fail call if Network Services and AllFiles not prosent
        {_INIT_SECURITY_POLICY_C2(ECapabilityNetworkServices, ECapabilityAllFiles), -1} 
        };

//Package all the above together into a policy
const CPolicyServer::TPolicy remoteFileEnginePolicy =
        {
        CPolicyServer::EAlwaysPass, //specifies all connect attempts should pass
        remoteFileEngineRangeCount,	//number of ranges                                   
        remoteFileEngineRanges,	//ranges array
        remoteFileEngineElementsIndex,	//elements<->ranges index
        remoteFileEngineElements,		//array of elements
        };


// DATA STRUCTURES
TRfeEnv* CRsfwRfeServer::iEnvp;

// ============================ MEMBER FUNCTIONS ==============================


// ----------------------------------------------------------------------------
// CRsfwRfeServer::CRsfwRfeServer
// ----------------------------------------------------------------------------
// 
inline CRsfwRfeServer::CRsfwRfeServer(TInt aPriority, TServerType aType)
    :CPolicyServer(aPriority, remoteFileEnginePolicy, aType)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::NewL
// ----------------------------------------------------------------------------
// 

CRsfwRfeServer* CRsfwRfeServer::NewL()
    {
    CRsfwRfeServer* self = CRsfwRfeServer::NewLC();
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::NewLC
// ----------------------------------------------------------------------------
// 

CRsfwRfeServer* CRsfwRfeServer::NewLC()
    {
    CRsfwRfeServer* self = new (ELeave) CRsfwRfeServer(EPriorityNormal,
                                               ESharableSessions);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::ConstructL
// ----------------------------------------------------------------------------
// 

void CRsfwRfeServer::ConstructL()
    {
    StartL(KRemoteFEName);
    DEBUGSTRING(("registered RFE name 0x%x", this));

    // Prepare the environment
    iEnvp = &iEnv;
    iDelayedShutdownTimer = CPeriodic::NewL(CActive::EPriorityLow);
    User::LeaveIfError(iEnvp->iFs.Connect());
    iEnvp->iRsfwConfig = CRsfwConfig::NewL(KCRUidRsfwCtrl);  
    
	// Make cache root directory
   	PrepareCacheRootL();
   	// Load configuration
   	// Create volume table
   	iVolumes = CRsfwVolumeTable::NewL(this, iEnvp->iRsfwConfig);
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::ThreadFunction
// ----------------------------------------------------------------------------
//
TInt CRsfwRfeServer::ThreadFunction(TAny* /*aNone*/)
    {
    CTrapCleanup* cleanupStack = CTrapCleanup::New();
    if (!cleanupStack)
        {
        PanicServer(ECreateTrapCleanup);
        }

//  __UHEAP_MARK;
    TRAPD(err, ThreadFunctionL());
//  __UHEAP_MARKENDC(0);
    if (err != KErrNone)
        {
        PanicServer(ESrvCreateServer);
        }

    delete cleanupStack;
    cleanupStack = NULL;

    return KErrNone;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::IncrementSessions
// ----------------------------------------------------------------------------
//
void CRsfwRfeServer::IncrementSessions()
    {
    StopDelayedShutdownTimer();
    iSessionCount++;
    DEBUGSTRING(("+session count = %d", iSessionCount));
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::DecrementSessions
// ----------------------------------------------------------------------------
//
void CRsfwRfeServer::DecrementSessions()
    {
    iSessionCount--;
    // this debug output crashes the server for some reason
//    DEBUGSTRING(("-session count = %d", iSessionCount));
    // Note that the event causing server to shut down
    // is not session count going to zero, as 
    // there are "permanent" session(s) from the File Server plugin.
    // (they would be closed when remote drives are unmounted, which never happens)
    // Instead, server shutdown is triggered by last connected volume
    // going to disconnect state, or inactivity timeout expires and
    // there are no open files.
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::AllEnginesIdling
// ----------------------------------------------------------------------------
//
void CRsfwRfeServer::AllEnginesIdling(TInt aTimeout)
    {
    if (!iShuttingDown)
        {
        DEBUGSTRING(("starting to shut down after %d seconds", aTimeout));
        if (aTimeout)
            {
            StartDelayedShutdownTimer(aTimeout);
            }
        else
            {
            ShutDown();
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::ServiceRequested
// ----------------------------------------------------------------------------
//
void CRsfwRfeServer::ServiceRequested()
    {
    StopDelayedShutdownTimer();
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::RunError
// ----------------------------------------------------------------------------
//
TInt CRsfwRfeServer::RunError(TInt aError)
    {
    if (aError == KErrBadDescriptor)
        {
        // A bad descriptor error implies a badly programmed client,
        // so panic it;
        // otherwise report the error to the client
        PanicClient(Message(), EBadDescriptor);
        }
    else
        {
        Message().Complete(aError);
        }

    // The leave will result in an early return from CServer::RunL(), skipping
    // the call to request another message. So do that now in order to keep the
    // server running.
    ReStart();

    return KErrNone;    // handled the error fully
    }


// ----------------------------------------------------------------------------
// CRsfwRfeServer::CustomFailureActionL
// ----------------------------------------------------------------------------
//
CPolicyServer::TCustomResult CRsfwRfeServer::CustomFailureActionL(const RMessage2& aMsg, 
                                                   TInt /* aAction */, 
                                                   const TSecurityInfo& /*aMissing */)
    {
    TCustomResult result = EFail;
    TSecureId secId = aMsg.SecureId();
    if (secId = KFileServerSecureUid) 
        {
        result = EPass;
        }
    return result;
    }



// ----------------------------------------------------------------------------
// CRsfwRfeServer::PanicClient
// ----------------------------------------------------------------------------
//
void CRsfwRfeServer::PanicClient(const RMessage2& aMessage, TRfePanic aPanic)
    {
    aMessage.Panic(KRfeServer, aPanic);
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::PanicServer
// ----------------------------------------------------------------------------
//
void CRsfwRfeServer::PanicServer(TRfePanic aPanic)
    {
    User::Panic(KRfeServer, aPanic);
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::ThreadFunctionL
// ----------------------------------------------------------------------------
//
void CRsfwRfeServer::ThreadFunctionL()
    {

    // Construct active scheduler
    CActiveScheduler* activeScheduler = new (ELeave) CActiveScheduler;
    CleanupStack::PushL(activeScheduler);

    // Install active scheduler.
    // We don't need to check whether an active scheduler is already installed
    // as this is a new thread, so there won't be one
    CActiveScheduler::Install(activeScheduler);

    // Change the name of the thread, so it is easier to recognize
    User::RenameThread(KRfeMain);
    // Construct our server
    CRsfwRfeServer::NewLC();    // anonymous

    RSemaphore semaphore;
    TInt err;
    err = semaphore.OpenGlobal(KRfeSemaphoreName);
    if (err == KErrNotFound)
        {
        err = semaphore.CreateGlobal(KRfeSemaphoreName, 0);
        }
    User::LeaveIfError(err);

    // Semaphore opened ok
    semaphore.Signal();
    semaphore.Close();

#ifdef _DEBUG
    {
    TInt8* p = (TInt8*)User::Alloc(1);
    DEBUGSTRING(("Test alloc addr=0x%x", p));
    delete p;
    DEBUGSTRING(("Enter alloc count=%d", User::CountAllocCells()));
    TInt b;
    TInt a = User::Available(b);
    DEBUGSTRING(("Enter alloc avail=%d, biggest=%d", a, b));
    }
#endif

    // Start handling requests
    CActiveScheduler::Start();
#ifdef _DEBUG
    {
    DEBUGSTRING(("Exit alloc count=%d", User::CountAllocCells()));
    TInt b;
    TInt a = User::Available(b);
    DEBUGSTRING(("Exit alloc avail=%d, biggest=%d", a, b));
    }
#endif
    CleanupStack::PopAndDestroy(2, activeScheduler);
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::NewSessionL
// ----------------------------------------------------------------------------
//     
CSession2* CRsfwRfeServer::NewSessionL(const TVersion &aVersion,
                                   const RMessage2&) const
    {
    // Check we're the right version
    if (!User::QueryVersionSupported(TVersion(KRfeMajorVersionNumber,
                                              KRfeMinorVersionNumber,
                                              KRfeBuildVersionNumber),
                                     aVersion))
        {
        User::Leave(KErrNotSupported);
        }
    // Make new session
    return CRsfwRfeSession::NewL(*const_cast<CRsfwRfeServer*> (this));
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::PrepareCacheRootL
// Get the cache path and create the directory if it does not exist
// ----------------------------------------------------------------------------
//   
void CRsfwRfeServer::PrepareCacheRootL()
    {
    TInt err;
    err = iEnvp->iRsfwConfig->Get(RsfwConfigKeys::KCacheDirectoryPath,
                                  iEnvp->iCacheRoot);
    if (err == KErrNone)
        {
        TBuf<KMaxRsfwConfItemLength> driveString;
        if ((iEnvp->iCacheRoot.Length() < 2) || (iEnvp->iCacheRoot[1] != ':'))
            {
            err = iEnvp->iRsfwConfig->Get(RsfwConfigKeys::KRsfwDefaultDrive,
                                          driveString);
            if (err != KErrNone)
                {
                driveString.Copy(KRSFWDefaultDrive);
                }
            }
        if (driveString.Length() < 2)
            {
            driveString.Append(':');
            }
        iEnvp->iCacheRoot.Insert(0, driveString);
        }
    else
        {
        HBufC* defaultcacheRoot = HBufC::NewL(KMaxPath);
        TPtr defaultCache(defaultcacheRoot->Des());
        defaultCache.Append(KRSFWDefaultDrive);
        defaultCache.Append(KCacheRootDefault);
        iEnvp->iCacheRoot.Copy(defaultCache);
        delete defaultcacheRoot;
        }
    RFs& fs = iEnvp->iFs;
    TUint att;
    TChar cacheDriveChar = iEnvp->iCacheRoot[0];
    User::LeaveIfError(fs.CharToDrive(cacheDriveChar, iEnvp->iCacheDrive));
    err = fs.Att(iEnvp->iCacheRoot, att);
    if (err != KErrNone)
        {
        // There was no prior cache root
        err = fs.MkDirAll(iEnvp->iCacheRoot);
        DEBUGSTRING(("Cache root creation failed with err=%d", err));
        User::LeaveIfError(err);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::ShutDown
// ----------------------------------------------------------------------------
//
void CRsfwRfeServer::ShutDown()
    {
    DEBUGSTRING(("shutting down"));
    iShuttingDown = ETrue;
    CActiveScheduler::Stop();
    delete iVolumes;
    iVolumes = NULL;
    delete iEnvp->iRsfwConfig;
    iEnvp->iRsfwConfig = NULL;
    iEnvp->iFs.Close();
    delete iDelayedShutdownTimer;
    iDelayedShutdownTimer = NULL;
    // REComSession::FinalClose must be called when everything else
    // related to ECom use has been deleted
    REComSession::FinalClose(); 
    DEBUGSTRING(("shut down"));
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::StartDelayedShutdownTimer
// ----------------------------------------------------------------------------
//
void CRsfwRfeServer::StartDelayedShutdownTimer(TInt aTimeout)
    {
    if (!iShuttingDown)
        {
        iDelayedShutdownTimer->Cancel();
        DEBUGSTRING(("shutting down in %d seconds",
                     aTimeout));
        TCallBack callBack(CRsfwRfeServer::DelayedShutdownTimerExpired, this);
        iDelayedShutdownTimer->Start(aTimeout * 1000000,
                                     aTimeout * 1000000,
                                     callBack);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::StopDelayedShutdownTimer
// ----------------------------------------------------------------------------
//
void CRsfwRfeServer::StopDelayedShutdownTimer()
    {
    if (iDelayedShutdownTimer)
        {
        iDelayedShutdownTimer->Cancel();
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRfeServer::DelayedShutdownTimerExpired
// ----------------------------------------------------------------------------
//
TInt CRsfwRfeServer::DelayedShutdownTimerExpired(TAny* aArg)
    {   
    CRsfwRfeServer* rfeServer = static_cast<CRsfwRfeServer*>(aArg);
    rfeServer->ShutDown();
    return 0;
    }

// ----------------------------------------------------------------------------
// E32Main
// ----------------------------------------------------------------------------
//
TInt E32Main()
    {
    return CRsfwRfeServer::ThreadFunction(NULL);
    }


// End of File
