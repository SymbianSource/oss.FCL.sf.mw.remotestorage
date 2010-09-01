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
* Description:  The definitions of file operation codes and their parameters 
 *
*/


#ifndef RSFWINTERFACE_H
#define RSFWINTERFACE_H

// INCLUDES
#include <f32file.h>
#include <s32strm.h>

#include "rsfwcommon.h"
#include "rsfwfile.h"

// CONSTANTS
_LIT(KRSFWDefaultDrive, "C:");
_LIT(KCacheRootDefault, "\\system\\data\\rsfw_cache\\");
_LIT(KTempFileName, "temp");
_LIT(KRsfwConfigPath, "\\data\\rscfw.cfg");
_LIT(KResolutionLogPath, "\\logs\\rsfw\\rsfw.log");
_LIT(KConfigNameCacheRoot, "cache_root");
_LIT(KDirReadAllMask, "*");
_LIT(KRemoteFs, "eremotefs");
_LIT(KRemoteFSName, "RemoteFS");
_LIT(KPanicSource, "RemoteFS");
_LIT(KRemoteVolumeName, "RemoteFS");
_LIT(KRemoteFEName, "RemoteFE");
_LIT(KCachePanicSource, "cache manager");
_LIT(KRfeInputData, "RemoteFeInputArgsChunk");
_LIT(KRfeOutputData, "RemoteFeOutputArgsChunk");
_LIT(KRfeDataSemaphoreName, "RemoteFeDataSemaphore");
_LIT(KRfeMain, "RemoteFeMain");
_LIT(KRsfwMounterExe, "rsfwbootmounter");

const TInt KMaxMimeTypeLength = 64;
// Node Types
const TUint KNodeTypeUnknown = 0x00;
const TUint KNodeTypeFile    = 0x01;
const TUint KNodeTypeDir     = 0x02;


// DATA TYPES
// Remote File System plug-in <--> Remote File Engine communications
enum TRfeIoctl
    {
    ERemoteFsIoctlRefresh = 1,
    ERemoteFsProtect,
    ERemoteFsHighCachePriority
    };
    
enum TRfeClose
    {
    ECloseNotModified,
    ECloseModified,
    ECloseLastFlushFailed
    };

// CLASS DECLARATIONS

/**
 *  Node identifier in the framework
 *
 *  @lib Rsfwsession.dll
 *  @since Series 60 3.1
 */
class TFid
    {
public:   // New functions
    inline TBool operator==( const TFid& aFid ) const;
public: // Data
    // Identifiers the volume. Each mount is an own volume.
    TInt iVolumeId;
    // Identifies the node (i.e. file or directory) inside the volume.
    TInt iNodeId;
    };

/**
 *  Directory attributes for a file or directory.
 *
 *  @lib rsfwsession.dll
 *  @since Series 60 3.1
 */
class TDirEntAttr
    {
public:   // New functions
    IMPORT_C void ExternalizeL( RWriteStream& aStream ) const;
    IMPORT_C void InternalizeL( RReadStream& aStream  );
    IMPORT_C void Clear();

public:   // Data
    // attribute bits
    TUint iAtt;
    // file size in bytes        
    TInt iSize;      
    // last modified         
    TTime iModified; 
    // Symbian data-type (UID) 
    TUid iUid3;
    };

/**
 *  Encapsulates a file or directory entry.
 *
 *  @lib rsfwsession.dll
 *  @since Series 60 3.1
 */
class TDirEnt
    {
public:   // New functions
    IMPORT_C void ExternalizeL( RWriteStream& aStream ) const;
    IMPORT_C void InternalizeL( RReadStream& aStream );
    IMPORT_C void Clear();

public:   // Data
    // attributes
    TDirEntAttr iAttr; 
    // filename
    TFileName iName;
    };

/**
 *  Remote File Engine input parameters header
 *
 *  @lib rsfwsession.dll
 *  @since Series 60 3.1
 */
class TRfeInArgs
    {
public:  
    // operation code, used to cast this instance into the right impl. class
    TInt iOpCode;
     // The fid of the target file (not used in root operation).
    TFid iFid;
    };
    
/**
 *  Remote File Engine output parameters header
 *
 *  @lib rsfwsession.dll
 *  @since Series 60 3.1
 */
class TRfeOutArgs
    {
public:   // Data
    TUint iUnique;      // The request ID
    };

/**
 *  Remote File Engine operation input and output parameter structures
 *
 *  @lib rsfwsession.dll
 *  @since Series 60 3.1
 */
// Close
class TRfeCloseInArgs : public TRfeInArgs
    {
public:
    TInt iFlags;
    };

// Create
class TRfeCreateInArgs : public TRfeInArgs
    {
public:
    TDirEnt iEntry;
    TInt iExcl;
    };

class TRfeCreateOutArgs : public TRfeOutArgs
    {
public:
    TFid iFid;
    TDirEntAttr iAttr;
    };

// Fetch and cache
class TRfeFetchInArgs : public TRfeInArgs
    {
public:
    TInt iFirstByte;
    TInt iLastByte;
    };

class TRfeFetchOutArgs : public TRfeOutArgs
    {
public:
    // last byte that was actually fetched, might be more than was requested
    TInt iLastByte;
    };

// Fetch without caching
class TRfeFetchDataInArgs : public TRfeInArgs
    {
public:
    TInt iFirstByte;
    TInt iLastByte;
    };
    
class TRfeFetchDataOutArgs : public TRfeOutArgs
    {
public:
    TFileName iTempPath;
    TBool iUseTempPath;
    };  
 
// flush
class TRfeFlushInArgs : public TRfeInArgs
    {
public:
    TInt iFirstByte;
    TInt iDataLength;
    TInt iTotalSize;
    };
    
    
// Fsync
class TRfeFsyncInArgs : public TRfeInArgs
    {
public:
    };

// GetAttr
class TRfeGetAttrInArgs : public TRfeInArgs
    {
public:
    };

class TRfeGetAttrOutArgs : public TRfeOutArgs
    {
public:
    TDirEntAttr iAttr;
    };

// Ioctl
class TRfeIoctlInArgs : public TRfeInArgs
    {
public:
    TInt iCmd;
    TInt iLen;
    union
        {
        TUint8 iData8[1];
        TUint32 iData32[1];
        };
    };

// Lookup
class TRfeLookupInArgs : public TRfeInArgs
    {
public:
    TFileName iName;
    TUint iNodeType;
    };

class TRfeLookupOutArgs : public TRfeOutArgs
    {
public:
    TFid iFid;
    };

// Mkdir
class TRfeMkdirInArgs : public TRfeInArgs
    {
public:
    TDirEnt iEntry;
    };

class TRfeMkdirOutArgs : public TRfeOutArgs
    {
public:
    TFid iFid;
    TDirEntAttr iAttr;
    };

// OpenByPath
class TRfeOpenByPathInArgs : public TRfeInArgs
    {
public:
    TUint iFlags;
    TBool iTrueOpen;
    };


class TRfeOpenByPathOutArgs : public TRfeOutArgs
    {
public:
    TFileName iPath;
    TDirEntAttr iAttr;
    };

// Remove
class TRfeRemoveInArgs : public TRfeInArgs
    {
public:
    TFileName iName;
    };

// Rename
class TRfeRenameInArgs : public TRfeInArgs
    {
public:
    TFileName iSrcName;
    TFid iDstFid;
    TFileName iDstName;
    TBool iOverWrite;
    };

// Rmdir
class TRfeRmdirInArgs : public TRfeInArgs
    {
public:
    TFileName iName;
    };

// Root operation returns the fid of the volume root
class TRfeRootInArgs : public TRfeInArgs
    {
    };

class TRfeRootOutArgs : public TRfeOutArgs
    {
public:
    TFid iFid;
    };

// SetAttr
class TRfeSetAttrInArgs : public TRfeInArgs
    {
public:
    TDirEntAttr iMask;
    TDirEntAttr iAttr;
    };

// Requests whether it is ok to write n bytes to the local cache
class TRfeWriteDataInArgs : public TRfeInArgs
    {
public:
    // Number of bytes to be written.
    TUint iBytes;
    };
  
class TRfeWriteDataOutArgs : public TRfeOutArgs
    {
public:
    // Permission to write.
    TBool iOkToWrite; 
    };

#include "rsfwinterface.inl"

#endif // RSFWINTERFACE_H

// End of File
