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
* Description:  Client side of Remote Storage FW API access functions.
 *
*/


// INCLUDE FILES
#include "rsfwsession.h"
#include "rsfwinterface.h"

#ifdef __WINS__
#include <e32math.h>
#endif

// CONSTANTS

// Number of message slots to reserve for this client server session.
// Since we only communicate synchronously here, we never have any
// outstanding asynchronous requests.
const TUint KDefaultMessageSlots = 4;

#ifdef __WINS__
const TUint KServerMinHeapSize =   0x1000;  //  4K
const TUint KServerMaxHeapSize = 0x100000;  // 64K
#endif

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// RRsfwSession::RRsfwSession
// C++ default constructor can NOT contain any code, that
// might leave.
// ----------------------------------------------------------------------------
//
EXPORT_C RRsfwSession::RRsfwSession() : RSessionBase()
    {
    }

// ----------------------------------------------------------------------------
// RRsfwSession::Connect
// Connects to the framework by starting the server if neccessary and creating
// a session.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::Connect()
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
// RRsfwSession::Close
// ----------------------------------------------------------------------------
//
EXPORT_C void RRsfwSession::Close()
    {
    }
    
// ----------------------------------------------------------------------------
// RRsfwSession::Version
// Returns the version of Remote File Engine
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TVersion RRsfwSession::Version() const
    {
    return(TVersion(KRfeMajorVersionNumber,
                    KRfeMinorVersionNumber,
                    KRfeBuildVersionNumber));
    }
    
// ----------------------------------------------------------------------------
// RRsfwSession::MoveFids
// Sends the rename operation to Remote File Engine by putting the parameters
// into the shared memory chunk, sending the request and reading the result.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::MoveFids(
    TFid aSourceFid,
    const TDesC& aSourceName,
    TFid aDestFid,
    const TDesC& aDestName,
    TBool aOverWrite) 
    {

    TRfeRenameInArgs* renameArgs = new TRfeRenameInArgs();
    if (!renameArgs) 
        {
        return KErrNoMemory;
        }
    
    //  TRfeRenameInArgs* writePtr = static_cast<TRfeRenameInArgs*>(iWritePtr);
    renameArgs->iOpCode = ERenameReplace;

    renameArgs->iFid = aSourceFid;
    renameArgs->iSrcName.Copy(aSourceName); 
    renameArgs->iDstFid = aDestFid;
    renameArgs->iDstName.Copy(aDestName);
    renameArgs->iOverWrite = aOverWrite;
       
    TPckg<TRfeRenameInArgs> pckgInArgs(*renameArgs);
    
    TInt result = SendRequest(ERenameReplace, aSourceFid.iVolumeId, 
                              TIpcArgs(&pckgInArgs));
    delete renameArgs;
    return result;
    }
    
// ----------------------------------------------------------------------------
// RRsfwSession::SetEntry
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//    
EXPORT_C TInt RRsfwSession::SetEntry(
    const TFid aFid,
    const TTime& aTime,
    TUint aSetAttMask,
    TUint aClearAttMask)
    {  
    TRfeSetAttrInArgs* setEntryArgs = new TRfeSetAttrInArgs();
    if (!setEntryArgs) 
        {
        return KErrNoMemory;
        }
        
    setEntryArgs->iOpCode = ESetAttr;
    setEntryArgs->iFid = aFid;

    // default: no change
    setEntryArgs->iAttr.iAtt = 0; 
    setEntryArgs->iMask.iAtt = 0; 

    if (aSetAttMask & KEntryAttReadOnly)
        {
        // Set read-only
        setEntryArgs->iAttr.iAtt |= KEntryAttReadOnly;
        setEntryArgs->iMask.iAtt |= KEntryAttReadOnly;
        }
    if (aClearAttMask & KEntryAttReadOnly)
        {
        // Reset read-only
        setEntryArgs->iMask.iAtt |= KEntryAttReadOnly;
        }
         
    // Setting time
    setEntryArgs->iAttr.iModified = aTime;

    // Let's see - we will not want to do anything unless
    // the attributes convey something significant.
    TInt result = KErrNone;
    if (setEntryArgs->iMask.iAtt)
        {
        TPckg<TRfeSetAttrInArgs> pckgInArgs(*setEntryArgs);
        result = SendRequest(ESetAttr, aFid.iVolumeId, TIpcArgs(&pckgInArgs));
        }       
    delete setEntryArgs;   
    return result;
    }
    
// ----------------------------------------------------------------------------
// RRsfwSession::FlushCache
// Flushes the directory cache of a file or directory by putting the parameters
// into the shared memory chunk and sending the request.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::FlushCache(
    TFid& aFid)
    {
    TRfeIoctlInArgs* ioctlArgs = new TRfeIoctlInArgs();
    if (!ioctlArgs) 
        {
        return KErrNoMemory;
        }

    ioctlArgs->iOpCode = EFsIoctl;
    ioctlArgs->iFid = aFid;
    ioctlArgs->iCmd = ERemoteFsIoctlRefresh;
    ioctlArgs->iLen = 0;

    TPckg<TRfeIoctlInArgs> pckgInArgs(*ioctlArgs);
    TInt result = SendRequest(EFsIoctl, aFid.iVolumeId, TIpcArgs(&pckgInArgs));
    delete ioctlArgs;
    return result;
    }
 
// ----------------------------------------------------------------------------
// RRsfwSession::SetHighCachePriority
// Sets higher cache priority for a file by putting the parameters
// into the shared memory chunk and sending the request.
// This feature is not supported currently.
// Perhaps will be implemented in near future.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::SetHighCachePriority(TFid& /* aFid */)
    {
    return KErrNotSupported;
    } 

// ----------------------------------------------------------------------------
// RRsfwSession::GetAttributes
// Sends GetAttr operation to Remote File Engine by putting the parameters
// into the shared memory chunk, sending the request and reading the result. 
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::GetAttributes(
    TFid aFileFid, 
    TEntry& aAttributes)
    {
    TRfeGetAttrInArgs* getattrArgs = new TRfeGetAttrInArgs();
    if (!getattrArgs) 
        {
        return KErrNoMemory;
        }
    TRfeGetAttrOutArgs* getattrOutArgs = new TRfeGetAttrOutArgs();
    if (!getattrOutArgs) 
        {
        return KErrNoMemory;
        }   
        
    getattrArgs->iOpCode = EGetAttr;
    getattrArgs->iFid = aFileFid;
    
    TPckg<TRfeGetAttrInArgs> pckgInArgs(*getattrArgs);
    TPckg<TRfeGetAttrOutArgs> pckgOutArgs(*getattrOutArgs);
    
    TInt result = SendRequest(EGetAttr, aFileFid.iVolumeId, 
                              TIpcArgs(&pckgInArgs, &pckgOutArgs));
    
    if (result == KErrNone) 
        {
        // Note that aAttributes.iType (the entry UID)
        // should only be set for a file whose
        // size is greater than or equal to sizeof(TCheckedUid).
        aAttributes.iAtt = getattrOutArgs->iAttr.iAtt;
        aAttributes.iSize = getattrOutArgs->iAttr.iSize;
        aAttributes.iModified = getattrOutArgs->iAttr.iModified;
        aAttributes.iType = KNullUid;       
        }

    delete getattrArgs;
    delete getattrOutArgs;
    return result;
    }
    
// ----------------------------------------------------------------------------
// RRsfwSession::OpenByPathL
// Sends OpenByPath operation to Remote File Engine by putting the parameters
// into the shared memory chunk, sending the request and reading the result. 
// Remote File Engine returns the path of the cache container file for this fid
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::OpenByPath(
    TFid aFid,
    TDes& aContainerPath,
    TDirEntAttr* aAttributes,
    TBool aTrueOpen)
    {
    TRfeOpenByPathInArgs* openbypathArgs = new TRfeOpenByPathInArgs();
    if (!openbypathArgs) 
        {
        return KErrNoMemory;
        }
    TRfeOpenByPathOutArgs* openbypathOutArgs = new TRfeOpenByPathOutArgs();
    if (!openbypathOutArgs) 
        {
        return KErrNoMemory;
        }
        
    openbypathArgs->iOpCode = EOpenByPath;
    openbypathArgs->iFid = aFid;

    // Flag field is used to pass attributes,
    // which indicate which lock should be obtained for the file
    if (aAttributes)
        {
        openbypathArgs->iFlags = aAttributes->iAtt;
        }
    else
        {
        openbypathArgs->iFlags = 0;
        }

    // tells whether the file is really opened or do we need it because 
    // ReadSection() was called... (in the latter case Symbian File Server 
    // does not open the file...) */
    openbypathArgs->iTrueOpen = aTrueOpen;
    
    TPckg<TRfeOpenByPathInArgs> pckgInArgs(*openbypathArgs);
    TPckg<TRfeOpenByPathOutArgs> pckgOutArgs(*openbypathOutArgs);
    
    TInt result = SendRequest(EOpenByPath, aFid.iVolumeId, TIpcArgs(&pckgInArgs, &pckgOutArgs));
    
    if (result == KErrNone) 
        {
        if (aAttributes)
            {
            *aAttributes = openbypathOutArgs->iAttr;
            }
        // Processing the response
        _LIT(KPathRedundancy, "\\.\\");
        TInt j = openbypathOutArgs->iPath.Find(KPathRedundancy);
        if (j != KErrNotFound)
            {
            TInt i = openbypathOutArgs->iPath.Length();
            TInt k;
            for (k = j; k + 2 < i; k++)
                {
                openbypathOutArgs->iPath[k] =
                    openbypathOutArgs->iPath[k + 2];
                }
            openbypathOutArgs->iPath.SetLength(i - 2);
            }
    
        const TInt maximumLength = aContainerPath.MaxLength();
        if (maximumLength >= openbypathOutArgs->iPath.Length())
            {
            aContainerPath.Copy(openbypathOutArgs->iPath);
            }
        else
            {
            aContainerPath.Copy(
                openbypathOutArgs->iPath.Left(maximumLength));
            }
        }
    delete openbypathArgs;
    delete openbypathOutArgs;
    return result;
    }

// ----------------------------------------------------------------------------
// RRsfwSession::RfeInit
// Gets the fid of the root of the mount for Remote File Engine by putting the 
// parameters into the shared memory chunk, sending the request and reading 
// the result.  
// This allows us to get other fids by lookup(). 
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::RfeInit(TFid& aRootFid)
    {
    TRfeRootInArgs* rootArgs = new TRfeRootInArgs();
    if (!rootArgs) 
        {
        return KErrNoMemory;
        }
    
    TRfeRootOutArgs* rootOutArgs = new TRfeRootOutArgs();
    if (!rootOutArgs) 
        {
        return KErrNoMemory;
        }
        
    rootArgs->iOpCode = EFsRoot;
    rootArgs->iFid.iVolumeId = 0;
    rootArgs->iFid.iNodeId = 0;
    
    TPckg<TRfeRootInArgs> pckgInArgs(*rootArgs);
    TPckg<TRfeRootOutArgs> pckgOutArgs(*rootOutArgs);
    
    TInt result = SendRequest(EFsRoot, aRootFid.iVolumeId, 
                              TIpcArgs(&pckgInArgs, &pckgOutArgs));
   
    if (result == KErrNone) 
        {
        aRootFid  = rootOutArgs->iFid;
        }
 
    delete rootArgs;
    delete rootOutArgs;
    return result;    
    }
    
// ----------------------------------------------------------------------------
// RRsfwSession::MakeDirectoryL
// Sends MkDir operation to Remote File Engine by putting the parameters
// into the shared memory chunk, sending the request and reading the result. 
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::MakeDirectory(
    TFid aParentFid, 
    const TDesC& aDirName)
    {
    TRfeMkdirInArgs* mkdirArgs = new TRfeMkdirInArgs();
    if (!mkdirArgs) 
        {
        return KErrNoMemory;
        }   
    
    mkdirArgs->iOpCode = EMkDir;
    mkdirArgs->iFid = aParentFid;

    mkdirArgs->iEntry.iName.Copy(aDirName);
    mkdirArgs->iEntry.iAttr.iAtt = 0; // not read only
    
    TPckg<TRfeMkdirInArgs> pckgInArgs(*mkdirArgs);
    TInt result = SendRequest(EMkDir,
                              aParentFid.iVolumeId,
                              TIpcArgs(&pckgInArgs));
    delete mkdirArgs;
    return result;
    }

// ----------------------------------------------------------------------------
// RRsfwSession::RemoveDirectoryL
// Sends Remove Directory operation to Remote File Engine by putting the 
// parameters into the shared memory chunk, sending the request and reading 
// the result. 
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::RemoveDirectory(
    TFid aParentFid, 
    const TDesC& aDirName)
    {
    TRfeRmdirInArgs* rmdirArgs = new TRfeRmdirInArgs();
    if (!rmdirArgs) 
        {
        return KErrNoMemory;
        }
       
    rmdirArgs->iOpCode = ERemoveDir;
    rmdirArgs->iFid = aParentFid;

    rmdirArgs->iName.Copy(aDirName);
    
    TPckg<TRfeRmdirInArgs> pckgInArgs(*rmdirArgs);
    TInt result = SendRequest(ERemoveDir,
                              aParentFid.iVolumeId,
                              TIpcArgs(&pckgInArgs));
    
    delete rmdirArgs;
    return result;
    }

// ----------------------------------------------------------------------------
// RRsfwSession::CreateFileL
// Sends Create File operation to Remote File Engine by putting the parameters
// into the shared memory chunk, sending the request and reading the result.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::CreateFile(
    TFid aParentFid,
    const TDesC& aFileName,
    TUint aMode,
    TUint aExcl,
    TFid& aNewFid)
    {
    TRfeCreateInArgs* createArgs = new TRfeCreateInArgs();
    if (!createArgs) 
        {
        return KErrNoMemory;
        }
    
    TRfeCreateOutArgs* createOutArgs = new TRfeCreateOutArgs();
    if (!createOutArgs) 
        {
        return KErrNoMemory;
        }
    
    createArgs->iOpCode = ECreateFile;
    createArgs->iFid = aParentFid;

    createArgs->iEntry.iName.Copy(aFileName);

    if (aMode & EFileWrite)
        {
        createArgs->iEntry.iAttr.iAtt = 0;
        }
    else
        {
        createArgs->iEntry.iAttr.iAtt = KEntryAttReadOnly;
        }
    createArgs->iEntry.iAttr.iSize = 0;
    createArgs->iEntry.iAttr.iModified = 0;

    createArgs->iExcl = aExcl;

    TPckg<TRfeCreateInArgs> pckgInArgs(*createArgs);
    TPckg<TRfeCreateOutArgs> pckgOutArgs(*createOutArgs);
    TInt result = SendRequest(ECreateFile,
                              aParentFid.iVolumeId,
                              TIpcArgs(&pckgInArgs, &pckgOutArgs));
    
    if (result == KErrNone) 
        {
        aNewFid = createOutArgs->iFid;
        }
        
    delete createArgs;
    delete createOutArgs;   
    return result;
    }

// ----------------------------------------------------------------------------
// RRsfwSession::RemoveFileL
// Sends Remove File operation to Remote File Engine by putting the parameters
// into the shared memory chunk, sending the request and reading the result.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::RemoveFile(
    TFid aParentFid, 
    const TDesC& aFileName)
    {
    TRfeRemoveInArgs* removeArgs = new TRfeRemoveInArgs();
    if (!removeArgs) 
        {
        return KErrNoMemory;
        }
    
    removeArgs->iOpCode = ERemove;
    removeArgs->iFid = aParentFid;

    removeArgs->iName.Copy(aFileName);

    TPckg<TRfeRemoveInArgs> pckgInArgs(*removeArgs);
    TInt result = SendRequest(ERemove,
                              aParentFid.iVolumeId,
                              TIpcArgs(&pckgInArgs));
    
    delete removeArgs;
    return result;
    }
 
// ----------------------------------------------------------------------------
// RRsfwSession::LookupL
// Sends Lookup operation to Remote File Engine by putting the parameters
// into the shared memory chunk, sending the request and reading the result.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::Lookup(
    TFid aParentFid, 
    const TDesC& aName, 
    TUint aNodeType,
    TFid& aFid)
    {
    TBool directory = EFalse;

    TPtrC peek;
    peek.Set(aName.Right(1)); // the last char
    if ((peek.Length() > 0) && (peek[0] == '\\'))
        {
        directory = ETrue;
        }
        
    TRfeLookupInArgs* lookupArgs = new TRfeLookupInArgs();
    if (!lookupArgs) 
        {
        return KErrNoMemory;
        }
    
    TRfeLookupOutArgs* lookupOutArgs = new TRfeLookupOutArgs();
    if (!lookupOutArgs) 
        {
        return KErrNoMemory;
        }   
        
    lookupArgs->iOpCode = ELookUp;
    lookupArgs->iFid = aParentFid;
    lookupArgs->iNodeType = aNodeType;

    lookupArgs->iName.Copy(aName);

    if (directory) 
        {
        // We don't want to copy the trailing backslash
        TInt len = lookupArgs->iName.Length();
        lookupArgs->iName.SetLength(len - 1);
        }
        
    TPckg<TRfeLookupInArgs> pckgInArgs(*lookupArgs);
    TPckg<TRfeLookupOutArgs> pckgOutArgs(*lookupOutArgs);
    TInt result = SendRequest(ELookUp,
                              aParentFid.iVolumeId,
                              TIpcArgs(&pckgInArgs, &pckgOutArgs));
    
    if (result == KErrNone) 
        {
        aFid = lookupOutArgs->iFid;
        }

    if (result == KErrNotFound)
        {
        if (directory)
            {
            return KErrPathNotFound;
            }
        else
            {
            return KErrNotFound;
            }
        }
        
    delete lookupArgs;
    delete lookupOutArgs;
    return result;
    }

// ----------------------------------------------------------------------------
// RRsfwSession::CloseFile
// Sends Close operation to Remote File Engine by putting the parameters
// into the shared memory chunk, sending the request and reading the result.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C void RRsfwSession::CloseFile(
    const TFid aFid, 
    const TUint aFlags)
    {
    // close cannot be called asynchronously in the file server API
    // and cannot return an error code
    // so we make a blind request to the server 
    SendReceive(EClose, TIpcArgs(aFid.iVolumeId, aFid.iNodeId, aFlags));
    }

// ----------------------------------------------------------------------------
// RRsfwSession::FlushL
// This "abuses" close operation code.
// The file is not really closed by the File Server,
// and Remote File Engine only writes the (changed) file back to the 
// remote server, but does not release a possible write lock.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::Flush(
    const TFid aFid,
    TInt aFirstByte,
    TInt aDataLength,
    TInt aTotalSize)
    {
    TRfeFlushInArgs* flushArgs = new TRfeFlushInArgs();
    if (!flushArgs) 
        {
        return KErrNoMemory;
        }
    
    flushArgs->iOpCode = EFlush;
    flushArgs->iFid = aFid;
    flushArgs->iFirstByte = aFirstByte;
    flushArgs->iDataLength = aDataLength;
    flushArgs->iTotalSize = aTotalSize; 
            
    TPckg<TRfeFlushInArgs> pckgInArgs(*flushArgs);
    
    TInt result = SendRequest(EFlush, aFid.iVolumeId, 
                    TIpcArgs(&pckgInArgs));

    delete flushArgs;
    return result;
    }

// ----------------------------------------------------------------------------
// RRsfwSession::Fetch
// Sends Fetch operation to Remote File Engine by putting the parameters
// into the shared memory chunk, sending the request and reading the result.
// Remote File Engine will write data into cache file, which path it knows by
// the fid.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::Fetch(
    TFid aFileFid, 
    TInt aFirstByte, 
    TInt aLastByte,
    TInt& aCachedBytes)
    {
    TRfeFetchInArgs* fetchArgs = new TRfeFetchInArgs();
    if (!fetchArgs) 
        {
        return KErrNoMemory;
        }
    TRfeFetchOutArgs* fetchOutArgs = new TRfeFetchOutArgs();
    if (!fetchOutArgs) 
        {
        return KErrNoMemory;
        }
    
    fetchArgs->iOpCode = EFetch;
    fetchArgs->iFid = aFileFid;
    fetchArgs->iFirstByte = aFirstByte;
    fetchArgs->iLastByte = aLastByte;
    
    TPckg<TRfeFetchInArgs> pckgInArgs(*fetchArgs);
    TPckg<TRfeFetchOutArgs> pckgOutArgs(*fetchOutArgs);
    TInt result = SendRequest(EFetch, aFileFid.iVolumeId, 
                              TIpcArgs(&pckgInArgs, &pckgOutArgs));

    if (result == KErrNone)
        {
        aCachedBytes = fetchOutArgs->iLastByte;
        }
   
    delete fetchArgs;
    delete fetchOutArgs;
    return result;
    } 
 
// ----------------------------------------------------------------------------
// RRsfwSession::FetchData
// Sends Fetch operation to Remote File Engine by putting the parameters
// into the shared memory chunk, sending the request and reading the result.
// Remote File Engine will write data into a temporary cache file,
// valid only for the duration of this request.
// Note that in this case Remote File Engine will read exactly the requested
// amount, so aLastByte is not reset.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::FetchData(
    TFid aFileFid, 
    TInt aFirstByte, 
    TInt aLastByte,
    TDes& aTempFileName,
    TBool& aUseTempPath)
    {
    TRfeFetchDataInArgs* fetchDataArgs = new TRfeFetchDataInArgs();
    if (!fetchDataArgs) 
        {
        return KErrNoMemory;
        }
    
    TRfeFetchDataOutArgs* fetchDataOutArgs = new TRfeFetchDataOutArgs();
    if (!fetchDataOutArgs) 
        {
        return KErrNoMemory;
        }

    fetchDataArgs->iOpCode = EFetchData;
    fetchDataArgs->iFid = aFileFid;
    fetchDataArgs->iFirstByte = aFirstByte;
    fetchDataArgs->iLastByte = aLastByte;
    
    TPckg<TRfeFetchDataInArgs> pckgInArgs(*fetchDataArgs);
    TPckg<TRfeFetchDataOutArgs> pckgOutArgs(*fetchDataOutArgs);
    TInt result = SendRequest(EFetchData, aFileFid.iVolumeId, 
                              TIpcArgs(&pckgInArgs, &pckgOutArgs));
    
    if (result == KErrNone) 
        {
        // Processing the response
        _LIT(KPathRedundancy, "\\.\\");
        TInt j = fetchDataOutArgs->iTempPath.Find(KPathRedundancy);
        if (j != KErrNotFound)
            {
            TInt i = fetchDataOutArgs->iTempPath.Length();
            TInt k;
            for (k = j; k + 2 < i; k++)
                {
                fetchDataOutArgs->iTempPath[k] =
                    fetchDataOutArgs->iTempPath[k + 2];
                }
            fetchDataOutArgs->iTempPath.SetLength(i - 2);
            }
    
        const TInt maximumLength = aTempFileName.MaxLength();
        if (maximumLength >= fetchDataOutArgs->iTempPath.Length())
            {
            aTempFileName.Copy(fetchDataOutArgs->iTempPath);
            }
        else
            {
            aTempFileName.Copy(
                fetchDataOutArgs->iTempPath.Left(maximumLength));
            }
        aUseTempPath = fetchDataOutArgs->iUseTempPath;
        }
    delete fetchDataArgs;
    delete fetchDataOutArgs;
    return result;
    } 
 
// ----------------------------------------------------------------------------
// RRsfwSession::OkToWriteL
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
EXPORT_C TInt RRsfwSession::OkToWrite(
    TFid aFid, 
    TUint aBytes,
    TBool& aOkToWrite)
    {
    TRfeWriteDataInArgs* writedataArgs = new TRfeWriteDataInArgs();
    if (!writedataArgs) 
        {
        return KErrNoMemory;
        }
    
    TRfeWriteDataOutArgs* writedataOutArgs = new TRfeWriteDataOutArgs();
    if (!writedataOutArgs) 
        {
        return KErrNoMemory;
        }   
        
    writedataArgs->iOpCode = EOkToWrite;
    writedataArgs->iFid = aFid;
    writedataArgs->iBytes = aBytes;
    
    TPckg<TRfeWriteDataInArgs> pckgInArgs(*writedataArgs);
    TPckg<TRfeWriteDataOutArgs> pckgOutArgs(*writedataOutArgs);
    TInt result = SendRequest(EOkToWrite, aFid.iVolumeId, 
                              TIpcArgs(&pckgInArgs, &pckgOutArgs));
    
    if (result == KErrNone) 
        {
        aOkToWrite = writedataOutArgs->iOkToWrite;
        }
    
    delete writedataArgs;
    delete writedataOutArgs;
    return result;
    }

// ----------------------------------------------------------------------------
// RRsfwSession::StartServer
// Starts the Remote File Engine if it is not running, uses semaphore to 
// synchronize startup.
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
TInt RRsfwSession::StartServer(const TDesC& aServerName)
    {
    TFindServer findRfe(aServerName);
    TFullName name;

    TInt result = findRfe.Next(name);
    if (result == KErrNone)
        {
        // Server already running
        return KErrNone;
        }

    RSemaphore semaphore;       
    result = semaphore.CreateGlobal(KRfeSemaphoreName, 0);
    if (result != KErrNone)
        {
        return  result;
        }

    result = CreateServerProcess(aServerName);
    if (result != KErrNone)
        {
        semaphore.Close();  
        return  result;
        }
    semaphore.Wait();
    semaphore.Close();       

    return  KErrNone;
    }

// ----------------------------------------------------------------------------
// RRsfwSession::CreateServerProcess
// Starts the Remote File Engine using name to find the binary
// (other items were commented in a header).
// ----------------------------------------------------------------------------
//
TInt RRsfwSession::CreateServerProcess(const TDesC& aServerName)
    {
    // Just load anything that matches with the name
    const TUidType serverUid(KNullUid, KNullUid, KNullUid);

    RProcess server;

    _LIT(KStartCommand, "");
    TInt result = server.Create(aServerName, KStartCommand, serverUid);
    if (result != KErrNone)
        {
        return  result;
        }
    server.Resume();
    server.Close();

    return  KErrNone;
    }

// ----------------------------------------------------------------------------
// RRsfwSession::SendRequest
// ----------------------------------------------------------------------------
//    
TInt RRsfwSession::SendRequest(TInt aOpCode, TInt aDrive, TIpcArgs aArgs) 
    {
    TInt result = SendReceive(aOpCode, aArgs);
    if (result == KErrServerTerminated)
        {
        // try to restart the server
        result = Connect();
        if (result == KErrNone) 
            {
            result = SendReceive(aOpCode, aArgs);
            }   
        }

    // Disable following codes will fix auto connection of rsfw
    /*
    if (result == KErrNotReady) 
        {
        // try to restore the mount
        result = SendReceive(EMountByDriveLetter, TIpcArgs(aDrive));
        if (result == KErrNone) 
            {
            result = SendReceive(aOpCode, aArgs);
            }   
        }
    */
    
    return result;
    }

//  End of File 
