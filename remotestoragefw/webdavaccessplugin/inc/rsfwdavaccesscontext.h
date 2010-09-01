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
* Description:  Contexts for keeping transaction state
 *
*/


#ifndef CRSFWDAVACCESSCONTEXT_H
#define CRSFWDAVACCESSCONTEXT_H

// INCLUDES
#include <e32base.h>
//#include <HttpErr.h>

#include "rsfwremoteaccess.h"
//#include "rsfwdavsession.h"
#include "rsfwdavtransaction.h"
//#include "rsfwdavfileinfo.h"

// FORWARD DECLARATIONS
class CRsfwDavAccess;
//class CRsfwDirEntAttr;

// CONSTANTS
const TInt KMaxMimeTypeLength = 64;

// CLASS DECLARATIONS

/**
 *  WebDAV access contexts (state machines)
 *
 *  @lib davaccess.lib
 *  @since Series 60 3.1
 */

class CRsfwDavAccessContext : public CBase
    {
public: // Constructors and destructor
    /**
     * Destructor.
     */
    virtual ~CRsfwDavAccessContext();
    
public: // New functions
     /**
     * This is used for GET and PUT operations, to allow cancelling
     * the operations by target path name (local dialog from File Manager will
     * call CancelByPath)
     * @return TDesC, the target path for the operation
     */
    virtual const TDesC& TargetPath();

    /**
     * Start transaction
     */
    virtual void StartL() = 0;

    /**
     * Receive a notification of a complete transaction
     */
    virtual void TransactionCompleteL() = 0;
    
    /**
     * Receive a notification of a failed transaction
     * @param aError error code
     */
    virtual void TransactionError(TInt aError) = 0;
   

    /**
     * Return the underlying WebDAV transaction
     * @return WebDAV transaction
     */
    inline CRsfwDavTransaction* WebDavTransaction()
        { return iWebDavTransaction; };

    /**
     * Set context id
     * @param aId id
     */
    inline void SetId(TUint aId) { iId = aId; };

    /**
     * Get id of the current WebDAV transaction
     * @return id
     */
    inline TUint WebDavTransactionId() { return iWebDavTransactionId; };

    /**
     * Get context id
     * @return id
     */
    inline TUint Id() { return iId; };

    /**
     * Get context status
     * @return status
     */
    inline TUint Status() { return iStatus; };

    /**
     * Get response handler
     * @return response handler
     */
    inline MRsfwRemoteAccessResponseHandler* ResponseHandler()
        { return iResponseHandler; };

    /**
     * Tell wether the context is in finished state
     * @return ETrue, if the context has finished
     */
    inline TBool Done() { return iDone; };
   
  
protected: // New functions
    /**
     * Submit transaction
     * @return none
     */
    void SubmitL();
    
    /**
     * Retry transaction
     * @return none
     */
    void Retry();
    
    /**
     * Map HTTP error code to SymbianOS error code
     * @param aError, input and output error code
     */
    void MapError(TInt& aError);
    

protected: // Data
    TUint                         iId;
    CRsfwDavAccess*                   iDavAccess;
    TUint                         iStatus;
    MRsfwRemoteAccessResponseHandler* iResponseHandler;
    CRsfwDavTransaction*           iWebDavTransaction;
    TUint                         iWebDavTransactionId;
    TBool                         iDone;
    TInt                          iTryCount;
    // allows to cancel the operation by path
    TBufC<KMaxPath>               iRemotePathName; 

    };

// ---------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------

class CRsfwDavAccessContextOptions: public CRsfwDavAccessContext
    {
public:
    /**
     * Two-phased constructor.
     */
    static CRsfwDavAccessContextOptions* NewL(
        CRsfwDavAccess* aDavAccess,
        MRsfwRemoteAccessResponseHandler* aResponseHandler);

public: // Functions from base classes
    // From CRsfwDavAccessContext
    void StartL();
    void TransactionCompleteL();
    void TransactionError(TInt aError);

private:
    void ConstructL(CRsfwDavAccess* aDavAccess,
                    MRsfwRemoteAccessResponseHandler* aResponseHandler);
    };

// -----------------------------------------------------------------
// PropFindDir
// -----------------------------------------------------------------

class CRsfwDavAccessContextPropFindDir: public CRsfwDavAccessContext
    {
public: // Constructors and destructor
    /**
     * Two-phased constructor.
     * 
     */
    static CRsfwDavAccessContextPropFindDir* NewL(
        CRsfwDavAccess* aDavAccess,
        MRsfwRemoteAccessResponseHandler* aResponseHandler,
        const TDesC& aPathName,
        TInt aDepth,
        CRsfwDirEntAttr** aDirEntAttr,
        RPointerArray<CRsfwDirEnt>* aDirEnts);

    /**
     * Destructor.
     */
    virtual ~CRsfwDavAccessContextPropFindDir();

public: // Functions from base classes
    // From CRsfwDavAccessContext
    void StartL();
    void TransactionCompleteL();
    void TransactionError(TInt aError);

private:
    void ConstructL(CRsfwDavAccess* aDavAccess,
                    MRsfwRemoteAccessResponseHandler* aResponseHandler,
                    const TDesC& aPathName,
                    TInt aDepth,
                    CRsfwDirEntAttr** aDirEntAttr,
                    RPointerArray<CRsfwDirEnt>* aDirEnts);
private: // Data
    TInt                    iDepth;
    CRsfwDirEntAttr**           iDirEntAttr;
    RPointerArray<CRsfwDirEnt>* iDirEnts;
    RPointerArray<CRsfwDirEnt>  iOwnDirEnts;
    };

// -----------------------------------------------------------------
// PropFindFile
// -----------------------------------------------------------------

class CRsfwDavAccessContextPropFindFile: public CRsfwDavAccessContext
    {
public:
    /**
     * Two-phased constructor.
     * 
     */
    static CRsfwDavAccessContextPropFindFile* NewL(
        CRsfwDavAccess* aDavAccess,
        MRsfwRemoteAccessResponseHandler* aResponseHandler,
        const TDesC& aPathName,
        CRsfwDirEntAttr** aDirEntAttr);

    /**
     * Destructor.
     */
    virtual ~CRsfwDavAccessContextPropFindFile();

public: // Functions from base classes
    // From CRsfwDavAccessContext
    void StartL();
    void TransactionCompleteL();
    void TransactionError(TInt aError);

private:
    void ConstructL(CRsfwDavAccess* aDavAccess,
                    MRsfwRemoteAccessResponseHandler* aResponseHandler,
                    const TDesC& aPathName,
                    CRsfwDirEntAttr** aDirEntAttr);

private: // Data
    CRsfwDirEntAttr**           iDirEntAttr;
    RPointerArray<CRsfwDirEnt>  iOwnDirEnts;
    };

// -----------------------------------------------------------------
// Get
// -----------------------------------------------------------------

class CRsfwDavAccessContextGet: public CRsfwDavAccessContext
    {
public:
    /**
     * Two-phased constructor.
     * 
     */
    static CRsfwDavAccessContextGet* NewL(
        CRsfwDavAccess* aDavAccess,
        MRsfwRemoteAccessResponseHandler* aResponseHandler,
        const TDesC& aRemotePathName,
        const TDesC& aLocalPathName,
        TInt aOffset,
        TInt* aLength,
        TUint aFlags);
    void StartL();
    void TransactionCompleteL();
    void TransactionError(TInt aError);

private:
    void ConstructL(CRsfwDavAccess* aDavAccess,
                    MRsfwRemoteAccessResponseHandler* aResponseHandler,
                    const TDesC& aRemotePathName,
                    const TDesC& aLocalPathName,
                    TInt aOffset,
                    TInt* aLength,
                    TUint aFlags);

private: // Data
    TBufC<KMaxPath>  iLocalPathName;
    TInt             iOffset;
    TInt*            iLength;
    TUint            iFlags;
    };

// -----------------------------------------------------------------
// Put
// -----------------------------------------------------------------

class CRsfwDavAccessContextPut: public CRsfwDavAccessContext
    {
public:
    /**
     * Two-phased constructor.
     * 
     */
    static CRsfwDavAccessContextPut* NewL(
        CRsfwDavAccess* aDavAccess,
        MRsfwRemoteAccessResponseHandler* aResponseHandler,
        const TDesC& aLocalPathName,
        const TDesC& aRemotePathName,
        const TDesC8& aMimeType,
        TInt aOffset,
        TInt aLength,
        TInt aTotalLength,
        const TDesC8* aLockToken);

public: // Functions from base classes
    // From CRsfwDavAccessContext
    void StartL();
    void TransactionCompleteL();
    void TransactionError(TInt aError);

private:
    void ConstructL(CRsfwDavAccess* aDavAccess,
                    MRsfwRemoteAccessResponseHandler* aResponseHandler,
                    const TDesC& aLocalPathName,
                    const TDesC& aRemotePathName,
                    const TDesC8& aMimeType,
                    TInt aOffset,
                    TInt aLength,
                    TInt aTotalLength,
                    const TDesC8* aLockToken);

private: // Data
    TBufC<KMaxPath>            iLocalPathName;
    TBufC8<KMaxMimeTypeLength> iMimeType;
    TInt                       iOffset;
    TInt                       iLength;
    TInt                       iTotalLength;
    const TDesC8*              iLockToken;
    
    // The recipient of the entity MUST NOT ignore any Content-*
    // (e.g. Content-Range) headers that it does not understand or implement
    // and MUST return a 501 (Not Implemented) response in such cases.
    TBool            iContentRangeSupported;
    };

// -----------------------------------------------------------------
// MkDir
// -----------------------------------------------------------------

class CRsfwDavAccessContextMkDir: public CRsfwDavAccessContext
    {
public:
    /**
     * Two-phased constructor.
     * 
     */
    static CRsfwDavAccessContextMkDir* NewL(
        CRsfwDavAccess* aDavAccess,
        MRsfwRemoteAccessResponseHandler* aResponseHandler,
        const TDesC& aPathName);

public: // Functions from base classes
    // From CRsfwDavAccessContext
    void StartL();
    void TransactionCompleteL();
    void TransactionError(TInt aError);

private:
    void ConstructL(CRsfwDavAccess* aDavAccess,
                    MRsfwRemoteAccessResponseHandler* aResponseHandler,
                    const TDesC& aPathName);
    };

// -----------------------------------------------------------------
// Delete
// -----------------------------------------------------------------

class CRsfwDavAccessContextDelete: public CRsfwDavAccessContext
    {
public:
    /**
     * Two-phased constructor.
     * 
     */
    static CRsfwDavAccessContextDelete* NewL(
        CRsfwDavAccess* aDavAccess,
        MRsfwRemoteAccessResponseHandler* aResponseHandler,
        const TDesC& aPathName,
        TBool aIsDir,
        const TDesC8* aLockToken);

public: // Functions from base classes
    // From CRsfwDavAccessContext
    void StartL();
    void TransactionCompleteL();
    void TransactionError(TInt aError);

private:
    void ConstructL(CRsfwDavAccess* aDavAccess,
                    MRsfwRemoteAccessResponseHandler* aResponseHandler,
                    const TDesC& aPathName,
                    TBool aIsdir,
                    const TDesC8* aLockToken);

private: // Data
    TBool            iIsDir;
    const TDesC8*    iLockToken;
    };

// -----------------------------------------------------------------
// Move
// -----------------------------------------------------------------

class CRsfwDavAccessContextMove: public CRsfwDavAccessContext
    {
public:
    /**
     * Two-phased constructor.
     * 
     */
    static CRsfwDavAccessContextMove* NewL(
        CRsfwDavAccess* aDavAccess,
        MRsfwRemoteAccessResponseHandler* aResponseHandler,
        const TDesC& aSrcPathName,
        const TDesC& aDstPathName,
        TBool aOverwrite,
        const TDesC8* aSrcLockToken,
        const TDesC8* aDstLockToken);

public: // Functions from base classes
    // From CRsfwDavAccessContext
    void StartL();
    void TransactionCompleteL();
    void TransactionError(TInt aError);

private:
    void ConstructL(CRsfwDavAccess* aDavAccess,
                    MRsfwRemoteAccessResponseHandler* aResponseHandler,
                    const TDesC& aSrcPathName,
                    const TDesC& aDstPathName,
                    TBool aOverwrite,
                    const TDesC8* aSrcLockToken,
                    const TDesC8* aDstLockToken);

private: // Data
    TBufC<KMaxPath>  iSrcPathName;
    const TDesC8*    iSrcLockToken;
    const TDesC8*    iDstLockToken;
    TBool            iOverwrite;
    };

// -----------------------------------------------------------------
// Lock
// -----------------------------------------------------------------

class CRsfwDavAccessContextLock: public CRsfwDavAccessContext
    {
public:
    /**
     * Two-phased constructor.
     * 
     */
    static CRsfwDavAccessContextLock* NewL(
        CRsfwDavAccess* aDavAccess,
        MRsfwRemoteAccessResponseHandler* aResponseHandler,
        const TDesC& aPathName,
        TUint aLockFlags,
        TUint& aTimeout,
        TDesC8** aLockToken);

    /**
     * Destructor.
     */
    virtual ~CRsfwDavAccessContextLock();

public: // Functions from base classes
    // From CRsfwDavAccessContext
    void StartL();
    void TransactionCompleteL();
    void TransactionError(TInt aError);

private:
    void ConstructL(CRsfwDavAccess* aDavAccess,
                    MRsfwRemoteAccessResponseHandler* aResponseHandler,
                    const TDesC& aPathName,
                    TUint aLockFlags,
                    TUint& aTimeout,
                    TDesC8** aLockToken);

private: // Data
    TUint            iLockFlags;
    TUint*           iTimeout;
    TDesC8**         iLockToken;
    CRsfwDavFileInfo*    iDavFileInfo;
    };

// -----------------------------------------------------------------
// RefreshLock
// -----------------------------------------------------------------

class CRsfwDavAccessContextRefreshLock: public CRsfwDavAccessContext
    {
public:
    /**
     * Two-phased constructor.
     * 
     */
    static CRsfwDavAccessContextRefreshLock* NewL(
        CRsfwDavAccess* aDavAccess,
        MRsfwRemoteAccessResponseHandler* aResponseHandler,
        const TDesC& aPathName,
        const TDesC8* aLockToken,
        TUint& aTimeout);
    
    /**
     * Destructor.
     */
    virtual ~CRsfwDavAccessContextRefreshLock();

public: // Functions from base classes
    // From CRsfwDavAccessContext
    void StartL();
    void TransactionCompleteL();
    void TransactionError(TInt aError);

private:
    void ConstructL(CRsfwDavAccess* aDavAccess,
                    MRsfwRemoteAccessResponseHandler* aResponseHandler,
                    const TDesC& aPathName,
                    const TDesC8* aLockToken,
                    TUint& aTimeout);

private: // Data
    TBufC<KMaxPath>  iPathName;
    const TDesC8*    iLockToken;
    TUint*           iTimeout;
    CRsfwDavFileInfo*    iDavFileInfo;
    };

// -----------------------------------------------------------------
// Unlock
// -----------------------------------------------------------------

class CRsfwDavAccessContextUnlock: public CRsfwDavAccessContext
    {
public:
    /**
     * Two-phased constructor.
     * 
     */
    static CRsfwDavAccessContextUnlock* NewL(
        CRsfwDavAccess* aDavAccess,
        MRsfwRemoteAccessResponseHandler* aResponseHandler,
        const TDesC& aPathName,
        const TDesC8* aLockToken);

public: // Functions from base classes
    // From CRsfwDavAccessContext
    void StartL();
    void TransactionCompleteL();
    void TransactionError(TInt aError);

private:
    void ConstructL(CRsfwDavAccess* aDavAccess,
                    MRsfwRemoteAccessResponseHandler* aResponseHandler,
                    const TDesC& aPathName,
                    const TDesC8* aLockToken);
    
private: // Data
    const TDesC8*    iLockToken;
    };

#endif // CRSFWDAVACCESSCONTEXT_H

// End of File
