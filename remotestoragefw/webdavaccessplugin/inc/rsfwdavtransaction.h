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
* Description:  WebDAV transaction
 *
*/


#ifndef CRSFWDAVTRANSACTION_H
#define CRSFWDAVTRANSACTION_H

// INCLUDES
#include <e32base.h>
#include <uri8.h>
//#include <f32file.h>
//#include <s32file.h>
//#include <http/MHTTPAuthenticationCallback.h>
#include <http/mhttpdatasupplier.h>
//#include <chttpformencoder.h>
#include <http/mhttptransactioncallback.h>

//#include <xml/parser.h>

#include "rsfwdavdefs.h"
//#include "rsfwremoteaccess.h"
//#include "rsfwdavsession.h"

// FORWARD DECLARATIONS
class CRsfwDavSession;
class CRsfwDirEnt;
class CRsfwDavFileInfo;

// CONSTANTS
const TInt KHttpPortNumber  =  80;
const TInt KHttpsPortNumber = 443;
_LIT(KHttpScheme, "http");
_LIT(KHttpsScheme, "https");
_LIT8(KHttpsScheme8, "https");

/**
 *  WebDAV transaction
 *
 *  @lib davaccess.lib
 *  @since Series 60 3.1
 */
class CRsfwDavTransaction: public CBase,
                          public MHTTPTransactionCallback,
                          public MHTTPDataSupplier
                     
    {
public: // Constructors and destructor
    /**
     * Two-phased constructor.
     */
    static CRsfwDavTransaction* NewL(CRsfwDavSession* aWebDavSession,
                                    TWebDavOp aWebDavOp,
                                    const TUriC8& aUri, 
                                    RStringF aMethod,
                                    TUint aTransactionId);
    /**
     * Destructor.
     */
    virtual ~CRsfwDavTransaction();

public: // New functions
    void SetBodyData(HBufC8* aRequestBodyBuffer);
    void SetBodyFileL(const TDesC& aPath, 
                      TInt aOffset, 
                      TInt* aLength,
                      TUint aFlags);
    void SetPropFindPath(HBufC* aPropFindPath);
    void SetPropFindDirEntryArray(RPointerArray<CRsfwDirEnt>& aDirEnts);
    void SubmitL();
    void Cancel();
    void SetDavFileInfoL(CRsfwDavFileInfo** aDavFileInfo, const TDesC& aPath);
    inline RHTTPTransaction HttpTransaction() {return iHttpTransaction;};
    inline TUint Id() {return iTransactionId;};
    inline TInt Status() {return iStatus;};

public: // Functions from base classes
    // From MHTTPTransactionCallback
    void MHFRunL(RHTTPTransaction aTransaction, 
                 const THTTPEvent& aEvent);
    TInt MHFRunError(TInt aError,
                     RHTTPTransaction aTransaction,
                     const THTTPEvent& aEvent);
    
    // From MHTTPDataSupplier
    TBool GetNextDataPart(TPtrC8& aDataPart);
    void ReleaseData();
    TInt OverallDataSize();
    TInt Reset();

private:
    void ConstructL(CRsfwDavSession* aWebDavSession,
                    TWebDavOp aWebDavOp,
                    const TUriC8& aUri, 
                    RStringF aMethod,
                    TInt aTransactionId);
    void TransactionCompleteL();
    void TransactionError();
    void Cleanup();
    void PropFindResponseBeginL(TInt aDepth);
    void LockQueryResponseBegin();
    void ParsePropFindResponseL(const TDesC8& aFragment);
    void ParseLockResponseL(const TDesC8& aFragment);
    void PropFindResponseEndL();
    void LockResponseEndL();

public: // data
    TWebDavOp               iWebDavOp;
    
private: // Data
    TUint                   iTransactionId;
    CRsfwDavSession*        iWebDavSession;
    RHTTPTransaction        iHttpTransaction;
    TBool                   iMoreToCome;
    RFs                     iFs;
    
    // files used with PUT and GET 
    RFile                   iBodyFile;
    TFileName               iBodyFilePath;
    TInt                    iBodyFileOffset;
    TUint                   iBodyFileFlags;
    TParse                  iParsedBodyFilePath;
    
    // pointer to client's "length" variable
    // GET operation will set this based on "content-length" from the server
    TInt*                   iClientsLength;

    HBufC8*                 iRequestBodyBuffer;

    // PROPFIND parser needs to now where it is prop finding....
    HBufC*                  iPropFindPath;
    // PROPFIND etc: response body is copied to a memory buffer
    HBufC8*                 iResponseBuffer;
    // how much body data has been sent to the HTTP stack
    TInt                    iSendDataCount;
    // total size of body the data
    TInt                    iOverallDataSize;
    TInt                    iStatus;
    TBool iNoContentLength;
    TBool                   iDiscardBody; // there may be body that we discard
    
    // Used with LOCKs to store files lock token
    CRsfwDavFileInfo*           iDavFileInfo; 
    
    // Used with PROPFIND to store metadata of all entries in a directory
    RPointerArray<CRsfwDirEnt>* iDirEnts;
    };

#endif // CRSFWDAVTRANSACTION_H
           
// End of File
