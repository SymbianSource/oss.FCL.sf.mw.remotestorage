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
* Description:  WebDAV plugin interface for RSFW
 *
*/


#ifndef CRSFWDAVACCESS_H
#define CRSFWDAVACCESS_H

// INCLUDES
//#include <HttpErr.h>

#include "rsfwremoteaccess.h"
#include "rsfwdavsession.h"
//#include "rsfwdavfileinfo.h"
#include "rsfwconnectionmanager.h"

// CONSTANTS
const TInt KCommRetries      = 3;

// DATA TYPES
enum TRemoteAccessOp
    {
    ERemoteAccessOpNone = 0,
    ERemoteAccessOpOpen,
    ERemoteAccessOpGetDirectory,
    ERemoteAccessOpGetDirectoryAttributes,
    ERemoteAccessOpGetFileAttributes,
    ERemoteAccessOpSetAttributes,
    ERemoteAccessOpGetFile,
    ERemoteAccessOpMakeDirectory,
    ERemoteAccessOpCreateFile,
    ERemoteAccessOpPutFile,
    ERemoteAccessOpDeleteDirectory,
    ERemoteAccessOpDeleteFile,
    ERemoteAccessOpRename,
    ERemoteAccessOpObtainLock,
    ERemoteAccessOpReleaseLock,
    ERemoteAccessOpRefreshLock
    };

// FORWARD DECLARATIONS
class CRsfwDavAccessContext;
//class CRsfwDirEnt;
//class CRsfwDirEntAttr;

// CLASS DECLARATION

/**
 *  WebDAV protocol plugin for Rsfw
 *
 *  @lib davaccess.lib
 *  @since Series 60 3.1
 */

class CRsfwDavAccess: public CRsfwRemoteAccess,
                  public MRsfwDavResponseObserver,
                  public MRsfwConnectionObserver
    {
public: // Constructors and destructor
    /**
     * Two-phased constructor.
     */
    static CRsfwDavAccess* NewL();
    
    /**
     * Destructor.
     */
    virtual ~CRsfwDavAccess();

public: // New functions
   
    /**
       Return information about the given object
       @param aPath path of the object
       @return file information
    */
    CRsfwDavFileInfo* DavFileInfoL(const TDesC& aPath);

    /**
       Add information about the given object
       @param aDavFileInfo information about the object
    */
    void AddDavFileInfo(CRsfwDavFileInfo* aDavFileInfo);

    /**
       Remove all information about the given object
       @param aPath path of the object
       @return file information
    */
    void RemoveDavFileInfoL(const TDesC& aPath);

    /**
       Return the WebDAV session object
       @return WebDAV session
    */
    inline CRsfwDavSession* WebDavSession() { return iWebDavSession; };

    /**
       Return next access context id
       @return id
    */
    inline TUint GetNextAccessContextId() { return ++iCurrentDavContextId; };

public: // Functions from base classes
    // From CRsfwRemoteAccess
    void SetupL(MRsfwRemoteAccessObserver* aRsfwRemoteAccessObserver);

	/*  In this plug-in aAuxData is the access point
		 special values for IAP selection
		DefaultPreferences = *
		AskUser = ?
	*/
    TUint OpenL(const TUriC& aUri,
                const TDesC& aFriendlyName,
                const TDesC& aUserName,
                const TDesC& aPassword,
                const TDesC& aAuxData,
                MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint GetDirectoryL(const TDesC& aPathName,
                        RPointerArray<CRsfwDirEnt>& aDirEnts,
                        MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint GetDirectoryAttributesL(
        const TDesC& aPathName,
        CRsfwDirEntAttr*& aAttr,
        MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint GetFileAttributesL(const TDesC& aPathName,
                             CRsfwDirEntAttr*& aAttr,
                             MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint SetAttributesL(const TDesC& aPathName,
                         CRsfwDirEntAttr& aAttr,
                         MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint GetFileL(const TDesC& aRemotePathName,
                   const TDesC& aLocalPathName,
                   TInt aOffset,
                   TInt* aLength,
                   TUint aFlags,
                   MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint PutFileL(const TDesC& aLocalPathName,
                   const TDesC& aRemotePathName,
                   const TDesC8& aMimeType,
                   TInt aOffset,
                   TInt aLength,
                   TInt aTotalLength,
                   MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint PutFileL(const TDesC& aLocalPathName,
                   const TDesC& aRemotePathName,
                   const TDesC8& aMimeType,
                   MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint CreateFileL(const TDesC& aPathName,
                      TBool aIsOverwriting,
                      MRsfwRemoteAccessResponseHandler* aResponseHandler);

    TUint MakeDirectoryL(const TDesC& aPathName,
                         MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint DeleteDirectoryL(const TDesC& aPathName,
                           MRsfwRemoteAccessResponseHandler* aResponseHandler);

    TUint DeleteFileL(const TDesC& aPathName,
                      MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint RenameL(const TDesC& aSrcPathName,
                  const TDesC& aDstPathName,
                  TBool aOverwrite,
                  MRsfwRemoteAccessResponseHandler* aResponseHandler);

    TUint ObtainLockL(const TDesC& aPathName,
                      TUint aLockFlags,
                      TUint& aTimeout,
                      TDesC8*& aLockToken,
                      MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint ReleaseLockL(const TDesC& aPathName,
                       MRsfwRemoteAccessResponseHandler* aResponseHandler);
    
    TUint RefreshLockL(const TDesC& aPathName,
                       TUint& aTimeout,
                       MRsfwRemoteAccessResponseHandler* aResponseHandler);

    void Cancel(TUint aId);
    
    void Cancel(TDesC& aTargetPath);

    TInt SetLockToken(const TDesC& aPathName, const TDesC8& aLockToken);

    // From MRsfwDavResponseObserver
    void RequestCompleteL(TUint aWebDavTransactionId);
    void RequestError(TUint aWebDavTransactionId, TInt aStatus);

    // From MRsfwConnectionObserver
    void HandleConnectionEventL(TInt aConnectionEvent, TAny* aArg);
    
private: 
    void ConstructL();
    TUint AddAccessContext(CRsfwDavAccessContext* aDavAccessContext);
    TInt LookupAccessContextByTransactionId(TUint aWebDavTransactionId);
    TInt LookupAccessContextByContextId(TUint aId);
    TInt LookupAccessContextByPath(TDesC& aTargetPath);
    TInt DavFileInfoIndexL(const TDesC& aPath);
    TUint OptionsL(MRsfwRemoteAccessResponseHandler* aResponseHandler);
    void SetLockTokenL(const TDesC& aPathName, const TDesC8& aLockToken);

private: // Data
    TBuf<KMaxPath>                          iRootDirectory;
    CRsfwDavSession*                         iWebDavSession;
    RPointerArray<CRsfwDavFileInfo>             iDavFileInfos; 
    RPointerArray<CRsfwDavAccessContext>    iDavAccessContexts;
    TUint                                   iCurrentDavContextId;
    MRsfwRemoteAccessObserver*              iRsfwRemoteAccessObserver;
    };

#endif // CRSFWDAVACCESS_H

// End of File
