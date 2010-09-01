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
* Description:  Remote File Engine session manager
*
*/



// INCLUDE FILES
#include <e32cons.h>
#include <e32std.h>
#include <e32svr.h>
#include <e32std.h>
#include <f32file.h>

#include "rsfwcommon.h"
#include "rsfwinterface.h"
#include "rsfwrfesession.h"
#include "rsfwremoteaccess.h"
#include "mdebug.h"
#include "rsfwrfeserver.h"
#include "rsfwrfemessagerequest.h"
#include "rsfwrequestallocator.h"
#include "rsfwrfeoperation.h"
#include "rsfwrfesyncoperation.h"
#include "rsfwrfeasyncoperation.h"


// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// CRsfwRfeSession::NewL
// ----------------------------------------------------------------------------
//
CRsfwRfeSession* CRsfwRfeSession::NewL(CRsfwRfeServer& aServer)
    {
    CRsfwRfeSession* self = CRsfwRfeSession::NewLC(aServer);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeSession::NewLC
// ----------------------------------------------------------------------------
//
CRsfwRfeSession* CRsfwRfeSession::NewLC(CRsfwRfeServer& aServer)
    {
    // the following debug print crashes easily with USER 24:
 //   DEBUGSTRING(("CRsfwRfeSession::NewLC"));
    CRsfwRfeSession* self;
    self = new (ELeave) CRsfwRfeSession(aServer);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeSession::CRsfwRfeSession
// ----------------------------------------------------------------------------
//
CRsfwRfeSession::CRsfwRfeSession(CRsfwRfeServer& aServer) :
    CSession2(),
    iRfeServer(aServer)
    {
    // Implementation not required
    }

// ----------------------------------------------------------------------------
// CRsfwRfeSession::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwRfeSession::ConstructL()
    {
    iVolumes = iRfeServer.iVolumes;
    iRfeServer.IncrementSessions();
    
    for (int i = 0; i < KDefaultMessageSlots; i++ ) 
        {
        iMessageRequests[i] = NULL;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRfeSession::~CRsfwRfeSession
// ----------------------------------------------------------------------------
//
CRsfwRfeSession::~CRsfwRfeSession()
    {
    // When the sessions are shared we don't know which
    // volumes can be freed at session shutdown 
    iRfeServer.DecrementSessions();
    }
    
// ----------------------------------------------------------------------------
// CRsfwRfeSession::DisconnectL
// ----------------------------------------------------------------------------
//
void CRsfwRfeSession::Disconnect(const RMessage2& aMessage)
    {
    // in all messages pending completion for this session
    // mark the session as NULL so that we do not try to complete them later
    for (int i = 0 ; i <KDefaultMessageSlots ; i++)
        {
        CRsfwRfeMessageRequest* mesrequest = iMessageRequests[i];
    
        if (mesrequest)
            {
            mesrequest->SetSession(NULL);
            }
       
        }
    CSession2::Disconnect(aMessage);   
    }


// ----------------------------------------------------------------------------
// CRsfwRfeSession::ServiceL
// ----------------------------------------------------------------------------
//
void CRsfwRfeSession::ServiceL(const RMessage2& aMessage)
    {  
    DEBUGSTRING16(("CRsfwRfeSession::ServiceL(), function %d", aMessage.Function()));
 
    iRfeServer.ServiceRequested();
   
    if(aMessage.Function() >= EMaxRfeOperations)
        {
        aMessage.Complete(KErrNotSupported);
        return;
        }
        
    CRsfwRfeMessageRequest* pR = NULL;
    pR=RsfwRequestAllocator::GetMessageRequest(aMessage, this);
    if (!pR)
        {
        aMessage.Complete(KErrNoMemory);
        return;
        }
        
    SetToMessageRequestArray(pR);
    
    // if volume was not found and this was a file system request
    // (not a mounting request), we have completed now with
    // KErrNotReady
    if (pR->iMessageCompleted) 
        {
        delete pR;
        return;
        }
    
    CRsfwRfeOperation* pO = NULL;
    pO = GetOperation(pR, aMessage.Function());
    if(!pO)
        { 
        delete pR;
        aMessage.Complete(KErrNoMemory);
        return;
        }
        
    // Request will take ownership of the operation
    pR->SetOperation(pO);   
        
    pR->Dispatch();
    }


// ----------------------------------------------------------------------------
// CRsfwRfeSession::SetToMessageRequestArray
// ----------------------------------------------------------------------------
//
void CRsfwRfeSession::SetToMessageRequestArray(CRsfwRfeMessageRequest* aMessageRequest)
    {  
    // set message request to the message request array
    for (int i = 0; i < KDefaultMessageSlots; i++) 
        {
        if (iMessageRequests[i] == NULL) 
            {
            // completely unused slot
            iMessageRequests[i] = aMessageRequest;
            break;
            }
        }
    
    }


// ----------------------------------------------------------------------------
// CRsfwRfeSession::RemoveFromMessageRequestArray
// call by request prior to self-destruction
// ----------------------------------------------------------------------------
//
void CRsfwRfeSession::RemoveFromMessageRequestArray(CRsfwRfeMessageRequest* aMessageRequest)
    {  
    // set message request to the message request array
    for (int i = 0; i < KDefaultMessageSlots; i++) 
        {
        if (iMessageRequests[i] == aMessageRequest) 
            {
            iMessageRequests[i] = NULL;
            }
        }
    
    }


// ----------------------------------------------------------------------------
// CRsfwRfeSession::Volume
// ----------------------------------------------------------------------------
//
CRsfwVolumeTable* CRsfwRfeSession::Volume() 
    {
    return iVolumes;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeSession::Server
// ----------------------------------------------------------------------------
//
CRsfwRfeServer* CRsfwRfeSession::Server() 
    {
    return &iRfeServer;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeSession::GetOperation
// Sets operations to be synchronous or asynhronous
// ----------------------------------------------------------------------------
//
CRsfwRfeOperation* CRsfwRfeSession::GetOperation(CRsfwRfeRequest* aRequest, TInt aOpCode)
    {
    CRsfwRfeOperation* pO = NULL;
    switch (aOpCode)
        {
    case EDismountByVolumeId:
    case EDismountByDriveLetter:
    case EGetMountList:
    case EGetMountInfo:
    case EFsRoot:
    case EOkToWrite:
    case ESetAttr:
    case EFsIoctl:
    case EDirRefresh:
    case ECancelAll:
        pO = RsfwRequestAllocator::GetSyncOperation(aRequest, aOpCode);
        break;

    case EMount:
    case EMountByDriveLetter:
    case ESetMountConnectionState:
    case ERenameReplace:        
    case EOpenByPath:
    case EFetch:
    case EFetchData:
    case ELookUp:
    case EGetAttr:
    case EMkDir:
    case ERemoveDir:
    case ERemove:
    case ECreateFile:
    case EClose:
    case EFlush:
        pO = RsfwRequestAllocator::GetAsyncOperation(aRequest, aOpCode);
        break;

    default:
       __ASSERT_DEBUG(FALSE, User::Panic(KRfeServer, EUndefinedRequest));
        // if this happens even in release build
        // pO will stay NULL and the request is completed with KErrNoMemory...
        }
   
    return pO; 
    }

// ----------------------------------------------------------------------------
// CRsfwRfeSession::PanicClient
// ----------------------------------------------------------------------------
//
#ifdef NONSHARABLE_SESSION
void CRsfwRfeSession::PanicClient(TInt aPanic) const
    {
    // Note: this panics the client thread, not server
    Panic(KRfeServer, aPanic);
    }
#endif

// End of File
