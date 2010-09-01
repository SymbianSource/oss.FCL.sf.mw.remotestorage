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
* Description:  Synchronous layer on top of the Access Protocol plug-in API
 *
*/


// INCLUDE FILES
#include "rsfwremoteaccesssync.h"


// ============================ MEMBER FUNCTIONS ==============================

CRsfwRemoteAccessSync* CRsfwRemoteAccessSync::NewL(const TDesC8& aService,
                                           CRsfwRemoteAccess* aRemoteAccess)
    {
    CRsfwRemoteAccessSync* self = new (ELeave) CRsfwRemoteAccessSync;
    CleanupStack::PushL(self);
    self->ConstructL(aService, aRemoteAccess);
    CleanupStack::Pop(self);
    return self;
    }

void CRsfwRemoteAccessSync::ConstructL(const TDesC8& aService,
                                   CRsfwRemoteAccess* aRemoteAccess)
    {
    if (aRemoteAccess)
        {
        iRemoteAccess = aRemoteAccess;
        }
    else
        {
        iRemoteAccess = CRsfwRemoteAccess::NewL(aService);
        iOwnRemoteAccess = ETrue;
        }
    iSchedulerWait = new CActiveSchedulerWait();   
    }

CRsfwRemoteAccessSync::~CRsfwRemoteAccessSync()
    {
    if (iOwnRemoteAccess)
        {
        delete iRemoteAccess;
        }
    delete iSchedulerWait;    
    }

TInt CRsfwRemoteAccessSync::Setup(
    MRsfwRemoteAccessObserver* aRsfwRemoteAccessObserver)
    {
    TRAPD(err, iRemoteAccess->SetupL(aRsfwRemoteAccessObserver));
    return err;
    }

TInt CRsfwRemoteAccessSync::Open(const TDesC& aUserName,
                             const TDesC& aPassword,
                             const TDesC& aServerName,
                             TInt aPortNumber,
                             const TDesC& aRootDirectory,
                             const TDesC& aAuxData)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TBuf<KMaxPath> url;
    url.Append(_L("http://"));
    url.Append(aServerName);
    url.Append(_L(":"));
    url.AppendNum(aPortNumber);
    url.Append(_L("/"));
    url.Append(aRootDirectory);
    
    
    TUriParser uriParser;
    uriParser.Parse(url);
    
    TRAP(iStatus, iRemoteAccess->OpenL(uriParser,
                                       aServerName,
                                       aUserName,
                                       aPassword,
                                       aAuxData,
                                       this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::GetDirectory(const TDesC& aPathName,
                                     RPointerArray<CRsfwDirEnt>& aDirEnts)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->GetDirectoryL(aPathName,
                                               aDirEnts,
                                               this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::GetFile(const TDesC& aRemotePathName,
                                const TDesC& aLocalPathName,
                                TInt aOffset,
                                TInt* aLength,
                                TUint aFlags)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->GetFileL(aRemotePathName,
                                          aLocalPathName,
                                          aOffset,
                                          aLength,
                                          aFlags,
                                          this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::MakeDirectory(const TDesC& aPathName)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->MakeDirectoryL(aPathName, this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::CreateFile(const TDesC& aPathName)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->CreateFileL(aPathName, 0, this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::PutFile(const TDesC& aLocalPathName,
                                const TDesC& aRemotePathName)
    {
    _LIT8(KTextPlain, "text/plain");
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->PutFileL(aLocalPathName,
                                          aRemotePathName,
                                          KTextPlain,
                                          this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::DeleteDirectory(const TDesC& aPathName)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->DeleteDirectoryL(aPathName, this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::DeleteFile(const TDesC& aPathName)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->DeleteFileL(aPathName, this));
    return Epilog();
    }


TInt CRsfwRemoteAccessSync::Rename(const TDesC& aSrcPathName,
                               const TDesC& aDstPathName,
                               TBool aOverwrite)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->RenameL(aSrcPathName,
                                         aDstPathName,
                                         aOverwrite,
                                         this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::GetDirectoryAttributes(const TDesC& aPathName,
                                               CRsfwDirEntAttr*& aAttr)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus,
         iRemoteAccess->GetDirectoryAttributesL(aPathName, aAttr, this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::GetFileAttributes(const TDesC& aPathName,
                                          CRsfwDirEntAttr*& aAttr)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->GetFileAttributesL(aPathName,
                                                    aAttr,
                                                    this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::SetAttributes(const TDesC& aPathName,
                                      CRsfwDirEntAttr& aAttr)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->SetAttributesL(aPathName, aAttr, this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::ObtainLock(const TDesC& aPathName,
                                   TUint aLockFlags,
                                   TUint& aTimeout,
                                   TDesC8*& aLockToken)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus,
         iRemoteAccess->ObtainLockL(aPathName,
                                    aLockFlags,
                                    aTimeout,
                                    aLockToken,
                                    this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::ReleaseLock(const TDesC& aPathName)
    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->ReleaseLockL(aPathName, this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::RefreshLock(const TDesC& aPathName,
                                    TUint& aTimeout)

    {
    if (iPending)
        {
        return KErrServerBusy;
        }
    iPending = ETrue;
    TRAP(iStatus, iRemoteAccess->RefreshLockL(aPathName, aTimeout, this));
    return Epilog();
    }

TInt CRsfwRemoteAccessSync::SetLockToken(const TDesC& aPathName,
                                     const TDesC8& aLockToken)
    {
    return iRemoteAccess->SetLockToken(aPathName, aLockToken);
    }

// -----------------------------------------------------------------
// from MRemoteAccessResponseHandler
// -----------------------------------------------------------------

void CRsfwRemoteAccessSync::HandleRemoteAccessResponse(TUint /* aId */,
                                                   TInt aStatus)
    {
    iStatus = aStatus;
    if (iSchedulerWait->IsStarted())
        {
        iSchedulerWait->AsyncStop();
        }
    iPending = EFalse;
    }

// -----------------------------------------------------------------

TInt CRsfwRemoteAccessSync::Epilog()
    {
    if (iPending && (iStatus == KErrNone))
        {
        iSchedulerWait->Start();
        }
    return iStatus;
    }

// End of File
