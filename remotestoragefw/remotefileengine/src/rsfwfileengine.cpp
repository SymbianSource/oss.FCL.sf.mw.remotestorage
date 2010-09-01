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
* Description:  Operation independent remote file handling functions
*
*/


#include <apgcli.h>
#include <bautils.h>

#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwrfestatemachine.h"
#include "rsfwinterface.h"
#include "rsfwcontrol.h"
#include "rsfwremoteaccess.h"
#include "rsfwfileengine.h"
#include "rsfwrfeserver.h"
#include "rsfwlockmanager.h"
#include "mdebug.h"
#include "rsfwdirent.h"
#include "rsfwdirentattr.h"
#include "rsfwinterface.h"

// ----------------------------------------------------------------------------
// CRsfwFileEngine::NewL
// ----------------------------------------------------------------------------
//
CRsfwFileEngine* CRsfwFileEngine::NewL(CRsfwVolume* aVolume)
    {
    CRsfwFileEngine* self = CRsfwFileEngine::NewLC(aVolume);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::NewLC
// ----------------------------------------------------------------------------
//
CRsfwFileEngine* CRsfwFileEngine::NewLC(CRsfwVolume* aVolume)
    {
    DEBUGSTRING(("CRsfwFileEngine::NewLC"));
    CRsfwFileEngine* self = new (ELeave) CRsfwFileEngine();
    DEBUGSTRING(("CRsfwFileEngine: in NewLC 0x%x", self));
    CleanupStack::PushL(self);
    self->ConstructL(aVolume);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::ConstructL(CRsfwVolume* aVolume)
    {
    iRemoteAccess = NULL;
    iRootFid = NULL;
    iRootFep = NULL;
    iVolume = aVolume;
    iFs = CRsfwRfeServer::Env()->iFs;
    iConnectionState = KMountNotConnected;
    __ASSERT_ALWAYS(iVolume != NULL, User::Panic(KRfeServer, EConstructingServerStructs));
    iInactivityTimeout =
       iVolume->iMountInfo.iMountConfig.iInactivityTimeout * 1000000;
    PrepareCacheL();
    // Create file table
    iFileTable = CRsfwFileTable::NewL(aVolume, iCacheRoot);
    __ASSERT_ALWAYS(iVolume->iVolumeTable != NULL, User::Panic(KRfeServer, 
    			EConstructingServerStructs));
    SetupRootL(iVolume->iVolumeTable->iPermanence);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::~CRsfwFileEngine
// ----------------------------------------------------------------------------
//
CRsfwFileEngine::~CRsfwFileEngine()
    {
    DEBUGSTRING(("CRsfwFileEngine destructor"));
    delete iFileTable;
    delete iRemoteAccess;
    delete iLockManager;
    StopInactivityTimer();
    delete iInactivityTimer;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::DispatchL
// we should only come here with some synchronous requests
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::DispatchL(TRfeInArgs& aIn, TRfeOutArgs& aOut)
    {

    switch(aIn.iOpCode)
        {
    case EFsIoctl:
        DEBUGSTRING(("IOCTL"));
        DoIoctlL(static_cast<TRfeIoctlInArgs&>(aIn),
                 aOut);
        break;

    case EFsRoot:
        DEBUGSTRING(("ROOT"));
        DoRootL(static_cast<TRfeRootInArgs&>(aIn),
                static_cast<TRfeRootOutArgs&>(aOut));
        break;

    case ESetAttr:
        DEBUGSTRING(("SETATTR"));
        DoSetAttrL(static_cast<TRfeSetAttrInArgs&>(aIn),
                   aOut);
        break;

    default:
        DEBUGSTRING(("WHAT??? - %d", aIn.iOpCode));
        User::Leave(KErrArgument);
        break;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::FullNameLC
// ----------------------------------------------------------------------------
//
HBufC* CRsfwFileEngine::FullNameLC(CRsfwFileEntry& aFe)
    {
    HBufC* fn = aFe.FullNameLC();
    return fn;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::FullNameL
// ----------------------------------------------------------------------------
//
HBufC* CRsfwFileEngine::FullNameL(CRsfwFileEntry& aFe)
    {
    HBufC* fn = FullNameLC(aFe);
    CleanupStack::Pop(fn);
    return fn;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::SetupAttributes
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::SetupAttributes(CRsfwFileEntry& aFe)
    {
    DEBUGSTRING(("CRsfwFileEngine::SetupAttributes"));
    // Construct the attributes for a newly created file or directory,
    // or a file that that was locally modified and just written to the server,
    // based on local knowledge of time and file size.
    // We assume that either the file is cached or it is an empty file.
    // We do not touch the local or protection attributes.

    TUint att;

    // Assume that the file type has already been setup
    if (aFe.Type() == KNodeTypeDir)
        {
        att = KEntryAttDir;
        }
    else
        {
        att = 0;
        }

    TTime time;
    if (aFe.IsCached())
        {
        TDesC* cacheNamep = aFe.CacheFileName();
        RFile f;
        if (f.Open(iFs, *cacheNamep, EFileShareAny) == KErrNone)
            {
            // attribute bits
            TUint a;
            f.Att(a);

            att |= a & KEntryAttReadOnly;

            if (aFe.Type() == KNodeTypeDir)
                {
                aFe.SetSize(0);
                }
            else
                {
                if (aFe.IsFullyCached())
                    {
                    // size
                    TInt siz;
                    f.Size(siz);
                    DEBUGSTRING(("File is fully cached, setting size to %d", siz));
                    aFe.SetSize(siz);
                    aFe.SetCachedSize(siz);
                    }
                else
                	{
                	DEBUGSTRING(("File is not fully cached, not touching the size"));
                	// file is not fully cached
                	// the size cannot be set from the local cache container	
                	}
                }
            // modification time
            f.Modified(time);

            f.Close();
            aFe.iUseCachedData = ETrue;
          }
        else 
          {
          // No cache
          aFe.SetSize(0);
          time.HomeTime();        
          }
        
        }
    else
        {
        // No cache
        aFe.SetSize(0);
        time.HomeTime();
        }

    aFe.SetAtt(att);

    aFe.SetModified(time);
    aFe.SetAttribValidationTime();
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::MakeDirectoryEntry
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::MakeDirectoryEntry(CRsfwFileEntry& aFe, TDirEnt& aDirEnt)
    {
    DEBUGSTRING(("CRsfwFileEngine::MakeDirectoryEntry"));
    DEBUGSTRING16(("name %S, att %d, size %d", aFe.Name(), aFe.Att(), aFe.Size()));;
    aDirEnt.Clear();
    aDirEnt.iName.Copy(*aFe.Name());
    aDirEnt.iAttr.iAtt = aFe.Att();
    aDirEnt.iAttr.iSize = aFe.Size();
    aDirEnt.iAttr.iModified = aFe.Modified();
    aDirEnt.iAttr.iUid3 = aFe.iUid;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::UpdateDirectoryContainerL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::UpdateDirectoryContainerL(CRsfwFileEntry& aFe)
    {
    // Construct the directory container based on
    // file table information
    DEBUGSTRING16(("Update directory container of %d (%S)", aFe.Fid().iNodeId, aFe.Name()));

    TDesC* cacheNamep = aFe.CacheFileName();
    if (!cacheNamep)
        {
        // There was no prior cache.
        DEBUGSTRING(("Cache missing!"));
        User::Leave(KErrGeneral);
        }

    RFile f;
    CleanupClosePushL(f);
    User::LeaveIfError(f.Replace(iFs,
                                 *cacheNamep,
                                 EFileShareAny | EFileWrite));
    RFileWriteStream fStream(f);
    CleanupClosePushL(fStream);

    RPointerArray<CRsfwFileEntry>* kidsp = aFe.Kids();
    TInt i;
    if (!(iVolume->iVolumeTable->EnsureCacheCanBeAddedL(
              sizeof(TEntry) * kidsp->Count())))      
        {   // pessimistic estimate
        User::Leave(KErrDiskFull);
        }
    for (i = 0; i < kidsp->Count(); i++)
        {
        CRsfwFileEntry* kidFep = (*kidsp)[i];
        TDirEnt dirEnt;
        MakeDirectoryEntry(*kidFep, dirEnt);
        dirEnt.ExternalizeL(fStream);      
        }
    CleanupStack::PopAndDestroy(2, &f); // f

    aFe.ResetLocallyDirty();
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::DataChanged
// ----------------------------------------------------------------------------
//
TInt CRsfwFileEngine::DataChanged(const CRsfwDirEntAttr& aOldAttr,
                              const CRsfwDirEntAttr& aNewAttr)
    {
    // Based on attributes or metadata in general,
    // tell whether the actual data, if cached,
    // should be updated
    if (aOldAttr.Att() == KEntryAttDir)
        {
        // use Last Modified (a weak entity tag)
        if (aOldAttr.Modified() == aNewAttr.Modified())
            {
            return EFalse;
            }
        else
            {
            return ETrue;
            }
        }
    else
        {
        // use ETags if available
        // a strong entity tag
        if (aOldAttr.ETag() && aNewAttr.ETag())
            {
            if (*aOldAttr.ETag() == *aNewAttr.ETag())
                {
                return EFalse;
                }
            else
                {
                return ETrue;
                }
            }

        // use Last Modified (a weak entity tag)
        // we assume it's file and compare also iSize...
        if ((aOldAttr.Modified() == aNewAttr.Modified()) &&
            (aOldAttr.Size() == aNewAttr.Size()))
            {
            return EFalse;
            }
        else
            {
            return ETrue;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::UseCachedData
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEngine::UseCachedData(CRsfwFileEntry& aFe)
    {
    if (!Disconnected())
        {
        return aFe.UseCachedData();
        }
    else
        {
        return ETrue;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::UseCachedAttributes
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEngine::UseCachedAttributes(CRsfwFileEntry& aFe)
    {
      if (!Disconnected())
        {
        if (aFe.Type() == KNodeTypeDir)
			{
			return iFileTable->Volume()->iVolumeTable->
				IsCachedAttrStillValid(aFe.iAttribValidation);
			}
		else
			{ // file
			return iFileTable->Volume()->iVolumeTable->
				IsCachedDataStillValid(aFe.iAttribValidation);

			}
        }
    else
        {
        return ETrue;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::GetAttributesL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::GetAttributesL(CRsfwFileEntry& aFe,
                                 CRsfwDirEntAttr*& aAttr,
                                 TUint aNodeType,
                                 CRsfwRfeStateMachine* aCaller)
    {
    // Gets attributes for File Entry aFe.
    // Uses either cached attributes (if they are still deemed to be valid), or
    // fetches the attributes from the server*/
    DEBUGSTRING(("GetAttributesL"));
    if ((aFe.Type() == aNodeType) && UseCachedAttributes(aFe))
        {
        // Nothing to do

        if (aFe.IsOpenedForWriting())
            {
            // update attributes when we are writing to the file
            DEBUGSTRING(("volatile attributes"));
            SetupAttributes(aFe);
            }
        else
            {
            DEBUGSTRING(("using cached attributes"));
            }
        aCaller->HandleRemoteAccessResponse(0, KErrNone); // "file exists"
        }
    else
        {
        // Refresh attributes
        UpdateAttributesL(aFe, aAttr, aNodeType, aCaller);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::UpdateAttributesL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::UpdateAttributesL(CRsfwFileEntry& aFe,
                                    CRsfwDirEntAttr*& aAttr,
                                    TUint aNodeType,
                                    MRsfwRemoteAccessResponseHandler* aCaller)
    {
    // UpdateAttributes doesn't attempt to use cached attributes
    HBufC* path = FullNameLC(aFe);
    TPtr p = path->Des();
    DEBUGSTRING16(("UpdateAttributesL of '%S'", &p));


    UpdateAttributesL(*path, aAttr, aNodeType, aCaller);

    CleanupStack::PopAndDestroy(path); // path
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::UpdateAttributesL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::UpdateAttributesL(TDesC& aPath,
                                    TDesC& aName,
                                    CRsfwDirEntAttr*& aAttr,
                                    TUint aNodeType,
                                    MRsfwRemoteAccessResponseHandler* aCaller)
    {
    HBufC* pn = HBufC::NewLC(KMaxPath);
    TPtr pnPtr = pn->Des();

    if (aPath.Length())
        {
        pnPtr.Copy(aPath);
        pnPtr.Append('/');
        }
    pnPtr.Append(aName);

    DEBUGSTRING16(("UpdateKidAttributes of '%S'", &pnPtr));

   	UpdateAttributesL(pnPtr, aAttr, aNodeType, aCaller);

    CleanupStack::PopAndDestroy(pn);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::UpdateAttributesL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::UpdateAttributesL(TDesC& aFullPath,
                                    CRsfwDirEntAttr*& aAttr,
                                    TUint aNodeType,
                                    MRsfwRemoteAccessResponseHandler* aCaller)
	{

	// If we have "recently" found out that this file/dir does NOT exist
	// we cache even this negative result. Time limit is cache expiry for
	// directory attributes
	 if ((aFullPath.Length() > 0) &&    // do not compare root folder (always exists)
	 (iLastFailedLookup == aFullPath) &&
	 	  (iFileTable->Volume()->iVolumeTable->
				IsCachedAttrStillValid(iLookupTime)))
    	{
    	if (aNodeType == KNodeTypeDir)
            {
            aCaller->HandleRemoteAccessResponse(0, KErrPathNotFound);
            }
        else if (aNodeType == KNodeTypeFile)
            {
            aCaller->HandleRemoteAccessResponse(0, KErrNotFound);
            }
        return;

    	}

	if (!Disconnected())
        {
        if (aNodeType == KNodeTypeDir)
            {
            RemoteAccessL()->GetDirectoryAttributesL(aFullPath, aAttr, aCaller);
            }
        else if (aNodeType == KNodeTypeFile)
            {
            RemoteAccessL()->GetFileAttributesL(aFullPath, aAttr, aCaller);
            }
        }
    else
        {
        User::Leave(KErrNotFound);
        }

	}

// ----------------------------------------------------------------------------
// CRsfwFileEngine::CreateContainerFileL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::CreateContainerFileL(CRsfwFileEntry& aFe)
    {
    // Create a container file for the Fid.
    // If the cache file already exists, it will be deleted

    RFile f;
    HBufC* cachePath = HBufC::NewMaxLC(KMaxPath);
    TPtr pathPtr = cachePath->Des();
    BuildContainerPathL(aFe, pathPtr);

    TInt err = f.Replace(iFs, *cachePath, EFileShareAny | EFileWrite);
    f.Close();
    if (err != KErrNone)
        {
        DEBUGSTRING(("Error when creating container file! err=%d", err));
        User::Leave(KErrGeneral);
        }  
    aFe.SetCacheFileName(cachePath);
    CleanupStack::PopAndDestroy(cachePath);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::FetchAndCacheL
// ----------------------------------------------------------------------------
//
TUint CRsfwFileEngine::FetchAndCacheL(CRsfwFileEntry& aFe,
                                  TInt aFirstByte,
                                  TInt* aLength,
                                  RPointerArray<CRsfwDirEnt>* aDirEntsp,
                                  CRsfwRfeStateMachine* aCaller)
    {
    // Fetch a file from the remote store and decrypt it if necessary.
    // The assumption is that the file has not yet been fetched
    // or has been cached up to the byte indicated by (aFe.iCachedSize - 1)
    // and filling the cache will continue linearly
    // i.e. aFirstByte = 0 || aFirstByte = aFe.iCachedSize
    // Access modules can fetch more than requested, so aLastByte might change.

    DEBUGSTRING(("Fetch fid %d, bytes %d - %d",
                 aFe.Fid().iNodeId,
                 aFirstByte,
                 aFirstByte + *aLength));

    TUint transactionId = 0;
    RFile f;
    HBufC* fullName = NULL;
    HBufC* cacheName = HBufC::NewMaxLC(KMaxPath);
    TPtr cachePtr = cacheName->Des();
    TInt err;
    
    TInt usedCache = iVolume->iVolumeTable->TotalCachedSize();

    // This much will be added to the cache by this fetch
    if (!iVolume->iVolumeTable->EnsureCacheCanBeAddedL(*aLength))
        {
        User::Leave(KErrDiskFull);
        }

    if (!Disconnected())
        {
        if (aFe.CacheFileName())
            {
            // modify an existing cachefile ...
            cachePtr = *(aFe.CacheFileName());

            if (aFe.Type() == KNodeTypeFile)
                {
                // If the cache file exists,
                // we will just continue filling it...
                err = f.Open(iFs, *cacheName, EFileShareAny | EFileWrite);
                if (err)
                    {
                    User::LeaveIfError(f.Replace(iFs,
                                                 *cacheName,
                                                 EFileShareAny | EFileWrite));
                    }
                }
            else
                {
                User::LeaveIfError(f.Replace(iFs,
                                             *cacheName,
                                             EFileShareAny | EFileWrite));
                }      
            }
        else
            {
            // create a new cache file
            CreateContainerFileL(aFe, cachePtr, f);
            }

        CleanupClosePushL(f);
        fullName = FullNameLC(aFe);
        if (aFe.Type() == KNodeTypeDir)
            {
            transactionId = GetDirectoryL(aFe,
                                          *fullName,
                                          f,
                                          aDirEntsp,
                                          aCaller);
            }
        else if (aFe.Type() == KNodeTypeFile)
            {
            f.Close();
            transactionId = RemoteAccessL()->GetFileL(*fullName,
                                                      *cacheName,
                                                      aFirstByte,
                                                      aLength,
                                                      0,
                                                      aCaller);
            }

        // fullName, f (duplicate close in the case of files)
        CleanupStack::PopAndDestroy(2, &f);
        }
    CleanupStack::PopAndDestroy(cacheName);    
    return transactionId;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::RequestConnectionStateL
// ----------------------------------------------------------------------------
//
TUint CRsfwFileEngine::RequestConnectionStateL(TUint aConnectionState,
                                           CRsfwRfeStateMachine* aCaller)
    {
    DEBUGSTRING16(("CRsfwFileEngine::RequestConnectionStateL %d", aConnectionState));
    DEBUGSTRING16(("current connection state is %d", iConnectionState));
    TUint transactionId = 0;
    if (aConnectionState != iConnectionState)
        {
        switch (aConnectionState)
            {
        case KMountNotConnected:
            DisconnectL();
            break;
        case KMountStronglyConnected:
            transactionId = ConnectL(ETrue, aCaller);
            break;

        default:
            break;
            }
        }
    // else does not do anything (if iConnectionState == aConnectionState)    
    return transactionId;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::EnteredConnectionStateL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::EnteredConnectionStateL(TUint aConnectionState,
                                          TBool aRequested)
    {
    DEBUGSTRING16(("CRsfwFileEngine::EnteredConnectionStateL %d", aConnectionState));
    DEBUGSTRING16(("current connection state is %d", iConnectionState));
    if (aConnectionState != iConnectionState)
        {
        iConnectionState = aConnectionState;
        iVolume->ConnectionStateChanged(iConnectionState);

        switch (aConnectionState)
            {
        case KMountNotConnected:
            if (!aRequested)
                {
                iRemoteAccess->Cancel(0);
                }
            break;

        case KMountStronglyConnected:
            if (aRequested)
                {
                if (iLockManager)
                    {
                    iLockManager->PopulateExternalLockTokenCacheL(iRootFep);
                    }
                }
            break;

        default:
            break;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::ConnectionState
// ----------------------------------------------------------------------------
//
TUint CRsfwFileEngine::ConnectionState()
    {
    return iConnectionState;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::LockManager
// ----------------------------------------------------------------------------
//
CRsfwLockManager* CRsfwFileEngine::LockManager()
    {
    return iLockManager;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::SetPermanenceL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::SetPermanenceL(TBool aPermanence)
    {
    iFileTable->SetPermanenceL(aPermanence);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::Disconnected
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEngine::Disconnected()
    {
    return (iConnectionState == KMountNotConnected);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::WriteDisconnected
// ----------------------------------------------------------------------------
//
TBool CRsfwFileEngine::WriteDisconnected()
    {
    // This also encompasses disconnected mode
    return (iConnectionState != KMountStronglyConnected);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::AddToCacheL
// ----------------------------------------------------------------------------
//
TInt CRsfwFileEngine::AddToCacheL(CRsfwFileEntry& aFe,
                              RPointerArray<CRsfwDirEnt>* aDirEnts,
                              CRsfwFileEngine *aFileEngine,
                              TUint cachedSize)
    {
    // returns the size of the cached data
    RFs fs = CRsfwRfeServer::Env()->iFs;
    TInt err;
    TInt kidsCount = 0;
    TInt containerSize = cachedSize;
    // holds true for files, will be overwritten for directories

    if (aFe.Type() == KNodeTypeDir)
        {
        // *********** originally from CRsfwFileEngine::GetDirectoryL()
        // **********************************************************
        // Unmark and mark kids only when getdirectory returns KErrNone
        // otherwise (i.e. KErrUpdateNotRequired) let's just keep
        // the cached kids...
        aFe.UnmarkKids();

        RApaLsSession lsSession;
        User::LeaveIfError(lsSession.Connect());
        CleanupClosePushL(lsSession);

        RFileWriteStream fStream;
        // Dump to the local cache
        User::LeaveIfError(
            fStream.Open(fs,
                         *(aFe.CacheFileName()),
                         EFileWrite | EFileShareAny));
        CleanupClosePushL(fStream);

        containerSize = fStream.Sink()->SizeL();
        TInt i;
        TLex lex;
        for (i = 0; i < aDirEnts->Count(); i++)
            {
		    CRsfwDirEnt* d = (*aDirEnts)[i];
            TUid appUid;
            // For each TDirEnt we just read...
            // ... if the server returned content-type
            if (d->Attr()->MimeType() && d->Attr()->MimeType()->Length())
                {
                err = lsSession.AppForDataType(*(d->Attr()->MimeType()),
                                               appUid);
                if (err == KErrNone)
                    {
                    d->Attr()->SetUid(appUid);
                    }
                }

            d->Attr()->SetAttFlags(KEntryAttRemote);
            CRsfwFileEntry* kidFep = aFe.FindKidByName(*d->Name());
            if (kidFep)
                {
                // We already know this kid
                // However we must check whether the kid has been modified
                CRsfwDirEntAttr* oldAttr = CRsfwDirEntAttr::NewLC();
                kidFep->GetAttributesL(*oldAttr);
                if (DataChanged(*oldAttr, *d->Attr()))
                    {
                    kidFep->RemoveCacheFile();
                    }
                CleanupStack::PopAndDestroy(oldAttr);
                if (kidFep->IsFullyCached())
                    {
                    // Mark the kid as cached
                    d->Attr()->ResetAttFlags(KEntryAttRemote);
                    }
                 // as this entry is "used", move it to the back of metadata LRU list
                 iVolume->iVolumeTable->MoveToTheBackOfMetadataLRUPriorityListL(kidFep);
                }

            // As a side effect,
            // insert this kid into the file table and
            // set its attributes
            if (!kidFep)
                {
                if (!iVolume->iVolumeTable->EnsureMetadataCanBeAddedL(&aFe))
                    {
                    User::Leave(KErrNoMemory);
                    }
                kidFep = CRsfwFileEntry::NewL(*d->Name(), &aFe);
                // Attach the new kid
                aFileEngine->iFileTable->AddL(kidFep);
                aFe.AddKid(*kidFep);
                }

            kidFep->Mark();
            
            // set attributes if getting directory listing also supports getting file attributes
            if (DirectoryListingContainsFileMetadata()) 
                {
                kidFep->SetAttributesL(*d->Attr(), ETrue);
                }
             else 
                {
                kidFep->SetAttributesL(*d->Attr(), EFalse);
                }

            TDirEnt dirEnt;
            MakeDirectoryEntry(*kidFep, dirEnt);
            dirEnt.ExternalizeL(fStream);
            kidsCount++;
            }

        aFe.DropUnmarkedKidsL();

        containerSize = fStream.Sink()->SizeL();
        // assumes that this fetch will write the whole directory,
        
        // i.e. there is no partial fetching for the directories
        if(!iFileTable->Volume()->iVolumeTable->
           EnsureCacheCanBeAddedL(containerSize))
            {
            User::Leave(KErrDiskFull);
            }
        fStream.CommitL();

        CleanupStack::PopAndDestroy(2, &lsSession); // fStream, lsSession

        // if the directory appeared to be childless add it to metadata LRU list
        if ( aDirEnts->Count() == 0 )
            {
            iVolume->iVolumeTable->AddToMetadataLRUPriorityListL(&aFe, ECachePriorityNormal);
            }

        }// if directory

    // assumes the files are cached in continuos chunks,
    // i.e. always cached up to the last byte fetched
    aFe.SetCachedSize(containerSize);

    aFe.SetCached(ETrue);

    // We have to update locally dirty bit for the parent container
    if (aFe.Parent())
        {
        aFe.Parent()->SetLocallyDirty();
        }
    // But the object itself cannot be remotely dirty any more
    aFe.ResetRemotelyDirty();

    // *** from CRsfwFileEngine::DoFetch ***
    if (aFe.Type() == KNodeTypeDir)
        {
        // the reason why kidsCount may be different than aFe.Kids.Count is that for big directories
        // some kids could have been removed when adding the others to memory. this is due to memory management cap.
        // however this should not happen so often
        aFe.KidsCount() == kidsCount ? aFe.iUseCachedData = ETrue : aFe.iUseCachedData = EFalse;
        }
    else
        {
        aFe.iUseCachedData = ETrue;
        }

    return containerSize;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::RemoteAccessL
// ----------------------------------------------------------------------------
//
CRsfwRemoteAccess* CRsfwFileEngine::RemoteAccessL()
    {
    DEBUGSTRING(("CRsfwFileEngine::RemoteAccessL"));
    if (!iRemoteAccess)
        {
        User::Leave(KErrNotReady);
        }

    // Prevent the inactivity timer from triggering
    // in the middle of a remote access operation
    StopInactivityTimer();

    return iRemoteAccess;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::OperationCompleted
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::OperationCompleted()
    {
    DEBUGSTRING(("File engine operation completed"));
    if (iVolume->iVolumeTable->iPermanence)
        {
        iFileTable->SaveMetaDataDelta();
        }
        
    if (iLockManager && (iLockManager->LockedCount() == 0))
        {
        // Start timer only if we don't have files open for writing
        StartInactivityTimer();
        }

    iVolume->OperationCompleted();
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::CancelTransaction
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::CancelTransaction(TUint iTransactionId)
    {
    DEBUGSTRING(("CRsfwFileEngine::CancelTransactionL"));
    if (iRemoteAccess) 
        {
        iRemoteAccess->Cancel(iTransactionId);
        }

    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::CancelTransaction
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::CancelTransactionL(TDesC& aPathName)
    {
    DEBUGSTRING(("CRsfwFileEngine::CancelTransactionL"));
    TPtrC testPtr;
    testPtr.Set(aPathName.Right(aPathName.Length() - 3));
    HBufC* cancelPath = HBufC::NewLC(KMaxPath);
    TPtr cancelPathPtr = cancelPath->Des();
    // change '\\' to '/' so the path matches
    TLex parser(testPtr);
    TChar theChar;
    
    for (int i = 0; i < testPtr.Length(); i++)
        {
        theChar = parser.Get();
        if (theChar == 0) 
            {
            break;
            }
        // assumes that the input string always has "\\" and not just "\"
        // this is true as the input is a file path
        if (theChar != '\\') 
            {
            cancelPathPtr.Append(theChar);
            }
        else 
            {
            cancelPathPtr.Append('/');
            }        
        }
    
    if (iRemoteAccess) 
        {
        iRemoteAccess->Cancel(*cancelPath);
        }
        
    CleanupStack::PopAndDestroy(cancelPath);    
    
    }

    

    
// ----------------------------------------------------------------------------
// CRsfwFileEngine::SetFailedLookup
// Caches the last failed lookup result
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::SetFailedLookup(TDesC& aPath, TDesC& aKidName)
	{
	iLastFailedLookup = aPath;
	iLastFailedLookup.Append('/');
	iLastFailedLookup.Append(aKidName);
	iLookupTime.HomeTime();
	DEBUGSTRING16(("SetFailedLookup: %S", &iLastFailedLookup));
	}

// ----------------------------------------------------------------------------
// CRsfwFileEngine::ResetFailedLookup
// Clears the last failed lookup result
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::ResetFailedLookup()
	{
	DEBUGSTRING16(("ResetFailedLookup: %S", &iLastFailedLookup));
	iLastFailedLookup.Zero();
	}

// ----------------------------------------------------------------------------
// CRsfwFileEngine::Volume
// ----------------------------------------------------------------------------
//
CRsfwVolume* CRsfwFileEngine::Volume()
    {
    return iVolume;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::PrepareCacheL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::PrepareCacheL()
    {
    // make sure the file cache (of this volume) exists
    iCacheRoot.Copy(CRsfwRfeServer::Env()->iCacheRoot);
    iCacheRoot.Append('C');
    iCacheRoot.AppendNum(iVolume->iMountInfo.iMountStatus.iVolumeId);
    iCacheRoot.Append('\\');
    
    if (! BaflUtils::FileExists(iFs, iCacheRoot))
        {
        // There was no prior cache directory
        TInt err = iFs.MkDirAll(iCacheRoot);
        DEBUGSTRING(("Cache directory created with err=%d", err));
        User::LeaveIfError(err);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::GetDirectoryL
// ----------------------------------------------------------------------------
//
TUint CRsfwFileEngine::GetDirectoryL(CRsfwFileEntry& /*aFe*/,
                                 TDesC& aFullName,
                                 RFile& /*aF*/,
                                 RPointerArray<CRsfwDirEnt>* aDirEntsp,
                                 MRsfwRemoteAccessResponseHandler* aCaller)
    {
    return RemoteAccessL()->GetDirectoryL(aFullName, *aDirEntsp, aCaller);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::BuildContainerPathL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::BuildContainerPathL(CRsfwFileEntry& aFe, TDes& aPath)
    {
    if (aPath.MaxLength() < (aPath.Length() + iCacheRoot.Length()))
        {
        aPath.Copy(iCacheRoot);
        }
    else 
        {
        User::Leave(KErrOverflow);
        }

    ApplyMultiDirCacheL(aPath);
    // This filename tagging based on container type is just for convenience
    if (aFe.Type() == KNodeTypeFile)
        {
        aPath.Append('F');
        }
    else
        {
        aPath.Append('D');
        }
    aPath.AppendNum((TInt)aFe.Fid().iNodeId);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::ApplyMultiDirCachePathL
// Due to Symbian performance problems with huge directories, items will not
// be stored in one directory in the cache.
// Now instead one dir like:
// C:\system\data\rsfw_cache\C16
// there will be dirs like:
// C:\system\data\rsfw_cache\C16\M0
// C:\system\data\rsfw_cache\C16\M1
// ... and so on
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::ApplyMultiDirCacheL(TDes& aPath)
    {
    // maximum number of items in a single dir in the cache
    const TInt KRsfwMaxItemsInDir = 100;
    TInt i;
    // this loop will surely break (or leave) at some point
    for ( i = 0; ; i++ )
        {
        // create path like "C:\system\data\rsfw_cache\C16\M0"
        HBufC* trypath = HBufC::NewMaxL(KMaxPath);
        TPtr pathPtr = trypath->Des();
        pathPtr.Copy(aPath);
        pathPtr.Append('M');
        pathPtr.AppendNum(i);
        pathPtr.Append('\\');

        // check whether dir exists and if so, how many items it contains
        CDir* dir = NULL;
        // note that KEntryAttDir att means files & directories
        TInt err = iFs.GetDir(*trypath, KEntryAttDir, ESortNone, dir);
        if ( err == KErrNone )
            {
            // count the items
            TInt count = dir->Count();
            delete dir;
            dir = NULL;
            
            //limit is not exceeded -> return the path
            if ( count < KRsfwMaxItemsInDir )
                {
                aPath.Copy(pathPtr);
                delete trypath;
                break;
                }
            // limit exceeded -> let's try the next dir
            else
                {
                delete trypath;
                continue;
                }    
            }        
        else if ( err == KErrPathNotFound )
            {
            // create dir and return the path to empty dir
            err = iFs.MkDir(*trypath);
            if (!err) 
                {
                aPath.Copy(pathPtr);
                delete trypath;
                }
            else 
                {
                delete trypath;
                DEBUGSTRING(("Error when creating cache dir! err=%d", err));
                User::Leave(KErrGeneral);
                }
   
            break;
            }
        else
            {
            delete trypath;
            DEBUGSTRING(("Cache directory cannot be created! err=%d", err));        
            User::Leave(KErrGeneral);
            }    
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::CreateContainerFileL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::CreateContainerFileL(CRsfwFileEntry& aFe,
                                       TDes& aPath,
                                       RFile& aF)
    {
    // Create a container file for the Fid.
    // If the cache file already exists, it will be deleted

    BuildContainerPathL(aFe, aPath);

    TInt err = aF.Replace(iFs, aPath, EFileShareAny | EFileWrite);
    if (err != KErrNone)
        {
        User::Leave(KErrGeneral);
        }
    aF.Close();

    aFe.SetCacheFileName(&aPath);
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::DoIoctlL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::DoIoctlL(TRfeIoctlInArgs& aIn, TRfeOutArgs& /* aOut */)
    {
    TFid fidp = aIn.iFid;
    TInt cmd = aIn.iCmd;

    TInt err = KErrNone;

    DEBUGSTRING(("ioctl fid %d - command=%d, data=%d",
                 fidp.iNodeId,
                 cmd,
                 aIn.iData32[0]));

    CRsfwFileEntry* fep = iFileTable->Lookup(fidp);
    if (fep)
        {
        switch (cmd)
            {
        case ERemoteFsIoctlRefresh:


            if (fep->Type() == KNodeTypeFile)
                {

                fep->SetCacheFileName(NULL);
                fep->SetCached(EFalse);

                // There is a change in the parent's container
                fep->Parent()->SetLocallyDirty();
                }
            break;

        case ERemoteFsHighCachePriority:
        default:
            err = KErrArgument;
            break;
            }
        }
    else
        {
        err = KErrNotFound;
        }

    if (err != KErrNone)
        {
        User::Leave(err);
        }

    return;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::DoRootL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::DoRootL(TRfeRootInArgs& /* aIn */, TRfeRootOutArgs& aOut)
    {
    SetupRootL(iVolume->iVolumeTable->iPermanence);
    aOut.iFid.iVolumeId = iRootFid->iVolumeId;
    aOut.iFid.iNodeId = iRootFid->iNodeId;
    return;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::DoSetAttrL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::DoSetAttrL(TRfeSetAttrInArgs& aIn, TRfeOutArgs& /* aOut */)
    // We cannot really set anything but this is the way to implement this
    // note that if this is implemented, it should really be a state machine
    {
    TInt err = KErrNone;
    TFid fidp = aIn.iFid;
#ifdef _DEBUG
    TDirEntAttr* attrp = &(aIn.iAttr);
#endif

    DEBUGSTRING(("setting attributes of fid %d, attr=0x%x, size=%d, time=",
                 fidp.iNodeId,
                 attrp->iAtt,
                 attrp->iSize));
    DEBUGTIME((attrp->iModified));

    // Get the file or directory to setattr
    CRsfwFileEntry* fep = iFileTable->Lookup(fidp);
    if (fep)
        {
        err = KErrNotSupported;
        }
    else
        {
        err = KErrNotFound;
        }

    User::Leave(err);
    return;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::SetupRootL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::SetupRootL(TBool aPermanence)
    {
    _LIT(KRootPath, ".");  // dummy

    if (!iRootFid)
        {
        CRsfwFileEntry* root = NULL;
        TInt err;
        if (aPermanence)
            {
            TRAP(err, root = iFileTable->LoadMetaDataL());
            }
        if (err == KErrCorrupt)    
            {
            DEBUGSTRING(("Metadata corrupted! Recreating cache file..."));
            // corrupted cache file, recreate filetable and cache file
            delete iFileTable;
            iFileTable = NULL;
            CleanupCorruptedCacheL();
            PrepareCacheL();
            iFileTable = CRsfwFileTable::NewL(iVolume, iCacheRoot);
            }
        if (!aPermanence || (err != KErrNone))
            {
            root = CRsfwFileEntry::NewL(KRootPath, NULL);
            // Insert root into the file table
            iFileTable->AddL(root);
            root->SetType(KNodeTypeDir);
            }
        if (aPermanence)
            {
            iFileTable->SaveMetaDataDelta();
            }
        iRootFep = root;
        iRootFid = &(root->Fid());
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::CleanupCorruptedCacheL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::CleanupCorruptedCacheL()
    {    
    // delete everything from the cache
    TFileName cachepath;
    cachepath.Copy(iCacheRoot);
    CFileMan* fileMan = CFileMan::NewL(iFs);
    fileMan->Delete(cachepath, CFileMan::ERecurse);
    delete fileMan;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::ConnectL
// ----------------------------------------------------------------------------
//
TUint CRsfwFileEngine::ConnectL(TBool aRestart, CRsfwRfeStateMachine* aCaller)
    {
    // Assume parameter format:
    // protocol://username:password@server:port/rootdir or
    // The ":password", ":port", and "[/]rootdir" can be omitted.
    // If the length of password parameter is bigger than 1,
    // it overrides the one in uri, if any.
    // Characters can be quoted with %<hexdigit><hexdigit> format
    TUint transactionId = 0;

    if (iRemoteAccess)
        {
        // We already have a remote accessor
        if (aRestart)
            {
            // Restarting
            delete iLockManager;
            iLockManager = NULL;
            delete iRemoteAccess;
            iRemoteAccess = NULL;
            }
        else
            {
            User::Leave(KErrAlreadyExists);
            }
        }

    DEBUGSTRING16(("ConnectL(): '%S'",
                   &iVolume->iMountInfo.iMountConfig.iUri));

    TUriParser uriParser;
    User::LeaveIfError(uriParser.Parse(iVolume->iMountInfo.iMountConfig.iUri));

    TPtrC userName;
    TPtrC password;
    TPtrC friendlyName;

    if (uriParser.IsPresent(EUriUserinfo))
        {
        TPtrC userInfo(uriParser.Extract(EUriUserinfo));
        // Split the user info into user name and password (seprated by ':')
        TInt pos = userInfo.Locate(':');
        if (pos != KErrNotFound)
            {
            password.Set(userInfo.Mid(pos + 1));
            userName.Set(userInfo.Left(pos));
            }
        else
            {
            userName.Set(userInfo);
            }
        }

    HBufC* userNameBuf = NULL;
    if (!userName.Length() &&
        iVolume->iMountInfo.iMountConfig.iUserName.Length())
        {
        // separate user name overwrites the username embedded in the URI
        userName.Set(iVolume->iMountInfo.iMountConfig.iUserName);
        }

    HBufC* passwordBuf = NULL;
    if (!password.Length() &&
        (iVolume->iMountInfo.iMountConfig.iPassword.Length() > 1))
        {
        // separate password overwrites the password embedded in the URI
        password.Set(iVolume->iMountInfo.iMountConfig.iPassword);
        }

    friendlyName.Set(iVolume->iMountInfo.iMountConfig.iName);

    TPtrC scheme(uriParser.Extract(EUriScheme));
    HBufC8* protocol = HBufC8::NewLC(scheme.Length());
    TPtr8 protocolPtr = protocol->Des();
    protocolPtr.Copy(scheme);
    iRemoteAccess = CRsfwRemoteAccess::NewL(protocolPtr);
    CleanupStack::PopAndDestroy(protocol);

    // user name and password are conveyed separately from the URI
    CUri* uri = CUri::NewLC(uriParser);
    uri->RemoveComponentL(EUriUserinfo);

    // leaves if error
    iRemoteAccess->SetupL(this);
    transactionId = iRemoteAccess->
        OpenL(uri->Uri(),
              friendlyName,
              userName,
              password,
              iVolume->iMountInfo.iMountConfig.iAuxData,
              aCaller);

    CleanupStack::PopAndDestroy(uri);
    if (passwordBuf)
        {
        CleanupStack::PopAndDestroy(passwordBuf);
        }
    if (userNameBuf)
        {
        CleanupStack::PopAndDestroy(userNameBuf);
        }

    // lock manager can be created before we know whether connecting was
    // succesful - however it must be deleted upon unsuccesful connect
    if (!iLockManager)
        {
        iLockManager = CRsfwLockManager::NewL(iRemoteAccess);
        }

    if ((iInactivityTimeout > 0) && !iInactivityTimer)
        {
        iInactivityTimer = CPeriodic::NewL(CActive::EPriorityLow);
        }
    return transactionId;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::DisconnectL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::DisconnectL()
    {
    DEBUGSTRING(("CRsfwFileEngine::DisconnectL"));
    if (iRemoteAccess) 
        {
        iRemoteAccess->Cancel(0);
        delete iRemoteAccess;
        iRemoteAccess = NULL;
        }

    if (iLockManager) 
        {
        delete iLockManager;
        iLockManager = NULL;
        }

    
    // Set open file count to zero
    // If there are open files, after disconnecting we do not necessarily
    // get close events.
    // Note that this variable is not "dirty bit" (file has currently
    // uncommitted modifications), so it is safe to set it to zero
    TInt openfiles = iFileTable->OpenFileCount();
    iFileTable->UpdateOpenFileCount(-openfiles);
        
    EnteredConnectionStateL(KMountNotConnected, ETrue);
    
    // publish connection status when disconnecting
    iVolume->iVolumeTable->PublishConnectionStatus(iVolume);
   
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::StartInactivityTimer
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::StartInactivityTimer()
    {
    if (iInactivityTimer)
        {
        DEBUGSTRING(("inactivity timer started (%d us)",
                     iInactivityTimeout));
        iInactivityTimer->Cancel();
        TCallBack callBack(CRsfwFileEngine::InactivityTimerExpired, this);
        iInactivityTimer->Start(iInactivityTimeout,
                                iInactivityTimeout,
                                callBack);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::StopInactivityTimer
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::StopInactivityTimer()
    {
    DEBUGSTRING(("CRsfwFileEngine::StopInactivityTimer"));
    if (iInactivityTimer)
        {
        DEBUGSTRING(("inactivity timer stopped"));
        iInactivityTimer->Cancel();
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::InactivityTimerExpired
// ----------------------------------------------------------------------------
//
TInt CRsfwFileEngine::InactivityTimerExpired(TAny* aArg)
    {
    DEBUGSTRING(("CRsfwFileEngine::InactivityTimerExpired"));
    CRsfwFileEngine* fileEngine = static_cast<CRsfwFileEngine*>(aArg);
    if (fileEngine->iFileTable->OpenFileCount() == 0) 
        {
        fileEngine->StopInactivityTimer();
        TRAP_IGNORE(fileEngine->DisconnectL());
        // "Simulate" operation completion (which may result in RFE shutdown)
        fileEngine->OperationCompleted();
        }
    else 
        {
        // if there are open files on this volume, just restart the inactivity timer
        fileEngine->StartInactivityTimer();
        }

    return 0;
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::HandleRemoteAccessEventL
// ----------------------------------------------------------------------------
//
void CRsfwFileEngine::HandleRemoteAccessEventL(TInt aEventType,
                                           TInt aEvent,
                                           TAny* /* aArg */)
    {
    DEBUGSTRING(("Handle remote access event: %d/%d in connection state %d",
                 aEventType,
                 aEvent,
                 iConnectionState));
    switch (aEventType)
        {
    case ERsfwRemoteAccessObserverEventConnection:
        switch (aEvent)
            {
        case ERsfwRemoteAccessObserverEventConnectionDisconnected:
            EnteredConnectionStateL(KMountNotConnected, EFalse);
            break;

        case ERsfwRemoteAccessObserverEventConnectionWeaklyConnected:
#if 0
            // This event does not appear
            EnteredConnectionStateL(KMountWeaklyConnected, EFalse);
#endif
            break;

        case ERsfwRemoteAccessObserverEventConnectionStronglyConnected:
#if 0
            // This event does not appear
            EnteredConnectionStateL(KMountStronglyConnected, EFalse);
#endif
            break;

        default:
            break;
            }
        break;

    default:
        break;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwFileEngine::PurgeFromCacheL
// ----------------------------------------------------------------------------
//
TInt CRsfwFileEngine::PurgeFromCache(const TDesC& aPath) 
    {
    // get the fid of the entry for which the cached data is removed
    CRsfwFileEntry* targetFid = FetchFep(aPath);
    if (!targetFid) 
        {
        return KErrPathNotFound;
        }
    // only directories can be refreshed currently    
    if (targetFid->Type() != KNodeTypeDir) 
        {
        return KErrArgument;
        }
    targetFid->SetCached(EFalse);    
    return KErrNone;
    }
                                           
CRsfwFileEntry* CRsfwFileEngine::FetchFep(const TDesC& aPath) 
    {
    DEBUGSTRING16(("CRsfwFileEngine::FetchFep for file %S", &aPath));
    if (aPath.Length() <= 1)
        {
        DEBUGSTRING(("returning rootFep"));
        return iRootFep;
        }
    else 
        {
        TInt delimiterPos = aPath.LocateReverse(KPathDelimiter);
        if (delimiterPos == (aPath.Length() - 1))
            {
            // The path ends with a slash,
            //i.e. this is a directory - continue parsing
            TPtrC nextdelimiter;
            nextdelimiter.Set(aPath.Left(delimiterPos));
            delimiterPos = nextdelimiter.LocateReverse(KPathDelimiter);
            }
        TPtrC entry(aPath.Right(aPath.Length() - (delimiterPos + 1)));
        TPtrC path(aPath.Left(delimiterPos + 1));
        
        // strip a trailing backslash if found
        delimiterPos = entry.LocateReverse(KPathDelimiter);
        if (delimiterPos == (entry.Length() - 1)) 
            {
            TPtrC stripped(entry.Left(entry.Length() - 1));
            return (FetchFep(path)->FindKidByName(stripped));
            }
           else 
            {
            return (FetchFep(path)->FindKidByName(entry));
            }
        
        }

    }
    
HBufC8* CRsfwFileEngine::GetContentType(TDesC& aName)
     {
     TInt err;
     RApaLsSession lsSession;
     err = lsSession.Connect();
     if (err) 
         {
         return NULL;
         }
         
     RFs fsSession;
     err = fsSession.Connect();
     if (err) 
         {
         lsSession.Close();
         return NULL;
         }
     fsSession.ShareProtected();
     TDataRecognitionResult dataType;
     RFile theFile;
     // the mode must mach the mode that is used in the file system plugin
     // (EFileWrite|EFileShareAny)
     err = theFile.Open(fsSession, aName, EFileWrite|EFileShareAny);
     if (err) 
         {
         lsSession.Close();
         fsSession.Close();
         return NULL;
         }
     err = lsSession.RecognizeData(theFile, dataType);
     lsSession.Close();
     theFile.Close();
     fsSession.Close();
     if (err) 
         {
         return NULL;
         }
      
     return dataType.iDataType.Des8().Alloc();
     }    

 // The purpose of these functions is to give capability info
 // for the access protocol plugin used.
 
 // Currently this information is hard coded and takes into account webdav and upnp
 // access modules. New function should be added to the access plugin api to get
 // this information from the protocol module
    
  // whether getting the directory listing also gives reliable file metadata
TBool CRsfwFileEngine::DirectoryListingContainsFileMetadata() 
    {
    _LIT(KUPnP, "upnp");
    if (iVolume->MountInfo()->iMountConfig.iUri.Left(4) == KUPnP)
        {
        return EFalse;
        }
    else 
        {
        return ETrue;
        }
    
    }

