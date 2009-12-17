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


#ifndef C_RSFWRFESERVER_H
#define C_RSFWRFESERVER_H

// INCLUDES
#include <e32base.h>
#include <f32file.h>

class CRsfwVolumeTable;

// CONSTANTS
/** Panic Category */
_LIT(KRfeServer, "RemoteFileEngine");

// for security check, same as File Server UID
const TUint KFileServerSecureUid = 0x100039e3;

/** server inactivity timeout, in seconds */
const TInt KRfeServerShutdownInterval = 5; 


// DATA TYPES
/** Remote File Engine panic codes */
enum TRfePanic
    {
    EBadRequest,
    EBadDescriptor,
    ESrvCreateServer,
    ECreateTrapCleanup,
    ENullRequestHandler,
    EUndefinedRequest,
    ECacheInconsistency,
    EConstructingServerStructs
    };

// MACROS

// FORWARD DECLARATIONS
class CRsfwConfig;
class CRsfwRfeServer;


// CLASS DECLARATION
class TRfeEnv
    {
public:
    RFs iFs;
    TInt iCacheDrive; 
    TFileName iCacheRoot;
    CRsfwConfig* iRsfwConfig;
    };


// CLASS DECLARATION

class CRsfwRfeServer: public CPolicyServer
    {
    friend class CRsfwRfeSession;

public:
    static CRsfwRfeServer* NewL();
    static CRsfwRfeServer* NewLC();
    static TInt ThreadFunction(TAny* aNone);

    void IncrementSessions();
    void DecrementSessions();

    void AllEnginesIdling(TInt aTimeout);
    void ServiceRequested();

    static TRfeEnv* Env()
        {
        return iEnvp;
        }

protected:
    TInt RunError(TInt aError);
    
    // custom action when capability checked failed - basically this allows File Server to always
    // pass based on its SID (File Server does not have NetworkServices or ReadDeviceData capabilities)
    TCustomResult CustomFailureActionL(const RMessage2& aMsg, 
                                       TInt aAction, 
                                       const TSecurityInfo& aMissing);

private:
    CRsfwRfeServer(TInt aPriority, TServerType aType) ;
    void ConstructL() ;
    static void PanicClient(const RMessage2& aMessage, TRfePanic aReason);
    static void PanicServer(TRfePanic aReason);
    static void ThreadFunctionL();
    CSession2* NewSessionL(const TVersion &aVersion,
                           const RMessage2& aMessage) const;
    void PrepareCacheRootL();
    void ShutDown();
    void StartDelayedShutdownTimer(TInt aTimeout);
    void StopDelayedShutdownTimer();
    static TInt DelayedShutdownTimerExpired(TAny* aArg);
    
protected:
    CRsfwVolumeTable*              iVolumes;

private:
    TInt                       iSessionCount;
    TRfeEnv                    iEnv;
    static TRfeEnv*            iEnvp;
    TBool                      iShuttingDown;
    CPeriodic*                 iDelayedShutdownTimer;
    };

#endif // RFESERVER_H

// End of File
