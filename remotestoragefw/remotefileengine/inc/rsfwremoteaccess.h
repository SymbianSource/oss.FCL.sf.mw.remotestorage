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
* Description:  Defines a class for accessing files via a file transport protocol
*
*/


#ifndef CRSFWREMOTEACCESS_H
#define CRSFWREMOTEACCESS_H

// INCLUDES
#include <e32base.h>
#include <uri16.h>

//FORWARD DECLARATIONS
class CRsfwDirEntAttr;
class CRsfwDirEnt;

// CONSTANTS
// 
// UID of this interface
const TUid KCRemoteAccessUid = {0x101F96E3};
// Default quota and free space sizes
const TInt KMountReportedSize         = 10000000;
const TInt KMountReportedFreeSize     = 5000000;

// Operation option flags
const TUint KRemoteAccessOptionGetToStartOfFile = 0x01;

// DATA TYPES
// Event types for MRsfwRemoteAccessObserver
enum TRsfwRemoteAccessObserverEvent
    {
    ERsfwRemoteAccessObserverEventConnection = 1
    };

// Connection events for MRsfwRemoteAccessObserver
enum TRsfwRemoteAccessObserverEventConnection
    {
    ERsfwRemoteAccessObserverEventConnectionDisconnected = 0,
    ERsfwRemoteAccessObserverEventConnectionWeaklyConnected,
    ERsfwRemoteAccessObserverEventConnectionStronglyConnected
    };

// CONSTANTS
const TInt KMaxMatchStringSize = 64;

// CLASS DECLARATION
/**
 *  Class for handling remote access events
 *
 *  @lib rsfwcommon.lib
 *  @since Series 60 3.1
 */
class MRsfwRemoteAccessObserver
    {
public:
    /**
     * Handles an event emanating from a remote access module.
     *
     * @param aEventType type of the event
     * @param aEvent event code
     * @param aArg miscellaneous arguments
     */
    virtual void HandleRemoteAccessEventL(TInt aEventType,
                                          TInt aEvent,
                                          TAny* aArg) = 0;
    };

// CLASS DECLARATION
/**
 *  Class for handling remote access operation responses.
 *
 *  @lib rsfwcommon.lib
 *  @since Series 60 3.1
 */
class MRsfwRemoteAccessResponseHandler
    {
public:
    /**
     * Handles responses for requests to a remote access module.
     *
     * @param aId transaction id
     * @param aStatus return status
     */
    virtual void HandleRemoteAccessResponse(TUint aId, TInt aStatus) = 0;
    };


// CLASS DECLARATION
/**
 *  Class for accessing files via a file transport protocol, like WebDAV.
 *
 *  @lib rsfwcommon.lib
 *  @since Series 60 3.1
 *
 *  The derived classes are supposed to be registered and instantiated
 *  by using the the ECOM architecture.
 */
class CRsfwRemoteAccess : public CBase
    {
public:
    /**
     * Two-phased constructor.
     *
     * @param aProtocol protocol name, like "http", "https", or "ftp"
     * @return a pointer to an object instance that implements
     *   this interface by using the given protocol.
     */
    IMPORT_C static CRsfwRemoteAccess* NewL(const TDesC8& aProtocol);
    
    IMPORT_C virtual ~CRsfwRemoteAccess();

    /**
     * Set up parameters for operation.
     * @param aRsfwRemoteAccessObserver MRsfwRemoteAccessObserver for receiving
     *  asynchronous events from the accessor plugin,
     *  e.g. changes in connectivity.
     *  This parameter may be NULL
     */
    virtual void SetupL(
        MRsfwRemoteAccessObserver* aRsfwRemoteAccessObserver) = 0;

    /**
     * Opens a connection to the server given by aServerName parameter.
     *
     * @param aUri URI of the remote repository.
     *   The URI must not contain authority part (user name/password)
     * @param aFriendlyName friendly name for the server 
     *                      (for possible access control dialog) (can be empty)
     * @param aUserName user name for access control (can be empty)
     * @param aPassword password for access control (can be empty)
     * @param aAuxData auxiliary parameters for connection setup (eg IAP info)
     * @param aResponseHandler response handler
     * @return identifier of the created transaction 
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint OpenL(const TUriC& aUri,
                        const TDesC& aFriendlyName,
                        const TDesC& aUserName,
                        const TDesC& aPassword,
                        const TDesC& aAuxData,
                        MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Gets contents of the directory given by aPathName parameter.
     *
     * @param aPathName path name of the directory
     * @param aDirentsp an array of directory entries to be filled.
     *   Any pre-existing CRsfwDirEnt items in the array are destroyed 
     *   and the array is reset before filling it with pointers to 
     *   new entries. Within the created CRsfwDirEntAttr objects, pointers 
     *   to descriptors for meta-data items that are not available or 
     *   that are irrelevant are set to NULL value.
     *   The caller owns the array and thus also the entries.
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint GetDirectoryL(const TDesC& aPathName,
                                RPointerArray<CRsfwDirEnt>& aDirEntsp,
                                MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Gets attributes of the directory given by aPathName parameter.
     * This function may also be called if the type of the object is
     * not yet known (e.g., the object could be a file).
     *
     * @param aPathName path name of the directory
     * @param aAttr A pointer to the attribute object to be filled. This 
     *   attribute is set to point to a newly created CRsfwDirEntAttr 
     *   object that will contain the directory attributes.
     *   In the created attribute object, pointers to descriptors for 
     *   meta-data items that are not available or that are irrelevant 
     *   are set to NULL value. The ownership of the object is 
     *   transferred to the caller. If the attributes cannot be defined, 
     *   the pointer will be set to NULL.
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint GetDirectoryAttributesL(const TDesC& aPathName,
                                          CRsfwDirEntAttr*& aAttr,
                                          MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;
    
    /**
     * Gets attributes of the file given by aPathName parameter.
     *
     * @param aPathName path name of the file
     * @param aAttr A pointer to the attribute object to be filled. This 
     *   attribute is set to point to a newly created CRsfwDirEntAttr 
     *   object that will contain the file attributes.
     *   In the created attribute object, pointers to descriptors for 
     *   meta-data items that are not available or that are irrelevant 
     *   are set to NULL value. The ownership of the object is 
     *   transferred to the caller. If the attributes cannot be defined, 
     *   the pointer will be set to NULL.
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint GetFileAttributesL(const TDesC& aPathName,
                                     CRsfwDirEntAttr*& aAttr,
                                     MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Sets attributes of the file or directory given by aPathName parameter.
     * This function is typically only used for files and even then
     * the implementation may do nothing since standard file attributes
     * are implied by the contents of the file or set in conjunction with
     * other operations on the file system object.
     *
     * @param aPathName path name of the file or directory
     * @param aAttr attribute structure
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint SetAttributesL(const TDesC& aPathName,
                                 CRsfwDirEntAttr& aAttr,
                                 MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Gets a remote file and copies it to a local file.
     * Note that byte ranges are not be implemented by all
     * file access protocols. 
     * A non-zero aLength means partial get.
     * Caller can assume that either aLength remains intact 
     * in which case byte range offset + aLength was fetched,
     * or aLength is reset to the full length of the file, in which 
     * case aOffset is meaningless.
     *
     * @param aRemotePathName path name of the remote file
     * @param aLocalPathName path name of the local file
     * @param aOffset offset of the first byte to be accessed
     * @param aLength length of data to be accessed/was accessed
     * (on entry NULL or zero value means fetching the whole file -
     * on exit contains the length of fetched data, unless the pointer is NULL)
     * @param aFlags operation qualifier.
     *   The following flags have been defined:
     *   KRemoteAccessOptionGetToStartOfFile: even if an offset is specified
     *      the fetched data is still put at the beginning of the local file.
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint GetFileL(const TDesC& aRemotePathName,
                           const TDesC& aLocalPathName,
                           TInt aOffset,
                           TInt* aLength,
                           TUint aFlags,
                           MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Puts a range of a file to the server.
     * A non-zero aLength means partial file putting.
     * The access protocol/server doesn't have to support partial file putting.
     * In this case, it should return KErrNotSupported (if aLength is not zero)
     *
     * @param aLocalPathName path name of the local file
     * @param aRemotePathName path name of the remote file
     * @param MIME-type of the file
     *   (will be put to Content-Type, e.g. text/plain or
     *   application/octet-stream)
     * @param aOffset offset of the first byte to be accessed
     * @param aLength length of data to be accessed/was accessed (NULL/0=all)
     * @param aTotalLength total length of the file, set to 0 if not known
     * @param aResponseHandler response handler
     * @return identifier of the created transaction 
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint PutFileL(const TDesC& aLocalPathName,
                           const TDesC& aRemotePathName,
                           const TDesC8& aMimeType,
                           TInt aOffset,
                           TInt aLength,
                           TInt aTotalLength,
                           MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Puts a file to the server.
     *
     * @param aLocalPathName path name of the local file
     * @param aRemotePathName path name of the remote file
     * @param MIME-type of the file (will be put to Content-Type,
     *   e.g. text/plain or application/octet-stream)
     * @param aResponseHandler response handler
     * @return identifier of the created transaction 
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint PutFileL(const TDesC& aLocalPathName,
                           const TDesC& aRemotePathName,
                           const TDesC8& aMimeType,
                           MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Creates an empty file on the remote server
     *
     * @param aPathName path name of the new file
     * @param aResponseHandler response handler
     * @param aOverWriting whether we are overwriting an existing file
     *        Note that the semantics of this operation is such that it must
     *        always overwrite an existing file. This boolean is for information.
     *        If the protocol requires additional parameter to allow overwriting,
     *        the parameter should be set if aOverWriting is TRUE.
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint CreateFileL(const TDesC& aPathName,
                              TBool aOverWriting,
                              MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Makes a directory.
     *
     * @param aPathName path name of the new directory
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint MakeDirectoryL(const TDesC& aPathName,
                                 MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;


    /**
     * Deletes a directory.
     *
     * @param aPathName path name of the directory to be deleted
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */

    virtual TUint DeleteDirectoryL(const TDesC& aPathName,
                                   MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Deletes a file.
     *
     * @param aPathName path name of the file to be deleted
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint DeleteFileL(const TDesC& aPathName,
                              MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;


    /**
     * Renames a file or a directory.
     * (may involve movement to another directory).
     *
     * @param aSrcPathName path name of the object to be renamed
     * @param aDstPathName new path name of the object
     * @param aOverwrite allow overwriting an existing object
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint RenameL(const TDesC& aSrcPathName,
                          const TDesC& aDstPathName,
                          TBool aOverwrite,
                          MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Obtains a lock for the given file system object
     * Note that this function is not be implemented by all
     * file access protocols (e.g. FTP), some protocols only
     * implement write locking (e.g. WebDAV).
     *
     * @param aPathName path name of the object to be locked
     * @param aLockFlags indicates whether a write or read lock is requested
     * @param aTimeout the timeout that is requested and granted (in seconds)
     * @param aLockToken acquired lock token - the caller gets ownership
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint ObtainLockL(const TDesC& aPathName,
                              TUint aLockFlags,
                              TUint& aTimeout,
                              TDesC8*& aLockToken,
                              MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Releases the lock of the given file system object
     * Note that this function is not be implemented by all
     * file access protocols (e.g. FTP).
     *
     * @param aPathName path name of the object to be locked
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint ReleaseLockL(const TDesC& aPathName,
                               MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Refreshes the lock of the given file system object
     * Note that this function is not be implemented by all
     * file access protocols (e.g. FTP).
     *
     * @param aPathName path name of the object to be locked
     * @param aTimeout the timeout that is requested and granted (in seconds)
     * @param aResponseHandler response handler
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    virtual TUint RefreshLockL(const TDesC& aPathName,
                               TUint& aTimeout,
                               MRsfwRemoteAccessResponseHandler* aResponseHandler) = 0;

    /**
     * Cancels a transaction
     * Eventually the HandleRemoteAccessResponseL will be called
     * with status KErrCancel

     * @param aId the identifier of the transaction to be canceled.
     *   If aId is zero, all pending requests are cancelled.
     */
    virtual void Cancel(TUint aId) = 0;

		 /**
     * Cancels a transaction
     * Eventually the HandleRemoteAccessResponseL will be called
     * with status KErrCancel

     * @param aTargetPath the path of the target file or directory for the 
     * operation that shall be cancelled
     * 
     */
    virtual void Cancel(TDesC& aTargetPath) = 0;


    /**
     * Sets lock token for the a given resource
     * This lock token value replaces any previously cached token value
     *
     * @param aPathName path name
     * @param aLockToken lock token
     * @return error code
     */
    virtual TInt SetLockToken(const TDesC& aPathName,
                              const TDesC8& aLockToken) = 0;
                              
     /**
     * Gets quota and size.
     *
     * @param aQuota  The maximum size of the drive for this user in bytes,
     * @param aSize  The amount of free space for this user on the disk in bytes.
     * @return identifier of the created transaction
     *		   (> 0 for async. operations, 0 if the operation is synchronous 
     *		   (has been completed when the call returns)
     */
    IMPORT_C virtual TInt GetQuotaAndSizeL(TInt& aQuota, TInt& aSize);
                     

private:
    // Unique instance identifier key
    TUid iDtor_ID_Key;
    };

#endif // CRSFWREMOTEACCESS_H

// End of File
