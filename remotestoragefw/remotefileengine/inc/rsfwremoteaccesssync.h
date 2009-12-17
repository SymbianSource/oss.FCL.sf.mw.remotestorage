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


#ifndef CRSFWREMOTEACCESSSYNC_H
#define CRSFWREMOTEACCESSSYNC_H

// INCLUDES
#include <e32base.h>

#include "rsfwremoteaccess.h"

// FORWARD DECLARATIONS
class CRsfwRemoteAccess;

// CLASS DECLARATION
/**
 *  Class for accessing files via a file transport protocol
 *  by using synchronous calls.
 *
 *  The derived classes are supposed to be registered and instantiated
 *  by using the the ECOM architecture.
 */

class CRsfwRemoteAccessSync: public CBase, public MRsfwRemoteAccessResponseHandler
    {
public:
    /**
     * Two-phased constructor.
     *
     * @param aProtocol protocol name, like "http", "https", or "ftp"
     * @return a pointer to an object instance that implements
     *   this interface by using the given protocol.
     */
    static CRsfwRemoteAccessSync* NewL(const TDesC8& aProtocol,
                                   CRsfwRemoteAccess* aRemoteAccess = NULL);

    ~CRsfwRemoteAccessSync();

    /**
     * Configures the remote access module.
     * Sets the connection state observer if available
     * In davaccess creates the WebDAV session class
     *
     * @param aRemoteAccess asynchronous accessor (may be NULL)
     * @param aRsfwRemoteAccessObserver connection event observer
     * @return error code 
     */
    TInt Setup(MRsfwRemoteAccessObserver* aRsfwRemoteAccessObserver);
    
    /**
     * Opens a connection to the server given by aServerName parameter.
     *
     * @param aUserName user name for access control (can be empty)
     * @param aPassword password for access control (can be empty)
     * @param aServerName the server's DNS name or IP address
     * @param aPortNumber port number (like 80 for HTTP)
     * @param aRootDirectory sub directory to be accessed (can be empty)
     * @param aAuxData auxiliary parameters for connection setup (IAP info)
     */
    TInt Open(const TDesC& aUserName,
              const TDesC& aPassword,
              const TDesC& aServerName,
              TInt aPortNumber,
              const TDesC& aRootDirectory,
              const TDesC& aAuxData);

    /**
     * Gets contents of the directory given by aPathName parameter.
     *
     * @param aPathName path name of the directory
     * @param aDirEnts an array of directory entries to be filled.
     */
    TInt GetDirectory(const TDesC& aPathName,
                      RPointerArray<CRsfwDirEnt>& aDirEntsp);

    /**
     * Gets attributes of the directory given by aPathName parameter.
     * This function may also be called if the type of the object is
     * not yet known (e.g., the object could be a file).
     *
     * @param aPathName path name of the directory
     * @param aAttr attribute structure to be filled
     */
    TInt GetDirectoryAttributes(const TDesC& aPathName,
                                CRsfwDirEntAttr*& aAttr);
    
    /**
     * Gets attributes of the file given by aPathName parameter.
     *
     * @param aPathName path name of the file
     * @param aAttr attribute structure to be filled
     */
    TInt GetFileAttributes(const TDesC& aPathName,
                           CRsfwDirEntAttr*& aAttr);

    /**
     * Sets attributes of the file or directory given by aPathName parameter.
     * This function is typically only used for files and even then
     * the implementation may do nothing since standard file attributes
     * are implied by the contents of the file or set in conjunction with
     * other operations on the file system object.
     *
     * @param aPathName path name of the file or directory
     * @param aAttr attribute structure
     */
    TInt SetAttributes(const TDesC& aPathName,
                       CRsfwDirEntAttr& aAttr);

    /**
     * Gets a remote file and copies it to a local file.
     * Note that byte ranges are not be implemented by all
     * file access protocols.
     *
     * @param aRemotePathName path name of the remote file
     * @param aLocalPathName path name of the local file
     * @param aOffset offset of the first byte to be accessed
     * @param aLength length of data to be accessed/was accessed (0=all)
     * @param aFlags operation flags (see RemoteAccess.h)
     */
    TInt GetFile(const TDesC& aRemotePathName,
                 const TDesC& aLocalPathName,
                 TInt aOffset,
                 TInt* aLength,
                 TUint aFlags);

    /**
     * Makes a directory.
     *
     * @param aPathName path name of the new directory
     */
    TInt MakeDirectory(const TDesC& aPathName);


    /**
     * Creates an empty file on the remote server
     *
     * @param aPathName path name of the new file
     */
    TInt CreateFile(const TDesC& aPathName);

    /**
     * Puts a file to the server.
     *
     * @param aLocalPathName path name of the local file
     * @param aRemotePathName path name of the remote file
     */
    TInt PutFile(const TDesC& aLocalPathName,
                 const TDesC& aRemotePathName);

    /**
     * Deletes a directory.
     *
     * @param aPathName path name of the directory to be deleted
     */

    TInt DeleteDirectory(const TDesC& aPathName);

    /**
     * Deletes a file.
     *
     * @param aPathName path name of the file to be deleted
     */
    TInt DeleteFile(const TDesC& aPathName);


    /**
     * Renames a file or a directory.
     * (may involve movement to another directory).
     *
     * @param aSrcPathName path name of the object to be renamed
     * @param aDstPathName new path name of the object
     * @param aOverwrite allow overwriting an existing object
     */
    TInt Rename(const TDesC& aSrcPathName,
                const TDesC& aDstPathName,
                TBool aOverwrite);

    /**
     * Obtains a lock for the given file system object
     * Note that this function is not be implemented by all
     * file access protocols (e.g., FTP), some protocols only
     * implement write locking (e.g., WebDAV).
     *
     * @param aPathName path name of the object to be locked
     * @param aLockFlags indicates whether a write or read lock is requested
     * @param aTimeout the timeout that is requested and granted
     * @param aLockToken acquired lock token - the caller gets ownership
     */
    TInt ObtainLock(const TDesC& aPathName,
                    TUint aLockFlags,
                    TUint& aTimeout,
                    TDesC8*& aLockToken
                    );

    /**
     * Releases the lock of the given file system object
     * Note that this function is not be implemented by all
     * file access protocols (e.g., FTP).
     *
     * @param aPathName path name of the object to be locked
     */
    TInt ReleaseLock(const TDesC& aPathName);

    /**
     * Refreshes the lock of the given file system object
     * Note that this function is not be implemented by all
     * file access protocols (e.g., FTP).
     *
     * @param aPathName path name of the object to be locked
     * @param aTimeout the timeout that is requested and granted
     * @param aResponseHandler response handler
     */
    TInt RefreshLock(const TDesC& aPathName, TUint& aTimeout);

    // from MRemoteAccessResponseHandler
    virtual void HandleRemoteAccessResponse(TUint aId, TInt aStatus);

    /**
     * Sets lock token for the a given resource
     * This lock token value replaces any previously cached token value
     *
     * @param aPathName path name
     * @param aLockToken lock token
     * @return error code
     */
    TInt SetLockToken(const TDesC& aPathName, const TDesC8& aLockToken);

private:
    void ConstructL(const TDesC8& aProtocol,
                    CRsfwRemoteAccess* aRemoteAccess = NULL);
    TInt Epilog();
    

private:
    CRsfwRemoteAccess*   iRemoteAccess;
    TBool                iOwnRemoteAccess; //  whether we own iRemoteAccess
    CActiveSchedulerWait* iSchedulerWait;
    TInt                 iStatus;          // operation return status
    TBool                iPending;         // is there a pending request
    };

#endif // CRSFWREMOTEACCESSSYNC_H

// End of File
