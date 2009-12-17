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
* Description:  Implements remote access plugin API using WebDAV protocol
 *
*/


// INCLUDE FILES
#include "rsfwdavaccess.h"
#include "rsfwdavfileinfo.h"
#include "rsfwdavaccesscontext.h"
#include "rsfwdavtransaction.h"
#include "mdebug.h"


// ============================ MEMBER FUNCTIONS ==============================

void CRsfwDavAccess::ConstructL()
    {
    }

CRsfwDavAccess* CRsfwDavAccess::NewL()
    {
    CRsfwDavAccess* self = new (ELeave) CRsfwDavAccess;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

CRsfwDavAccess::~CRsfwDavAccess()
    {
    delete iWebDavSession;
    iDavFileInfos.ResetAndDestroy();
    iDavAccessContexts.ResetAndDestroy(); 
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::DavFileInfoL
// Find the file info for the given file name.
// ----------------------------------------------------------------------------
//
CRsfwDavFileInfo* CRsfwDavAccess::DavFileInfoL(const TDesC& aName)
    {
    TInt index = DavFileInfoIndexL(aName);
    if (index != KErrNotFound)
        {
        return iDavFileInfos[index];
        }
    return NULL;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::AddDavFileInfo
// Add a new file info entry.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccess::AddDavFileInfo(CRsfwDavFileInfo* aDavFileInfo)
    {
#ifdef _DEBUG
    TPtrC namePtr;
    if (aDavFileInfo->Name())
        {
        namePtr.Set(*aDavFileInfo->Name());
        }
    TPtrC8 lockPtr;
    if (aDavFileInfo->LockToken())
        {
        lockPtr.Set(*aDavFileInfo->LockToken());
        }
    DEBUGSTRING16(("Add file info: name='%S'", &namePtr));
    DEBUGSTRING8(("               lock='%S', time=%d",
                  &lockPtr,
                  aDavFileInfo->Timeout()));
    
#endif // DEBUG
    iDavFileInfos.Append(aDavFileInfo);
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::RemoveDavFileInfoL
// Remove a file info entry.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccess::RemoveDavFileInfoL(const TDesC& aPath)
    {
    TInt index = DavFileInfoIndexL(aPath);
    if (index != KErrNotFound)
        {
        CRsfwDavFileInfo* davFileInfo =  iDavFileInfos[index];
#ifdef _DEBUG
        TPtrC namePtr;
        if (davFileInfo->Name())
            {
            namePtr.Set(*davFileInfo->Name());
            }
        DEBUGSTRING16(("Remove file info: name='%S'", &namePtr));
#endif // DEBUG
        iDavFileInfos.Remove(index);
        delete davFileInfo;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::SetupL
// Setup - should be immediately followed by NewL() call.
// ----------------------------------------------------------------------------
//
void CRsfwDavAccess::SetupL(MRsfwRemoteAccessObserver* aRsfwRemoteAccessObserver)
    {
    DEBUGSTRING(("DAV: SetupL"));
    MRsfwConnectionObserver* rsfwConnectionObserver = NULL;
    if (aRsfwRemoteAccessObserver)
        {
        // Cascade remote access observers
        iRsfwRemoteAccessObserver = aRsfwRemoteAccessObserver;
        rsfwConnectionObserver = this;
        }
    iWebDavSession = CRsfwDavSession::NewL(this, rsfwConnectionObserver);
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::OpenL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::OpenL(const TUriC& aUri,
                        const TDesC& /*aFriendlyName*/,
                        const TDesC& aUserName,
                        const TDesC& aPassword,
                        const TDesC& aAuxData,
                        MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    if (!aResponseHandler)
        {
        User::Leave(KErrArgument);
        }

    HBufC* url = HBufC::NewLC(KHttpsScheme().Length() +
                              KMaxServerNameLen +
                              1 +
                              KMaxPath);
    TPtr urlPtr = url->Des();

    TInt portNumber = 0;
    if (aUri.IsPresent(EUriPort))
        {
        TLex portx(aUri.Extract(EUriPort));
        if (portx.Val(portNumber) != KErrNone)
            {
            portNumber = 0;
            }
        }

    // Check scheme and map it to port number or vice versa
    TPtrC scheme;
    if (aUri.IsPresent(EUriScheme))
        {
        scheme.Set(aUri.Extract(EUriScheme));
        }
    if (scheme.Length())
        {
        if (portNumber == 0)
            {
            if (scheme.CompareF(KHttpsScheme) == 0)
                {
                portNumber = KHttpsPortNumber;
                }
            else
                {
                portNumber = KHttpPortNumber;
                }
            }
        }
    else
        {
        if (portNumber == 0)
            {
            portNumber = KHttpPortNumber;
            }
        if (portNumber == KHttpPortNumber)
            {
            scheme.Set(KHttpScheme);
            }
        else if (portNumber == KHttpsPortNumber)
            {
            scheme.Set(KHttpsScheme);
            }
        else
            {
            User::Leave(KErrBadName);
            }
        }
        
    TPtrC rootDirectory;
    if (aUri.IsPresent(EUriPath))
        {
        rootDirectory.Set(aUri.Extract(EUriPath));
        }
    iRootDirectory.Copy(rootDirectory);
    if (!iRootDirectory.Length() ||
        iRootDirectory[iRootDirectory.Length() - 1] != '/')
        {
        // Append trailing '/'
        iRootDirectory.Append('/');
        }
    
    urlPtr.Copy(scheme);
    urlPtr.Append(':');
    urlPtr.Append('/');
    urlPtr.Append('/');
    urlPtr.Append(aUri.Extract(EUriHost));
    // There needs to be a slash between server name and the root dir
    // (we assume that there cannot be an excess of slash characters)
    if (urlPtr[urlPtr.Length() - 1] != '/')
        {
        if (!iRootDirectory.Length() || (iRootDirectory[0] != '/'))
            {
            urlPtr.Append('/');
            }
        }
    urlPtr.Append(iRootDirectory);
    
    DEBUGSTRING16(("DAV: OpenL to URL '%S'", &urlPtr));
    
    iWebDavSession->OpenL(urlPtr,
                          portNumber,
                          aUserName,
                          aPassword,
                          aAuxData);
    CleanupStack::PopAndDestroy(url);
    return OptionsL(aResponseHandler);
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::GetDirectoryL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::GetDirectoryL(const TDesC& aPathName,
                                RPointerArray<CRsfwDirEnt>& aDirEnts,
                                MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    // Get the contents of the directory
    DEBUGSTRING16(("DAV: GetDirectory '%S'", &aPathName));

    // check that arguments are sensible
    // aPathName might here be null (mounted root directory)
    if (aPathName.Length() > KMaxPath)
        {
        User::Leave(KErrBadName);
        }

    if (!aResponseHandler)
        {
        User::Leave(KErrArgument);
        }

    CRsfwDavAccessContextPropFindDir* davAccessContextPropFindDir =
        CRsfwDavAccessContextPropFindDir::NewL(this,
                                           aResponseHandler,
                                           aPathName,
                                           1,
                                           NULL,
                                           &aDirEnts);
    TUint id = AddAccessContext(davAccessContextPropFindDir);
    davAccessContextPropFindDir->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::GetFileL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::GetFileL(const TDesC& aRemotePathName,
                           const TDesC& aLocalPathName,
                           TInt aOffset,
                           TInt* aLength,
                           TUint aFlags,
                           MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {

#ifdef _DEBUG
    {
    TInt length;
    if (aLength)
        {
        length = *aLength;
        }
    else
        {
        length = 0;
        }
    DEBUGSTRING16(("DAV: GetFile rn='%S', ln='%S' (off=%d, len=%d)",
                   &aRemotePathName,
                   &aLocalPathName,
                   aOffset,
                   length));
    }
#endif // DEBUG

    // check that arguments are sensible
    if (aOffset < 0 ||
       (aLength && *aLength < 0) ||
       (!aResponseHandler))
        {
        User::Leave(KErrArgument);
        }

    if ((aLocalPathName.Length() == 0) ||
        (aLocalPathName.Length() > KMaxPath) ||
        (aRemotePathName.Length() == 0) ||
        (aRemotePathName.Length() > KMaxPath))
        {
        User::Leave(KErrBadName);
        }

    CRsfwDavAccessContextGet* davAccessContextGet =
        CRsfwDavAccessContextGet::NewL(this,
                                   aResponseHandler,
                                   aRemotePathName,
                                   aLocalPathName,
                                   aOffset,
                                   aLength,
                                   aFlags);
    TUint id = AddAccessContext(davAccessContextGet);
    davAccessContextGet->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::PutFileL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::PutFileL(const TDesC& aLocalPathName,
                           const TDesC& aRemotePathName,
                           const TDesC8& aMimeType,
                           TInt aOffset,
                           TInt aLength,
                           TInt aTotalLength,
                           MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    // Copy file to server using PUT,
    // might require lock token, if the destination file is locked...
#ifdef _DEBUG
    {
    TInt length;
    if (aLength)
        {
        length = aLength;
        }
    else
        {
        length = 0;
        }
    DEBUGSTRING16(("DAV: PutFile ln='%S', rn='%S' (off=%d, len=%d)",
                   &aLocalPathName,
                   &aRemotePathName,
                   aOffset,
                   length));
    }
#endif // DEBUG

    // check that arguments are sensible
    if (aOffset < 0 ||
       (aLength < 0) ||
       (((aOffset + aLength) > aTotalLength) && aTotalLength > 0) ||
       (!aResponseHandler))
        {
        User::Leave(KErrArgument);
        }

    // note that aLocalPathName can be undefined
    // (CreateFile calls with null-ptr)
    if ((aLocalPathName.Length() > KMaxPath) ||
        (aRemotePathName.Length() == 0) ||
        (aRemotePathName.Length() > KMaxPath) ||
        (aMimeType.Length() > KMaxMimeTypeLength))
        {
        User::Leave(KErrBadName);
        }


    const HBufC8* lockToken = NULL;
    CRsfwDavFileInfo *davFileInfo = DavFileInfoL(aRemotePathName);
    if (davFileInfo)
        {
        lockToken = davFileInfo->LockToken();
        }
    CRsfwDavAccessContextPut* davAccessContextPut =
        CRsfwDavAccessContextPut::NewL(this,
                                   aResponseHandler,
                                   aLocalPathName,
                                   aRemotePathName,
                                   aMimeType,
                                   aOffset,
                                   aLength,
                                   aTotalLength,
                                   lockToken);
    TUint id = AddAccessContext(davAccessContextPut);
    davAccessContextPut->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::PutFileL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::PutFileL(const TDesC& aLocalPathName,
                           const TDesC& aRemotePathName,
                           const TDesC8& aMimeType,
                           MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    return PutFileL(aLocalPathName,
                    aRemotePathName,
                    aMimeType,
                    0,
                    NULL,
                    0,
                    aResponseHandler);
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::CreateFileL
// overwriting info not needed in HTTP PUT
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::CreateFileL(const TDesC& aPathName,
                              TBool /*aOverWriting*/,
                              MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    DEBUGSTRING16(("DAV: CreateFile '%S'", &aPathName));
    // check that arguments are sensible
    if (!aResponseHandler)
        {
        User::Leave(KErrArgument);
        }

    if ((aPathName.Length() == 0) ||
       (aPathName.Length() >  KMaxPath))
        {
        User::Leave(KErrBadName);
        }
        
    // Could create the file only from LOCK,
    // but that seems to cause 500 Internal Error with several
    // Apache servers...
    TPtrC null;
    return PutFileL(null, aPathName, KTextPlain, 0, NULL, 0, aResponseHandler);
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::MakeDirectoryL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::MakeDirectoryL(
    const TDesC& aPathName,
    MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    DEBUGSTRING16(("DAV: MakeDirectory", &aPathName));

    // check that arguments are sensible
    if (!aResponseHandler)
        {
        User::Leave(KErrArgument);
        }

    if ((aPathName.Length() == 0) ||
       (aPathName.Length() >  KMaxPath))
        {
        User::Leave(KErrBadName);
        }

    CRsfwDavAccessContextMkDir* davAccessContextMkDir =
        CRsfwDavAccessContextMkDir::NewL(this,
                                     aResponseHandler,
                                     aPathName);
    TUint id = AddAccessContext(davAccessContextMkDir);
    davAccessContextMkDir->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::DeleteDirectoryL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::DeleteDirectoryL(
    const TDesC& aPathName,
    MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    DEBUGSTRING16(("DAV: DeleteDirectory '%S'", &aPathName));

    // check that arguments are sensible
    if (!aResponseHandler)
        {
        User::Leave(KErrArgument);
        }

    if ((aPathName.Length() == 0) ||
       (aPathName.Length() >  KMaxPath))
        {
        User::Leave(KErrBadName);
        }

    CRsfwDavAccessContextDelete* davAccessContextDelete =
        CRsfwDavAccessContextDelete::NewL(this,
                                      aResponseHandler,
                                      aPathName,
                                      ETrue,
                                      NULL);
    TUint id = AddAccessContext(davAccessContextDelete);
    davAccessContextDelete->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::DeleteFileL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::DeleteFileL(const TDesC& aPathName,
                              MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    DEBUGSTRING16(("DAV: DeleteFile '%S'", &aPathName));

    // check that arguments are sensible
    if (!aResponseHandler)
        {
        User::Leave(KErrArgument);
        }

    if ((aPathName.Length() == 0) ||
       (aPathName.Length() >  KMaxPath))
        {
        User::Leave(KErrBadName);
        }

    const HBufC8* lockToken = NULL;
    CRsfwDavFileInfo *davFileInfo = DavFileInfoL(aPathName);
    if (davFileInfo)
        {
        lockToken = davFileInfo->LockToken();
        }
    CRsfwDavAccessContextDelete* davAccessContextDelete =
        CRsfwDavAccessContextDelete::NewL(this,
                                      aResponseHandler,
                                      aPathName,
                                      EFalse,
                                      lockToken);
    TUint id = AddAccessContext(davAccessContextDelete);
    davAccessContextDelete->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::RenameL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::RenameL(const TDesC& aSrcPathName,
                          const TDesC& aDstPathName,
                          TBool aOverwrite,
                          MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    DEBUGSTRING16(("DAV: Rename '%S' to '%S'", &aSrcPathName, &aDstPathName));

     // check that arguments are sensible
    if (!aResponseHandler)
        {
        User::Leave(KErrArgument);
        }

    if ((aSrcPathName.Length() == 0) ||
        (aSrcPathName.Length() >  KMaxPath) ||
        (aDstPathName.Length() == 0) ||
        (aDstPathName.Length() >  KMaxPath))
        {
        User::Leave(KErrBadName);
        }

    // lock token for the source file, if the source is locked...
    const HBufC8* lockToken = NULL;
    CRsfwDavFileInfo* davFileInfo = DavFileInfoL(aSrcPathName);
    if (davFileInfo)
        {
        lockToken = davFileInfo->LockToken();
        }
        
    
    // lock token for the destination file, if the destination is locked
    const HBufC8* destLockToken = NULL;
    CRsfwDavFileInfo* destDavFileInfo = DavFileInfoL(aDstPathName);
    if (destDavFileInfo)
        {
        destLockToken = destDavFileInfo->LockToken();
        }
    
        
    CRsfwDavAccessContextMove* davAccessContextMove =
        CRsfwDavAccessContextMove::NewL(this,
                                    aResponseHandler,
                                    aSrcPathName,
                                    aDstPathName,
                                    aOverwrite,
                                    lockToken,
                                    destLockToken);
    TUint id = AddAccessContext(davAccessContextMove);
    davAccessContextMove->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::GetDirectoryAttributesL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::GetDirectoryAttributesL(
    const TDesC& aPathName,
    CRsfwDirEntAttr*& aAttr,
    MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    DEBUGSTRING16(("DAV: GetDirectoryAttributes of '%S'",
                   &aPathName));

    // check that arguments are sensible
    // aPathName might here be null (mounted root directory)
    if (!aResponseHandler)
        {
        User::Leave(KErrArgument);
        }

    if (aPathName.Length() > KMaxPath)
        {
        User::Leave(KErrBadName);
        }

    CRsfwDavAccessContextPropFindDir* davAccessContextPropFindDir =
        CRsfwDavAccessContextPropFindDir::NewL(this,
                                           aResponseHandler,
                                           aPathName,
                                           0,
                                           &aAttr,
                                           NULL);
    TUint id = AddAccessContext(davAccessContextPropFindDir);
    davAccessContextPropFindDir->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::GetFileAttributesL
// ----------------------------------------------------------------------------
//
TUint
CRsfwDavAccess::GetFileAttributesL(const TDesC& aPathName,
                               CRsfwDirEntAttr*& aAttr,
                               MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    DEBUGSTRING16(("DAV: GetFileAttributes of '%S'",
                   &aPathName));

    // check that arguments are sensible
    if (!aResponseHandler)
        {
        User::Leave(KErrArgument);
        }

    if ((aPathName.Length() == 0) ||
       (aPathName.Length() >  KMaxPath))
        {
        User::Leave(KErrBadName);
        }

    CRsfwDavAccessContextPropFindFile* davAccessContextPropFindFile =
        CRsfwDavAccessContextPropFindFile::NewL(this,
                                            aResponseHandler,
                                            aPathName,
                                            &aAttr);
    TUint id = AddAccessContext(davAccessContextPropFindFile);
    davAccessContextPropFindFile->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::SetAttributesL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::SetAttributesL(
    const TDesC& /* aPathName */,
    CRsfwDirEntAttr& /* aAttr */,
    MRsfwRemoteAccessResponseHandler* /* aResponseHandler */)
    {
    DEBUGSTRING(("DAV: SetAttributes"));
    User::Leave(KErrNotSupported);
    // Not reached
    return 0;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::ObtainLockL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::ObtainLockL(const TDesC& aPathName,
                              TUint aLockFlags,
                              TUint& aTimeout,
                              TDesC8*& aLockToken,
                              MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    // check that arguments are sensible
    if ((!aResponseHandler) ||
        (aTimeout == 0))
        {
        User::Leave(KErrArgument);
        }

    // CRsfwDavAccess is the place to know about webdab locks
    // We only try to obtain a lock if the file was opened for writing
    if ((!(aLockFlags & EFileWrite)) ||
        (iWebDavSession->WebDavSupportClass() < KDavVersionTwo))
        {
        // WebDAV doesn't have read locks
        aResponseHandler->HandleRemoteAccessResponse(0, KErrNotSupported); 
        return 0;
        }
    
    if ((aPathName.Length() == 0) ||
       (aPathName.Length() >  KMaxPath))
        {
        User::Leave(KErrBadName);
        }

    CRsfwDavAccessContextLock* davAccessContextLock =
        CRsfwDavAccessContextLock::NewL(this,
                                    aResponseHandler,
                                    aPathName,
                                    aLockFlags,
                                    aTimeout,
                                    &aLockToken);
    TUint id = AddAccessContext(davAccessContextLock);
    davAccessContextLock->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::ReleaseLockL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::ReleaseLockL(const TDesC& aPathName,
                               MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    /*
      Precondition:
      - File has been at least locked so there must be fileInfo and
      locktoken for it
    */
    // check that arguments are sensible
    if (!aResponseHandler)
        {
        User::Leave(KErrArgument);
        }

    if ((aPathName.Length() == 0) ||
       (aPathName.Length() >  KMaxPath))
        {
        User::Leave(KErrBadName);
        }
           
    // Must send the Lock Token
    CRsfwDavFileInfo* davFileInfo = DavFileInfoL(aPathName);
    if (!davFileInfo)
        {
        User::Leave(KErrNotFound);
        }

    const HBufC8* lockToken = davFileInfo->LockToken();
    if (!lockToken)
        {
        User::Leave(KErrNotFound);
        }
    // Prevent further access to lock token
    davFileInfo->SetFlag(TRsfwDavFileInfoFlags::EUnlockPending);

    CRsfwDavAccessContextUnlock* davAccessContextUnlock =
        CRsfwDavAccessContextUnlock::NewL(this,
                                      aResponseHandler,
                                      aPathName,
                                      lockToken);
    TUint id = AddAccessContext(davAccessContextUnlock);
    davAccessContextUnlock->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::RefreshLockL
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::RefreshLockL(const TDesC& aPathName,
                               TUint& aTimeout,
                               MRsfwRemoteAccessResponseHandler* aResponseHandler)

    {
    /*
      Precondition:
      - File has been at least locked
      so there must be fileInfo and locktoken for it
    */
    // check that arguments are sensible
    if ((!aResponseHandler) ||
        (aTimeout == 0))
        {
        User::Leave(KErrArgument);
        }

    if ((aPathName.Length() == 0) ||
       (aPathName.Length() >  KMaxPath))
        {
        User::Leave(KErrBadName);
        }


    // Must send the Lock Token
    CRsfwDavFileInfo* davFileInfo = DavFileInfoL(aPathName);
    if (!davFileInfo)
        {
        User::Leave(KErrNotFound);
        }
    const HBufC8* lockToken = davFileInfo->LockToken();
    if (!lockToken)
        {
        User::Leave(KErrNotFound);
        }

    CRsfwDavAccessContextRefreshLock* davAccessContextRefreshLock =
        CRsfwDavAccessContextRefreshLock::NewL(this,
                                           aResponseHandler,
                                           aPathName,
                                           lockToken,
                                           aTimeout);
    TUint id = AddAccessContext(davAccessContextRefreshLock);
    davAccessContextRefreshLock->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::CancelL by ID
// ----------------------------------------------------------------------------
//
void CRsfwDavAccess::Cancel(TUint aId)
    {
    TInt i;
    if (aId)
        {
        i = LookupAccessContextByContextId(aId);
        if (i != KErrNotFound)
            {
            CRsfwDavAccessContext* context = iDavAccessContexts[i];
            // sometimes it may happen that transaction has not been created for 
            // some context (e.g. if a leave has occured)
            if (context->WebDavTransaction())
                {
                context->WebDavTransaction()->Cancel();
                }
            else
                {
                iDavAccessContexts.Remove(i);
                delete context;                
                }                        
            }
        }
    else
        {
        // Cancel all pending transactions.
        // Note that cancelling one transaction may result in creating the other
        // e.g. if you cancel PUT, then RELEASE LOCK is generated.
        // Anyway the purpose here is to 'kill' all of them, even those generated later
        DEBUGSTRING(("Cancelling all pending transactions..."));
        while (iDavAccessContexts.Count() > 0)
            {
            CRsfwDavAccessContext* context = iDavAccessContexts[0];
            TUint id = context->Id();
            Cancel(id);    
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::CancelL by path
// ----------------------------------------------------------------------------
//
void CRsfwDavAccess::Cancel(TDesC& aTargetPath)
    {
    DEBUGSTRING16(("CRsfwDavAccess::Cancel '%S'", &aTargetPath));
    TInt i;
    i = LookupAccessContextByPath(aTargetPath);
    if (i != KErrNotFound)
        {
        DEBUGSTRING16(("found transaction....cancelling webdavop %d", 
                    iDavAccessContexts[i]->WebDavTransaction()->iWebDavOp));
        iDavAccessContexts[i]->WebDavTransaction()->Cancel();
        }
    }



// ----------------------------------------------------------------------------
// CRsfwDavAccess::SetLockToken
// ----------------------------------------------------------------------------
//
TInt CRsfwDavAccess::SetLockToken(const TDesC& aPathName,
                              const TDesC8& aLockToken)
    {
    TRAPD(err, SetLockTokenL(aPathName, aLockToken));
    return err;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::SetLockTokenL
// ----------------------------------------------------------------------------
//
void CRsfwDavAccess::SetLockTokenL(const TDesC& aPathName,
                               const TDesC8& aLockToken)
    {
    HBufC* path = HBufC::NewLC(iRootDirectory.Length() + KMaxPath + 1);
    TPtr pathPtr = path->Des();
    pathPtr.Copy(iRootDirectory);
    pathPtr.Append(aPathName);

    DEBUGSTRING16(("DAV: setting lock token: path = '%S'", &pathPtr));
    DEBUGSTRING8(("                        token = '%S'", &aLockToken));
    
    if (aLockToken.Length())
        {
        CRsfwDavFileInfo* fileInfo = DavFileInfoL(pathPtr);
        if (fileInfo)
            {
            fileInfo->SetLockTokenL(aLockToken);
            }
        else 
            {
            // new file
            fileInfo = CRsfwDavFileInfo::NewL();
            CleanupStack::PushL(fileInfo);
            fileInfo->SetNameL(pathPtr);
            fileInfo->SetLockTokenL(aLockToken);
            CleanupStack::Pop(fileInfo);
            // Ownership transferred to file info array
            AddDavFileInfo(fileInfo);
            }
        }
    else
        {
        // Remove lock token
        RemoveDavFileInfoL(pathPtr);
        }

    CleanupStack::PopAndDestroy(path);
    }

// ----------------------------------------------------------------------------
// From MRsfwDavResponseObserver
// ----------------------------------------------------------------------------

void CRsfwDavAccess::RequestCompleteL(TUint aWebDavTransactionId)
    {
    TInt i = LookupAccessContextByTransactionId(aWebDavTransactionId);
    if (i != KErrNotFound)
        {
        CRsfwDavAccessContext* context = iDavAccessContexts[i];
        DEBUGSTRING(("DAV: Request %d complete", aWebDavTransactionId));
        context->TransactionCompleteL();
        if (context->Done())
            {
            MRsfwRemoteAccessResponseHandler* responseHandler =
                context->ResponseHandler();
            TUint id = context->Id();
            TInt status = context->Status();
            iDavAccessContexts.Remove(i);
            delete context;
            responseHandler->HandleRemoteAccessResponse(id, status);
            }
        }
    else
        {
        DEBUGSTRING(("DAV: Stray request (id=%d) complete",
                     aWebDavTransactionId));
        }
    }

void CRsfwDavAccess::RequestError(TUint aWebDavTransactionId, TInt aStatus)
    {
    TInt i = LookupAccessContextByTransactionId(aWebDavTransactionId);
    if (i != KErrNotFound)
        {
        CRsfwDavAccessContext* context = iDavAccessContexts[i];
        DEBUGSTRING(("DAV: Request %d error %d",
                     aWebDavTransactionId,
                     aStatus));
        context->TransactionError(aStatus);
        if (context->Done())
            {
            MRsfwRemoteAccessResponseHandler* responseHandler =
                context->ResponseHandler();
            TUint id = context->Id();
            TInt status = context->Status();
            iDavAccessContexts.Remove(i);
            delete context;
            responseHandler->HandleRemoteAccessResponse(id, status);
            }
        }
    else
        {
        DEBUGSTRING(("DAV: Stray request (id=%d) error",
                     aWebDavTransactionId));
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::AddAccessContext
// Add a context entry in the table of currently active contexts.
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::AddAccessContext(CRsfwDavAccessContext* aDavAccessContext)
    {
    TUint id = GetNextAccessContextId();
    DEBUGSTRING(("DAV: Added transaction %d", id));
    aDavAccessContext->SetId(id);
    iDavAccessContexts.Append(aDavAccessContext);
    return id;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::LookupAccessContextByTransactionId
// Find an access context for the given WebDAV transaction id.
// ----------------------------------------------------------------------------
//
TInt CRsfwDavAccess::LookupAccessContextByTransactionId(TUint aWebDavTransactionId)
    {
    TInt i;
    for (i = 0; i < iDavAccessContexts.Count(); i++)
        {
        if (iDavAccessContexts[i]->WebDavTransactionId() ==
            aWebDavTransactionId)
            {
            return i;
            }
        }
    DEBUGSTRING(("Access context for id %d is missing !!!",
                 aWebDavTransactionId));
    return KErrNotFound;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::LookupAccessContext
// Find an access context having the given id.
// ----------------------------------------------------------------------------
//
TInt CRsfwDavAccess::LookupAccessContextByContextId(TUint aId)
    {
    TInt i;
    for (i = 0; i < iDavAccessContexts.Count(); i++)
        {
        if (iDavAccessContexts[i]->Id() == aId)
            {
            return i;
            }
        }
    DEBUGSTRING(("Access context for id %d is missing !!!", aId));
    return KErrNotFound;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::LookupAccessContextByPath
// Find an access context based on the path of the targer file/directory.
// ----------------------------------------------------------------------------
//
TInt CRsfwDavAccess::LookupAccessContextByPath(TDesC& aTargetPath)
    { 
    TInt i;
    for (i = 0; i < iDavAccessContexts.Count(); i++)
        {
        if (iDavAccessContexts[i]->TargetPath() == aTargetPath)
            {
            return i;
            }
        }
    return KErrNotFound;
    }


// ----------------------------------------------------------------------------
// CRsfwDavAccess::DavFileInfoIndexL
// Find the index to file info array for the given file name.
// ----------------------------------------------------------------------------
//
TInt CRsfwDavAccess::DavFileInfoIndexL(const TDesC& aName)
    {
    HBufC* path = HBufC::NewLC(iRootDirectory.Length() + KMaxPath + 1);
    TPtr pathPtr = path->Des();
    pathPtr.Copy(iRootDirectory);
    pathPtr.Append(aName);

    DEBUGSTRING16(("Finding file info for '%S'", &pathPtr));
    TInt index;
    for (index = 0; index < iDavFileInfos.Count(); index++)
        {
        const HBufC* name = iDavFileInfos[index]->Name();
        if (name)
            {
            if (pathPtr.Compare(*name) == 0)
                {
                DEBUGSTRING(("info found"));
                CleanupStack::PopAndDestroy(path);
                return index;
                }
            }
        }
    CleanupStack::PopAndDestroy(path);
    return KErrNotFound;
    }

// ----------------------------------------------------------------------------
// CRsfwDavAccess::OptionsL
// Perform an Options query
// ----------------------------------------------------------------------------
//
TUint CRsfwDavAccess::OptionsL(MRsfwRemoteAccessResponseHandler* aResponseHandler)
    {
    CRsfwDavAccessContextOptions* davAccessContextOptions =
        CRsfwDavAccessContextOptions::NewL(this, aResponseHandler);
    TUint id = AddAccessContext(davAccessContextOptions);
    davAccessContextOptions->StartL();
    return id;
    }

// ----------------------------------------------------------------------------
// From MRsfwConnectionObserver
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// CRsfwDavAccess::HandleConnectionEventL
// Perform an Options query
// ----------------------------------------------------------------------------
//
void CRsfwDavAccess::HandleConnectionEventL(TInt aConnectionEvent,
                                        TAny* /*aArg*/)

    {
    switch (aConnectionEvent)
        {
    case ERsfwConnectionObserverEventConnectionDisconnected:
        if (iRsfwRemoteAccessObserver)
            {
            iRsfwRemoteAccessObserver->
                HandleRemoteAccessEventL(
                    ERsfwRemoteAccessObserverEventConnection,
                    ERsfwRemoteAccessObserverEventConnectionDisconnected,
                    NULL);
            }
        break;
        
    case ERsfwConnectionObserverEventConnectionWeaklyConnected:
        // If we were reacting to link up events, we would do OptionsL(this);
        break;
        
    case ERsfwConnectionObserverEventConnectionStronglyConnected:
        // If we were reacting to link up events, we would do OptionsL(this);
        break;
        
    default:
        break;
        }
    }

//  End of File
