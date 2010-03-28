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
* Description:  WebDAV session
 *
*/


#ifndef CRSFWDAVSESSION_H
#define CRSFWDAVSESSION_H

// INCLUDES
#include <http/mhttpauthenticationcallback.h>
#include <xml/parser.h>
#include "rsfwdavdefs.h"


// FORWARD DECLARATIONS
class CRsfwDavTransaction;
class CRsfwDavFileInfo;
class CRsfwPropFindParser;
class CRsfwConnectionManager;
class MRsfwDavResponseObserver;
class MRsfwConnectionObserver;
class CRsfwLockQueryParser;
class CRsfwDirEnt;

// CLASS DECLARATION
/**
 *  Response handler for WebDAV requests
 *
 *  @lib davaccess.lib
 *  @since Series 60 3.1
 */
class MRsfwDavResponseObserver
    {
public:
    /**
     * Receive a notification of a completed request.
     * @param aWebDavTransactionId id of the request.
     */    
    virtual void RequestCompleteL(TUint aWebDavTransactionId) = 0;

    /**
     * Receive a notification of a failed request.
     * @param aWebDavTransactionId id of the request.
     */    
    virtual void RequestError(TUint aWebDavTransactionId, TInt aStatus) = 0;
    };


// CLASS DECLARATION
/**
 *  WebDAV session
 *
 *  Class created for the interface to  webdav client library
 *  Designed to encapsulate http,xml stuff
 *  Intended for communicating with one server at a time !
 * 
 *  Uses http transport framework: can do SSL, Basic + Digest Auth
 *  Uses symbian xmllib library for parsing
 *
 *  @lib davaccess.lib
 *  @since Series 60 3.1
 */
class CRsfwDavSession: public CBase,
                      public MHTTPAuthenticationCallback
    {
public: // Constructors and destructor
    /**
     * Two-phased constructor.
     */
    static CRsfwDavSession*
    NewL(MRsfwDavResponseObserver* aWebDavResponseObserver,
         MRsfwConnectionObserver* aRsfwConnectionObserver);
    /**
     * Destructor.
     */
    virtual ~CRsfwDavSession();

public: // New functions
    /**
       @function OpenL
       @discussion Opens the HTTP session and
         sets up parameters for the session
       @param aHost The full URI of the server
       including the path to the root directory
       @param aPort The port to connect to there
       @param aUserName  UserName to be used in Http basic or digest auth there
       @param aPassword  Password to be used in Http basic or digest auth there
       @param aAuxData Auxiliary information used for IAP selection
    */
    void OpenL(const TDesC& aHost,
               TInt aPort,
               const TDesC& aUserName,
               const TDesC& aPassword,
               const TDesC& aAuxData);
    
    /**
       @function OptionsL
       @discussion Runs an OPTIONS command to the passed uri to see if its
       available and force the authentication to run
       @return nothing
    */
    CRsfwDavTransaction* OptionsL();
    
    /**
       @function PropFindL
       @discussion Runs a PROPFIND command against the current server.
       Tells the server to only look for name, size related properties
       @param aPath The path relative to the root which should be used
       @param aIsDir Indicates whether the path points to a directory or a file
       @param aDirEnts Directory entry array to be filled
       @return pointer to the submitted WebDAV transaction
    */
    CRsfwDavTransaction* PropFindL(const TDesC& aPath,
                                  TInt aDepth,
                                  TBool aIsDir,
                                  RPointerArray<CRsfwDirEnt>& aDirEnts);

    /**
       @function GetL
       @discussion Runs GET command against the server
       @param aSrcPath The name of the resource to be fetched:
         expects this to be the path relative to the root directory
       @param aDstPath The path of the local file where the data is fetched
       @param aOffset offset from the start of the file
       @param aLength data length (can be NULL)
       @param aFlags operation options (see RemoteAccess.h)
       @return pointer to the submitted WebDAV transaction
    */
    // == GET
    CRsfwDavTransaction* GetL(const TDesC& aSrcPath,
                             const TDesC& aDstPath,
                             TInt aOffset,
                             TInt* aLength,
                             TUint aFlags);

    /**
       @function PutL
       @discussion Runs PUT command against the server
       @param aSrcPath The name of the resource to be copied:
       expects this to be an absolute path
       @param aDstPath The name of the resource to be created on the server:
       expected to be the path relative to the root directory
       @param aMimeType The MIME-type of the file 
       @param aOffset offset from the start of the file
       @param aLength data length (can be NULL)
       @param aTotalLength The total length, can be 0 if aLength is NULL or 0
       @param aUseContentRange Whether the server is assumed to support 
       Content-Range- header
       @param aLocktoken Possible lock token
       @return pointer to the submitted WebDAV transaction
    */
    // == PUT
    CRsfwDavTransaction* PutL(const TDesC& aSrcPath,
                             const TDesC& aDstPath,
                             const TDesC8& aMimeType,
                             TInt aOffset,
                             TInt aLength,
                             TInt aTotalLength,
                             TBool aUseContentRange,
                             const TDesC8* aLockToken = NULL);

    /**
       @function DeleteL
       @discussion Runs DELETE command against the server
       @param aResource The name of the resource to be deleted:
         expects this to be the path relative to the root directory
       @return pointer to the submitted WebDAV transaction
    */
    CRsfwDavTransaction* DeleteL(const TDesC& aPath, 
                                TBool aDir,
                                const TDesC8* aLockToken = NULL);

    /**
       @function MkDirL
       @discussion Runs MKCOL webdav command against the server
       @param aPath The name of the directory to be made:
       expects this to be the path relative to the root directory
       @return pointer to the submitted WebDAV transaction
    */
    CRsfwDavTransaction* MkDirL(const TDesC& aPath);   

    /**
       @function MoveL
       @discussion Runs MOVE command against the server
       @param aOldPath The name of the resource to be renamed:
       expects this to be the path relative to the root directory
       @param aNewPath The new name of the resource
       to be created on the server:
       expected to be the path relative to the root directory
       @param aOverwrite Specifies
       whether the server should overwrite a non-null destination resource 
       @return pointer to the submitted WebDAV transaction
    */
    CRsfwDavTransaction* MoveL(const TDesC& aOldPath,
                              const TDesC& aNewPath,
                              TBool aOverwrite,
                              const TDesC8* aSrcLockToken,
                              const TDesC8* aDstLockToken);

    /**
       @function LockL
       @discussion Runs LOCK command against the server
       @param aPath the resource to be locked:
       expects this to be the path relative to the root directory
       @param aFlags flags
       @param aTimeout lock timeout in seconds
       @param aDavFileInfo the location where collected file info should be set
       @return pointer to the submitted WebDAV transaction
    */
    CRsfwDavTransaction* LockL(const TDesC& aPath,
                              TUint aFlags,
                              TUint aTimeOut,
                              CRsfwDavFileInfo** aDavFileInfo);

    /**
       @function UnlockL
       @discussion Runs UNLOCK command against the server
       @param aPath the resource to be locked:
       expects this to be the path relative to the root directory
       @param aLockToken lock token
       @return pointer to the submitted WebDAV transaction
    */
    CRsfwDavTransaction* UnlockL(const TDesC& aPath,
                                const TDesC8* aLockToken);

    /**
       @function RefreshLockL
       @discussion Refreshes a locked resource by using LOCK method
       @param aPath the resource to be locked:
       expects this to be the path relative to the root directory
       @param aTimeout lock timeout in seconds
       @param aLockToken lock token
       @param aDavFileInfo the location where collected file info should be set
       @return pointer to the submitted WebDAV transaction
    */
    CRsfwDavTransaction* RefreshLockL(const TDesC& aPath,
                                     TUint aTimeOut,
                                     const TDesC8* aLockToken,
                                     CRsfwDavFileInfo** aDavFileInfo);

    /**
       @function Error
       @return Most recently recieved error code
    */        
    TInt Error();

    /**
       @function ErrorMsg
       @return Most recently received error message from server
    */            
    const TDesC& ErrorMsg();

    RHTTPSession& HttpSession();
    void SetConnected(TBool aConnected);
    void SetWebDavSupportClass(TInt aWebDavSupportClass);
    TInt WebDavSupportClass();
    
    inline RFs& FileServerSession() {return iFs;};
    inline const TDesC& RootDirectory() {return iDavRoot;};
    void WebDavTransactionCompleteL(CRsfwDavTransaction* aWebDavTransaction);
    void WebDavTransactionError(CRsfwDavTransaction* aWebDavTransaction);
    void SetPropFindParametersL(RPointerArray<CRsfwDirEnt>* aDirEntArray,
                               const TDesC& aPropFindPath,
                               TInt aDepth);  
    void SetLockQueryParameters(CRsfwDavFileInfo* aDavFileInfo);
    void ParsePropFindResponseL(const TDesC8& aResponse);
    void ParseLockResponseL(const TDesC8& aResponse);
    void PropFindResponseEndL();
    void LockResponseEndL();  
    void CancelParsing(TWebDavOp aOp);                               

public: // Functions from base classes
    // From MHTTPAuthenticationCallback
    TBool GetCredentialsL(const TUriC8& aURI,
                          RString aRealm, 
                          RStringF aAuthenticationType,
                          RString& aUserName, 
                          RString& aPassword);

private:
    void ConstructL(MRsfwDavResponseObserver* aWebDavResponseObserver,
                    MRsfwConnectionObserver* aRsfwConnectionObserver);
    RStringPool StringPool();
    void Slashify(TDes& aStr);
    HBufC* BuildPathLC(const TDesC& aRoot,
                       const TDesC& aPath,
                       TBool aEndSlash);
    HBufC* BuildFullPathLC(const TDesC& aPath, TBool aEndSlash);
    HBufC8* BuildUriLC(const TDesC& aPath,
                       TBool aEndSlash,
                       TUriParser8* aUriParser);
    void SetHeaderL(RHTTPHeaders aHeaders,
                    TInt aHdrField,
                    const TDesC8& aHdrValue);
    void SetHeaderL(RHTTPHeaders aHeaders,
                    const TDesC8& aHdrName,
                    const TDesC8& aHdrValue);
    void SetBasicHeadersL(RHTTPHeaders aHeaders, 
                          const TUriC8& aUri,
                          TBool aNoProxy);
    void SetDepthHeaderL(RHTTPHeaders aHeaders, TInt aDepth);
    void SetLockTokenHeaderL(RHTTPHeaders aHeaders, 
                             const TDesC8* aUri, 
                             const TDesC8* aLockToken,
                             TBool aUseTaggedLockToken);
    TBool IsConnected();
    TUint NextWebDavTransactionId();
    HBufC8* EncodeL(const TDesC& aData);
    void SetupConnectionL();
 
private: // Data
    // information about the connection
    HBufC8*  iUserName; 
    HBufC8*  iPassword;
    TBuf<KMaxServerNameLen>  iHost;
    TBuf<KMaxPath>           iDavRoot;
    TBuf<KMaxPath>           iHostRoot;
    TBuf<KMaxConnParameter>  iAuxData;
    HBufC8*                  iEncodedHost; // UTF8-encoded host name part

    MRsfwDavResponseObserver* iWebDavResponseObserver;
    MRsfwConnectionObserver* iRsfwConnectionObserver;
    
    CRsfwPropFindParser*         iPropFindParserImpl;
    CRsfwLockQueryParser*        iLockQueryParserImpl;
    Xml::CParser*            iPropFindParser;
    Xml::CParser*            iLockQueryParser;
    
    // whether XML parser should be cancelled if the transaction is cancelled
    TBool                   iPropfindParsingActive; 
                                   

    TBool        iConnected;    // whether we have a successful TCP session
    RFs          iFs;
    TInt         iWebDavSupportClass;
    TInt         iCredentialRequestCount;
    TUint        iCurrentWebDavTransactionId;
    RHTTPSession iHttpSession;
    CRsfwConnectionManager* iRsfwConnectionManager;
    };

#endif // CRSFWDAVSESSION_H

// End of File
