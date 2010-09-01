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
* Description:  API for WebDAV operations
 *
*/


// INCLUDE FILES
#include <httpstringconstants.h>
#include <http/rhttpheaders.h>
#include <escapeutils.h>
#include <xml/matchdata.h>

#include "rsfwdavsession.h"
#include "rsfwdavtransaction.h"
#include "rsfwconnectionmanager.h"
#include "rsfwpropfindparser.h"
#include "rsfwlockqueryparser.h"
#include "mdebug.h"

// CONSTANTS
// characters that will be encoded in the url
_LIT8(KSpecials8, " \"<>#%{}|\\^~[]`");

// ============================ MEMBER FUNCTIONS ==============================
CRsfwDavSession* CRsfwDavSession::NewL(
    MRsfwDavResponseObserver* aWebDavResponseObserver,
    MRsfwConnectionObserver* aRsfwConnectionObserver)
    {
    CRsfwDavSession* self = new (ELeave) CRsfwDavSession;
    CleanupStack::PushL(self);
    self->ConstructL(aWebDavResponseObserver, aRsfwConnectionObserver);
    CleanupStack::Pop(self);
    return self;
    }

void CRsfwDavSession::ConstructL(
    MRsfwDavResponseObserver* aWebDavResponseObserver,
    MRsfwConnectionObserver* aRsfwConnectionObserver)
    {
    DEBUGSTRING(("DavSess: ConstructL: enter"));
    User::LeaveIfError(iFs.Connect());
    iWebDavResponseObserver = aWebDavResponseObserver;
    iRsfwConnectionObserver = aRsfwConnectionObserver;

    // Create classes needed for parsing PropFind and Lock request replies
    // Creating these later seems to cause emulator hang-ups.
    // If the problem does not exist in the real device we may want to
    // delay especially Lock parser creation as locking may not be used at all.
    Xml::CMatchData* matchData = Xml::CMatchData::NewLC();
	matchData->SetMimeTypeL(KTextXml);
	// Select libxml2 parsesr
	matchData->SetVariantL(_L8("libxml2"));
	// Select Symbian XML Parser (=Expat)
//	matchData->SetVariantL(_L8("Symbian"));
    iPropFindParserImpl = CRsfwPropFindParser::NewL();
    iPropFindParser = Xml::CParser::NewL(*matchData, *iPropFindParserImpl);
    iLockQueryParserImpl = CRsfwLockQueryParser::NewL();
    iLockQueryParser = Xml::CParser::NewL(*matchData, *iLockQueryParserImpl);
    CleanupStack::PopAndDestroy(matchData);

    // Open the RHTTPSession
    iHttpSession.OpenL();
    iHttpSession.FilterCollection().RemoveFilter( 
            iHttpSession.StringPool().StringF( HTTP::EAuthentication, RHTTPSession::GetTable() ));
    // Install this class as the callback for authentication requests:
    // it will take care of basic/digest auth, SSL
    InstallAuthenticationL(iHttpSession);
    DEBUGSTRING(("auth filter installed"));
    }

CRsfwDavSession::~CRsfwDavSession()
    {
    DEBUGSTRING(("CRsfwDavSession::~CRsfwDavSession"));
    delete iPropFindParser;
    delete iPropFindParserImpl;
    delete iLockQueryParser;
    delete iLockQueryParserImpl;
    delete iEncodedHost;
    if (iUserName) 
    {
    	delete iUserName;	
    }
    if (iPassword) 
    {
    	delete iPassword;
    }
  	iHttpSession.Close();
  	delete iRsfwConnectionManager;
    iFs.Close();
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::SetConnected
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::SetConnected(TBool aConnected)
    {
    iConnected = aConnected;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::SetWebDavSupportClass
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::SetWebDavSupportClass(TInt aWebDavSupportClass)
    {
    iWebDavSupportClass = aWebDavSupportClass;
    }
    
// ----------------------------------------------------------------------------
// CRsfwDavSession::WebDavSupportClass
// ----------------------------------------------------------------------------
//
TInt CRsfwDavSession::WebDavSupportClass()
    {
    return iWebDavSupportClass;
    }


// ----------------------------------------------------------------------------
// CRsfwDavSession::OpenL
// After calling this function, use options query
// to trigger TCP level connection.
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::OpenL(const TDesC& aUri,
                           TInt aPort,
                           const TDesC& aUserName,
                           const TDesC& aPassword,
                           const TDesC& aAuxData)
    {
    // Store connection parameters
    iHost.Zero();
    iDavRoot.Zero();

    // Here we add the port using a simple approach:
    // it needs to go after http://name/
    TInt slashCnt = 0;
    TInt cnt = 0;
    while (cnt < aUri.Length())
        {
        TChar ch = aUri[cnt++];
        if (ch == '/')
            {
            slashCnt++;
            if (slashCnt == 3)
                {
                iHost.Append(':');
                iHost.AppendNum(aPort);
                // At this point we know that
                // the remainder of the uri is the root directory
                }  
            }

        if (slashCnt > 2)
            {
            iDavRoot.Append(ch);
            }
        else
            {
            iHost.Append(ch);
            }
        }
    // We elso need an encoded form of the host part
    iEncodedHost = EncodeL(iHost.Right(iHost.Length() - KProtocolPrefix));

    // iDavRoot must be a directory, and thus should end with a slash
    Slashify(iDavRoot);

    // Make the pair
    iHostRoot.Copy(iHost);
    iHostRoot.Append(iDavRoot);

    // Assume that the parameters are constant and stable across the session
    iUserName = EncodeL(aUserName);
    iPassword = EncodeL(aPassword);
    iAuxData.Copy(aAuxData);

    DEBUGSTRING16(("connecting to host='%S', port=%d, root='%S', data=%S",
                   &iHost,
                   aPort,
                   &iDavRoot,
                   &iAuxData));

   if (iAuxData.Length() == 0) 
   {
   		// in case of empty access point info, set it to '?' (ask user)
   		_LIT(KAskUser, "?");
   		iAuxData.Copy(KAskUser);
   }
   
   if (!iRsfwConnectionManager)
   		{
        iRsfwConnectionManager =
             CRsfwConnectionManager::NewL(iRsfwConnectionObserver);
        iRsfwConnectionManager->UseIapL(aAuxData);
        }
        
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::OptionsL
// ----------------------------------------------------------------------------
//
CRsfwDavTransaction* CRsfwDavSession::OptionsL()
    {
    TPtrC null;
    TUriParser8 uriParser;
    HBufC8* uri = BuildUriLC(null, EFalse, &uriParser);

    DEBUGSTRING8(("OPTIONS '%S'", &uriParser.UriDes()));

    // Establish a link-layer connection
    if (iRsfwConnectionManager)
        {
        SetupConnectionL();
        }

    RStringPool stringPool = StringPool();

    // Introducing the webdav headers for the propfind
    RStringF mOptions = stringPool.OpenFStringL(KWebDavOptions);
    CleanupClosePushL(mOptions);
    CRsfwDavTransaction* webDavTransaction =
        CRsfwDavTransaction::NewL(this,
                                 EWebDavOpOptions,
                                 uriParser,
                                 mOptions,
                                 NextWebDavTransactionId());
    CleanupStack::PopAndDestroy(); // mOptions
    CleanupStack::PushL(webDavTransaction);

    RHTTPHeaders hdr = webDavTransaction->
        HttpTransaction().Request().GetHeaderCollection(); 
 
 	SetBasicHeadersL(hdr, uriParser, ETrue);
 	
    CleanupStack::Pop(2); //webDavTransaction, uri
    delete uri;
    return webDavTransaction;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::PropFindL
// Implements WebDAV PROPFIND method.
// Parameters: Name of the directory or file
//             CRsfwDirEnt pointer array to be filled with directory metadata
//             PROPFIND depth
//             aIsDir, is this directory or file,
//                     some extra checks are made based on this...
// ----------------------------------------------------------------------------
//
CRsfwDavTransaction* CRsfwDavSession::PropFindL(const TDesC &aPath,
                                              TInt aDepth,
                                              TBool aIsDir,
                                              RPointerArray<CRsfwDirEnt>& aDirEnts)
    {
    if (!IsConnected())
        {
        User::Leave(KErrNotReady);
        }

    TUriParser8 uriParser;
    HBufC8* uri = BuildUriLC(aPath, aIsDir, &uriParser);

    DEBUGSTRING8(("PROPFIND '%S'", &uriParser.UriDes()));
 
    // This function is used from several places
    TWebDavOp op;
    if (aDepth == 1)
        {
        op = EWebDavOpPropFindMulti;
        }
    else // (aDepth == 0)
        {
        op = EWebDavOpPropFindSingle;  
        }

    RStringPool stringPool = StringPool();
    RStringF mPropFind = stringPool.OpenFStringL(KWebDavPropFind);
    CleanupClosePushL(mPropFind);
    CRsfwDavTransaction* webDavTransaction =
        CRsfwDavTransaction::NewL(this,
                                 op,
                                 uriParser,
                                 mPropFind,
                                 NextWebDavTransactionId());
    CleanupStack::PopAndDestroy(); // mPropFind
    CleanupStack::PushL(webDavTransaction);

    RHTTPHeaders hdr = webDavTransaction->
        HttpTransaction().Request().GetHeaderCollection();

    // Add headers appropriate to all methods
    SetBasicHeadersL(hdr, uriParser, ETrue);

    SetHeaderL(hdr, HTTP::EContentType, KTextXml);

    // Assumes that the target uri has been cached in an earlier connect
    SetDepthHeaderL(hdr, aDepth);

    // XML body
    HBufC8* requestBodyBuffer = HBufC8::NewL(KDefaultSubmitSize);
    TPtr8 requestBodyBufferPtr = requestBodyBuffer->Des();

    // To make things at least little bit faster,
    // let's try to get "minimal" set
    // Maybe useful one day:
    // - <D:getcontentlanguage/>
    // - <D:creationdate/>
    // Apache mod_dav 1.0.3 doesn't support:
    //  - <D:displayname/>
    _LIT8(KPropFindRequestBody, "\
<?xml version=\"1.0\"?>\
<propfind xmlns=\"DAV:\">\
<prop>\
<getcontenttype/>\
<getlastmodified/>\
<getcontentlength/>\
<resourcetype/>\
<getetag/>\
</prop>\
</propfind>\
");
    requestBodyBufferPtr.Append(KPropFindRequestBody);
    webDavTransaction->SetBodyData(requestBodyBuffer);

    webDavTransaction->SetPropFindDirEntryArray(aDirEnts);
    // We must remember the work directory,
    // as we don't want to list that when building directory listing.
    HBufC* propFindPath = HBufC::NewL(iHostRoot.Length() +
                                       KMaxPath +
                                       1);
    TPtr propFindPathPtr = propFindPath->Des();
    propFindPathPtr.Copy(iDavRoot);
    propFindPathPtr.Append(aPath);
    // The whole path must end with a slash
    Slashify(propFindPathPtr);
    // Before comparing the path from the server is decoded,
    // so we can compare against the original 16-bit string.
    webDavTransaction->SetPropFindPath(propFindPath);
    CleanupStack::Pop(2); // webdavtransaction, uri
    delete uri;
    return webDavTransaction;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::GetL
// ----------------------------------------------------------------------------
//
CRsfwDavTransaction* CRsfwDavSession::GetL(const TDesC& aSrcPath,
                                         const TDesC& aDstPath,
                                         TInt aOffset,
                                         TInt* aLength,
                                         TUint aFlags)
    {
    // Basically just a HTTP get with some local processing
    if (!IsConnected())
        {
        User::Leave(KErrNotReady);
        }

    TUriParser8 uriParser;
    HBufC8* uri = BuildUriLC(aSrcPath, EFalse, &uriParser);

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
    DEBUGSTRING8(("GET '%S' (off=%d, len=%d)",
                  &uriParser.UriDes(),
                  aOffset,
                  length));
    }
#endif // DEBUG

    // Introducing the webdav headers for GET
    RStringPool stringPool = StringPool();
    RStringF mGet = stringPool.StringF(HTTP::EGET,
                                       RHTTPSession::GetTable());  
    CRsfwDavTransaction* webDavTransaction =
        CRsfwDavTransaction::NewL(this,
                                 EWebDavOpGet,
                                 uriParser,
                                 mGet,
                                 NextWebDavTransactionId());
    CleanupStack::PushL(webDavTransaction);

    // Not sure if this is needed: we are setting conditions
    // which mod_dav never gets, cos this is a GET..
    RHTTPHeaders hdr =
        webDavTransaction->HttpTransaction().Request().GetHeaderCollection();

    // Add headers appropriate to all methods
    SetBasicHeadersL(hdr, uriParser, ETrue);

    if (aLength && (*aLength > 0)) // partial get
        {
        TBuf8<KMaxFieldValueLength> rangeHeader;
        _LIT8(KBytesEquals, "bytes=");
        rangeHeader.Append(KBytesEquals);
        rangeHeader.AppendNum(aOffset);
        rangeHeader.Append('-');
        rangeHeader.AppendNum(aOffset + *aLength - 1);
        SetHeaderL(hdr, HTTP::ERange, rangeHeader);
        }

    webDavTransaction->SetBodyFileL(aDstPath, aOffset, aLength, aFlags);
    CleanupStack::Pop(webDavTransaction);
    CleanupStack::PopAndDestroy(uri);
    return webDavTransaction;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::PutL
// This amounts to a PUT sending the src data to the destination.
// Expects that the aSrcPath param is relative to iDavRoot.
// If aSrcPath is empty, an empty file will be created.
// ----------------------------------------------------------------------------
//
CRsfwDavTransaction* CRsfwDavSession::PutL(const TDesC& aSrcPath,
                                         const TDesC& aDstPath,
                                         const TDesC8& aMimeType,
                                         TInt aOffset,
                                         TInt aLength,
                                         TInt aTotalLength,
                                         TBool aUseContentRange,
                                         const TDesC8* aLockToken)
    {  
    if (!IsConnected())
        {
        User::Leave(KErrNotReady);
        }

    TUriParser8 uriParser;
    HBufC8* uri = BuildUriLC(aDstPath, EFalse, &uriParser);

    DEBUGSTRING8(("PUT '%S'", &uriParser.UriDes()));

    RStringPool stringPool = StringPool();
    RStringF mPut = stringPool.OpenFStringL(KWebDavPut);
    CleanupClosePushL(mPut);
    CRsfwDavTransaction* webDavTransaction =
        CRsfwDavTransaction::NewL(this,
                                 EWebDavOpPut,
                                 uriParser,
                                 mPut,
                                 NextWebDavTransactionId());
    CleanupStack::PopAndDestroy(); // mPut
    CleanupStack::PushL(webDavTransaction);

    RHTTPHeaders hdr =
        webDavTransaction->HttpTransaction().Request().GetHeaderCollection(); 

    // Add headers appropriate to all methods
    SetBasicHeadersL(hdr, uriParser, ETrue);  
    SetHeaderL(hdr, HTTP::EContentType, aMimeType);

    if (aLength > 0) // partial put
        {
        if (aUseContentRange)
            {
            TBuf8<KMaxFieldValueLength> rangeHeader;
            _LIT8(KBytes, "bytes ");
            rangeHeader.Append(KBytes);
            rangeHeader.AppendNum(aOffset);
            rangeHeader.Append('-');
            rangeHeader.AppendNum(aOffset + aLength - 1);
            rangeHeader.Append('/');
            if (aTotalLength == 0) 
                {
                // The asterisk "*" character means that 
                // the instance-length is unknown at the time when 
                // the message was generated. 
                rangeHeader.Append('*');
                }
            else
                {
                rangeHeader.AppendNum(aTotalLength);
                }
            SetHeaderL(hdr, HTTP::EContentRange, rangeHeader);
            }
        else
            {
            // server doesn't support Content-Range
            // Leave with KrrNotSupported
            // *aLength = aTotalLength;
            }  
        }

    if (aLockToken)
        {  
        SetLockTokenHeaderL(hdr, uri, aLockToken, ETrue);
        }

    webDavTransaction->SetBodyFileL(aSrcPath, aOffset, &aLength, 0);
    CleanupStack::Pop(webDavTransaction);
    CleanupStack::PopAndDestroy(uri);
    return webDavTransaction;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::DeleteL
// ----------------------------------------------------------------------------
//
CRsfwDavTransaction* CRsfwDavSession::DeleteL(const TDesC& aPath,
                                            TBool aIsDir,
                                            const TDesC8* aLockToken)
    {
    // Needs to take locking into account
    if (!IsConnected())
        {
        User::Leave(KErrNotReady);
        }

    TUriParser8 uriParser;
    HBufC8* uri = BuildUriLC(aPath, aIsDir, &uriParser);

    DEBUGSTRING8(("DELETE '%S'", &uriParser.UriDes()));

    RStringPool stringPool = StringPool();
    RStringF mDelete = stringPool.OpenFStringL(KWebDavDelete);
    CleanupClosePushL(mDelete);
    CRsfwDavTransaction* webDavTransaction =
        CRsfwDavTransaction::NewL(this,
                                 EWebDavOpDelete,
                                 uriParser,
                                 mDelete,
                                 NextWebDavTransactionId());
    CleanupStack::PopAndDestroy(); // mDelete
    CleanupStack::PushL(webDavTransaction);

    // need to add a special dir on the i
    RHTTPHeaders hdr =
        webDavTransaction->HttpTransaction().Request().GetHeaderCollection(); 
    // Add headers appropriate to all methods
    SetBasicHeadersL(hdr, uriParser, ETrue);

    if (aLockToken)
        {
        SetLockTokenHeaderL(hdr, uri, aLockToken, ETrue); 
        }

    CleanupStack::Pop(webDavTransaction);
    CleanupStack::PopAndDestroy(uri);
    return webDavTransaction;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::MkDirL
// ----------------------------------------------------------------------------
//
CRsfwDavTransaction* CRsfwDavSession::MkDirL(const TDesC& aPath)
    {
    // Executes a MKCOL with the specified name
    if (!IsConnected())
        {
        User::Leave(KErrNotReady);
        }

    TUriParser8 uriParser;
    HBufC8* uri = BuildUriLC(aPath, ETrue, &uriParser);

    DEBUGSTRING8(("MKCOL '%S'", &uriParser.UriDes()));

    RStringPool stringPool = StringPool();
    RStringF mMkCol = stringPool.OpenFStringL(KWebDavMkCol);
    CleanupClosePushL(mMkCol);
    CRsfwDavTransaction* webDavTransaction =
        CRsfwDavTransaction::NewL(this,
                                 EWebDavOpMkCol,
                                 uriParser,
                                 mMkCol,
                                 NextWebDavTransactionId());
    CleanupStack::PopAndDestroy(1); // mMkCol
    CleanupStack::PushL(webDavTransaction);

    // Neeed to add a special dir on the i
    RHTTPHeaders hdr =
        webDavTransaction->HttpTransaction().Request().GetHeaderCollection(); 

    // Add headers appropriate to all methods
    SetBasicHeadersL(hdr, uriParser, ETrue);

    CleanupStack::Pop(2); // webDavTransaction, uri
    delete uri;
    return webDavTransaction;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::MoveL
// ----------------------------------------------------------------------------
//
CRsfwDavTransaction* CRsfwDavSession::MoveL(const TDesC& aOldPath,
                                          const TDesC& aNewPath,
                                          TBool aOverwrite,
                                          const TDesC8* aSrcLockToken,
                                          const TDesC8* aDstLockToken)
    {
    if (!IsConnected())
        {
        User::Leave(KErrNotReady);
        }

    TUriParser8 uriParserNew;
    HBufC8* uriNew = BuildUriLC(aNewPath, EFalse, &uriParserNew);
    TUriParser8 uriParserOld;
    HBufC8* uriOld = BuildUriLC(aOldPath, EFalse, &uriParserOld);

    DEBUGSTRING8(("MOVE '%S' to '%S'",
                  &uriParserOld.UriDes(),
                  &uriParserNew.UriDes()));

    RStringPool stringPool = StringPool();
    RStringF mMove = stringPool.OpenFStringL(KWebDavMove);
    CleanupClosePushL(mMove);
    CRsfwDavTransaction* webDavTransaction =
        CRsfwDavTransaction::NewL(this,
                                 EWebDavOpMove,
                                 uriParserOld,
                                 mMove,
                                 NextWebDavTransactionId());
    CleanupStack::PopAndDestroy(); // mMove
    CleanupStack::PushL(webDavTransaction);

    RHTTPHeaders hdr =
        webDavTransaction->HttpTransaction().Request().GetHeaderCollection();

    // Add headers appropriate to all methods
    SetBasicHeadersL(hdr, uriParserOld, ETrue);
    
    if (aSrcLockToken)
        {
        SetLockTokenHeaderL(hdr, uriOld, aSrcLockToken, ETrue);
        }
    
    
     if (aDstLockToken)
        {
        SetLockTokenHeaderL(hdr, uriNew, aDstLockToken, ETrue);
        }      
        
    SetHeaderL(hdr, KWebDavDest, *uriNew);
    if (aOverwrite)
        {
        SetHeaderL(hdr, KWebDavOverwrite, KWebDavOverwriteY);
        }
    else
        {
        SetHeaderL(hdr, KWebDavOverwrite, KWebDavOverwriteN);
        }

    CleanupStack::Pop(webDavTransaction);
    CleanupStack::PopAndDestroy(2, uriNew);   // uriOld, uriNew
    return webDavTransaction;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::LockL
// ----------------------------------------------------------------------------
//
CRsfwDavTransaction* CRsfwDavSession::LockL(const TDesC& aPath,
                                          TUint aFlags,
                                          TUint aTimeout,
                                          CRsfwDavFileInfo** aDavFileInfo)
    {
    // Opens LOCK transaction
    if (!IsConnected())
        {
        User::Leave(KErrNotReady);
        }

    TUriParser8 uriParser;
    HBufC8* uri = BuildUriLC(aPath, EFalse, &uriParser);

    DEBUGSTRING8(("LOCK '%S' (%d seconds)", &uriParser.UriDes(), aTimeout));

    RStringPool stringPool = StringPool();
    RStringF mLock = stringPool.OpenFStringL(KWebDavLock);
    CleanupClosePushL(mLock);
    CRsfwDavTransaction* webDavTransaction =
        CRsfwDavTransaction::NewL(this,
                                 EWebDavOpLock,
                                 uriParser,
                                 mLock,
                                 NextWebDavTransactionId());
    CleanupStack::PopAndDestroy(&mLock);
    CleanupStack::PushL(webDavTransaction);

    // headers
    RHTTPHeaders hdr =
        webDavTransaction->HttpTransaction().Request().GetHeaderCollection(); 

    // Add headers appropriate to all methods
    SetBasicHeadersL(hdr, uriParser, ETrue);

    SetHeaderL(hdr, HTTP::EContentType, KTextXml);

    HBufC8* timeoutBuffer = HBufC8::NewLC(KMaxFieldValueLength);
    TPtr8 timeoutBufferPtr = timeoutBuffer->Des();
    timeoutBufferPtr.Append(KSecondDash);
    if (aTimeout != 0)
        {
        timeoutBufferPtr.AppendNum(aTimeout);
        }
    SetHeaderL(hdr, KWebDavTimeout, timeoutBufferPtr);
    CleanupStack::PopAndDestroy(timeoutBuffer);

    // XML body
    HBufC8* requestBodyBuffer = HBufC8::NewL(KDefaultSubmitSize);
    TPtr8 requestBodyBufferPtr = requestBodyBuffer->Des();

    // Note: locktype "write" is currently the only legal value
    _LIT8(KLockHeaderFormat, "\
<?xml version=\"1.0\" encoding=\"utf-8\" ?>\
<D:lockinfo xmlns:D=\"DAV:\">\
<D:lockscope><D:%S/></D:lockscope>\
<D:locktype><D:write/></D:locktype>\
<D:owner xmlns:x=\"http://www.webdav.org/\">\
<x:lock-user>%S</x:lock-user>\
<x:created-by>%S</x:created-by>\
</D:owner>\
</D:lockinfo>");

    _LIT8(KLockScopeShared, "shared");
    _LIT8(KLockScopeExclusive, "exclusive");
    TPtrC8 lockScope;
    if (aFlags & EFileShareAny)
        {
        lockScope.Set(KLockScopeShared);
        }
    else
        {
        lockScope.Set(KLockScopeExclusive);
        }
     
    requestBodyBufferPtr.Format(KLockHeaderFormat, &lockScope, iUserName, iUserName);
    webDavTransaction->SetBodyData(requestBodyBuffer);

    HBufC* fileInfoPath = BuildFullPathLC(aPath, EFalse);
    webDavTransaction->SetDavFileInfoL(aDavFileInfo, *fileInfoPath);
    CleanupStack::PopAndDestroy(fileInfoPath);
    CleanupStack::Pop(webDavTransaction);
    CleanupStack::PopAndDestroy(uri);
    return webDavTransaction;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::UnlockL
// ----------------------------------------------------------------------------
//
CRsfwDavTransaction* CRsfwDavSession::UnlockL(const TDesC& aPath,
                                            const TDesC8* aLockToken)
    {
    // Opens LOCK transaction
    if (!IsConnected())
        {
        User::Leave(KErrNotReady);
        }

    if (iWebDavSupportClass < KDavVersionTwo)
        {
        User::Leave(KErrNotSupported);
        }

    TUriParser8 uriParser;
    HBufC8* uri = BuildUriLC(aPath, EFalse, &uriParser);

    DEBUGSTRING8(("UNLOCK '%S'", &uriParser.UriDes()));

    RStringPool stringPool = StringPool();
    RStringF mUnlock = stringPool.OpenFStringL(KWebDavUnlock);
    CleanupClosePushL(mUnlock);
    CRsfwDavTransaction* webDavTransaction =
        CRsfwDavTransaction::NewL(this,
                                 EWebDavOpUnlock,
                                 uriParser,
                                 mUnlock,
                                 NextWebDavTransactionId());
    CleanupStack::PopAndDestroy(); // mUnlock
    CleanupStack::PushL(webDavTransaction);

    RHTTPHeaders hdr =
        webDavTransaction->HttpTransaction().Request().GetHeaderCollection(); 

    // Add headers appropriate to all methods
    SetBasicHeadersL(hdr, uriParser, ETrue);

    HBufC8* lockToken = HBufC8::NewLC(aLockToken->Length() +
                                      KLockTokenOverhead);
    TPtr8 lockTokenPtr = lockToken->Des();
    lockTokenPtr.Append('<');
    lockTokenPtr.Append(*aLockToken);
    lockTokenPtr.Append('>');
    SetHeaderL(hdr, KWedDavLockToken, lockTokenPtr);
    CleanupStack::PopAndDestroy(lockToken);

    CleanupStack::Pop(2); // webdavtransaction , uri
    delete uri;
    return webDavTransaction;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::RefreshLockL
// ----------------------------------------------------------------------------
//
CRsfwDavTransaction* CRsfwDavSession::RefreshLockL(const TDesC& aPath,
                                                 TUint aTimeout,
                                                 const TDesC8* aLockToken,
                                                 CRsfwDavFileInfo** aDavFileInfo)
    {
    // Opens LOCK transaction
    if (!IsConnected())
        {
        User::Leave(KErrNotReady);
        }

    if (iWebDavSupportClass < KDavVersionTwo)
        {
        User::Leave(KErrNotSupported);
        }

    TUriParser8 uriParser;
    HBufC8* uri = BuildUriLC(aPath, EFalse, &uriParser);

    DEBUGSTRING8(("LOCK (refresh) '%S' (%d seconds)",
                  &uriParser.UriDes(),
                  aTimeout));

    RStringPool stringPool = StringPool();
    RStringF mLock = stringPool.OpenFStringL(KWebDavLock);
    CleanupClosePushL(mLock);
    CRsfwDavTransaction* webDavTransaction =
        CRsfwDavTransaction::NewL(this,
                                 EWebDavOpRefreshLock,
                                 uriParser,
                                 mLock,
                                 NextWebDavTransactionId());
    CleanupStack::PopAndDestroy(&mLock);
    CleanupStack::PushL(webDavTransaction);

    RHTTPHeaders hdr =
        webDavTransaction->HttpTransaction().Request().GetHeaderCollection(); 

    // Add headers appropriate to all methods
    SetBasicHeadersL(hdr, uriParser, ETrue);

    // do not use tagged lock token, as refresh 'If' header 
    // should always contain only a single lock token 
    // (only one lock may be refreshed at a time).
    SetLockTokenHeaderL(hdr, uri, aLockToken, EFalse);

    HBufC8* timeoutBuffer = HBufC8::NewLC(KMaxFieldValueLength);
    TPtr8 timeoutBufferPtr = timeoutBuffer->Des();
    timeoutBufferPtr.Append(KSecondDash);
    if (aTimeout != 0)
        {
        timeoutBufferPtr.AppendNum(aTimeout);
        }
    SetHeaderL(hdr, KWebDavTimeout, timeoutBufferPtr);
    CleanupStack::PopAndDestroy(timeoutBuffer);

    HBufC* fileInfoPath = BuildFullPathLC(aPath, EFalse);
    webDavTransaction->SetDavFileInfoL(aDavFileInfo, *fileInfoPath);
    CleanupStack::PopAndDestroy(fileInfoPath);
    CleanupStack::Pop(webDavTransaction);
    CleanupStack::PopAndDestroy(uri);
    return webDavTransaction;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::GetCredentialsL
// From MHTTPAuthenticationCallback
// ----------------------------------------------------------------------------
//
TBool CRsfwDavSession::GetCredentialsL(const TUriC8& /* aURI */,
                                      RString aRealm,
                                      RStringF /*aAuthenticationType*/,
                                      RString& aUserName,
                                      RString& aPassword)
    {
    // if we have not tried to send the current credentials once,
    // and we have at least username proceed, othwise return KErrAccessDenied
    if (iCredentialRequestCount || (!iUserName))
        {
        iCredentialRequestCount = 0;
        User::Leave(KErrAccessDenied);
        }
    iCredentialRequestCount++;

    TRAPD(err, aUserName = aRealm.Pool().OpenStringL(*iUserName));
    if (!err)
        {
        TRAP(err, aPassword = aRealm.Pool().OpenStringL(*iPassword));
        if (!err)
            {
            return ETrue;
            }
        }  
    return EFalse;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::SetHeaderL
// Convenience method for setting up the header.
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::SetHeaderL(RHTTPHeaders aHeaders,
                                TInt aHdrField,
                                const TDesC8& aHdrValue)
    {
    RStringF valStr = StringPool().OpenFStringL(aHdrValue);
    CleanupClosePushL(valStr);
    THTTPHdrVal val(valStr);
    aHeaders.SetFieldL(
        StringPool().StringF(aHdrField, RHTTPSession::GetTable()), val);
    CleanupStack::PopAndDestroy(&valStr);
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::SetHeaderL
// Convenience method for setting up the header.
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::SetHeaderL(RHTTPHeaders aHeaders,
                                const TDesC8& aHdrName,
                                const TDesC8& aHdrValue)
    {
    RStringF nameStr = StringPool().OpenFStringL(aHdrName);
    CleanupClosePushL(nameStr);
    RStringF valueStr = StringPool().OpenFStringL(aHdrValue);
    CleanupClosePushL(valueStr);
    THTTPHdrVal value(valueStr);
    aHeaders.SetFieldL(nameStr, value);
    CleanupStack::PopAndDestroy(2); // valueStr, nameStr
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::SetBasicHeadersL
// Convenience method for setting up the header.
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::SetBasicHeadersL(RHTTPHeaders aHeaders, 
                                       const TUriC8& aUri,
                                       TBool aNoProxy)
    {
    // Add headers appropriate to all methods
    SetHeaderL(aHeaders, HTTP::EUserAgent, KUserAgent);
    SetHeaderL(aHeaders, HTTP::EAccept, KAccept);
    // do not send host header if using SSL (not supported currently)
    TPtrC8 scheme;
    if (aUri.IsPresent(EUriScheme))
        {
        scheme.Set(aUri.Extract(EUriScheme));
        }
    if (scheme.CompareF(KHttpsScheme8) != 0) 
    	{
    	SetHeaderL(aHeaders, HTTP::EHost, *iEncodedHost);
    	}
    SetHeaderL(aHeaders, HTTP::EConnection, KKeepAlive);
    if (aNoProxy) 
        {
        // see RFC 2518 10.4.5 If Header and Non-DAV Aware Proxies
        // "As in general clients may not be able to reliably detect 
        // non-DAV aware intermediates, they are advised to always 
        // prevent caching using the request directives mentioned above."
        SetHeaderL(aHeaders, HTTP::EPragma, KWebDavNoProxy);
        SetHeaderL(aHeaders, HTTP::ECacheControl, KWebDavNoProxy);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::SetDepthHeaderL
// Some DAV requests require this for specifying depth of copies etc.
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::SetDepthHeaderL(RHTTPHeaders aHeaders, TInt aDepth)
    {
    RStringF depthStr = StringPool().OpenFStringL(KWebDavDepth);
    CleanupClosePushL(depthStr);
    THTTPHdrVal depth(aDepth);
    aHeaders.SetFieldL(depthStr, depth);
    CleanupStack::PopAndDestroy(&depthStr);
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::SetLockTokenHeaderL
// The lock token header can be tagged with the resource (file) URI
// This is especially important for DELETE and MOVE of a file,
// as they will also affect the container (directory), which is not locked
// (in which case supplying a lock token is an error).
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::SetLockTokenHeaderL(RHTTPHeaders aHeaders,
                                         const TDesC8* aUri,
                                         const TDesC8* aLockToken,
                                         TBool aUseTaggedLockToken)
    {
    DEBUGSTRING(("using a tagged lock token"));
    // KLockTokenOverhead for 'tagged' lock token is 2 chars around the uri,
    // one space, and 4 chars around the locktoken
    // i.e. <target-url> (<target-token>)
    TInt tagoverhead;
    if (aUseTaggedLockToken) 
        {
        tagoverhead = KTaggedLockTokenOverhead;
        }
    else 
        {
        tagoverhead = KLockTokenOverhead;
        }
    HBufC8* lockToken = HBufC8::NewLC(aUri->Length()+ aLockToken->Length() +
                                      tagoverhead);
    TPtr8 lockTokenPtr = lockToken->Des();
    if (aUseTaggedLockToken) 
        {
        lockTokenPtr.Format(KTaggedParenthAngleFormat, aUri, aLockToken);
        }
    else 
        {
        lockTokenPtr.Format(KParenthAngleFormat, aLockToken);
        }

    DEBUGSTRING8(("lt='%S'", &lockTokenPtr));
    SetHeaderL(aHeaders, KWebDavIf, lockTokenPtr);
    CleanupStack::PopAndDestroy(lockToken);
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::WebDavTransactionCompleteL
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::WebDavTransactionCompleteL(
    CRsfwDavTransaction* aWebDavTransaction)
    {
    TUint webDavTransactionId = aWebDavTransaction->Id();
    delete aWebDavTransaction;
    iWebDavResponseObserver->RequestCompleteL(webDavTransactionId);
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::WebDavTransactionError
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::WebDavTransactionError(
    CRsfwDavTransaction* aWebDavTransaction)
    {
    TUint webDavTransactionId = aWebDavTransaction->Id();
    TInt status = aWebDavTransaction->Status();
    delete aWebDavTransaction;
    iWebDavResponseObserver->RequestError(webDavTransactionId, status);
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::SetPropFindParameters
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::SetPropFindParametersL(
    RPointerArray<CRsfwDirEnt>* aDirEntArray,
    const TDesC& aPropFindPath,
    TInt aDepth)
    {
    iPropFindParserImpl->SetDirEntArray(aDirEntArray);
    iPropFindParserImpl->SetTargetDirectory(aPropFindPath, aDepth);
    iPropFindParser->ParseBeginL();
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::SetLockQueryParameters
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::SetLockQueryParameters(CRsfwDavFileInfo* aDavFileInfo)
    {
    iLockQueryParserImpl->SetDavFileInfo(aDavFileInfo);
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::ParsePropFindResponseL
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::ParsePropFindResponseL(const TDesC8& aResponse)
    {
    // only first call to this function really initiates data structures in the XML parser
    iPropfindParsingActive = ETrue;
    iPropFindParser->ParseL(aResponse);
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::ParseLockResponseL
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::ParseLockResponseL(const TDesC8& aResponse)
    {
    iLockQueryParser->ParseL(aResponse);
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::PropFindResponseEndL
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::PropFindResponseEndL()
    {
    iPropFindParser->ParseEndL();
    iPropfindParsingActive = EFalse;
    TInt err = iPropFindParserImpl->GetLastError();
    if (err) 
   	 	{
    	User::Leave(err);
    	}
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::LockResponseEndL
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::LockResponseEndL()
    {
    iLockQueryParser->ParseEndL();
    TInt err = iPropFindParserImpl->GetLastError();
    if (err) 
   	 	{
    	User::Leave(err);
    	}
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::CancelParsing
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::CancelParsing(TWebDavOp aOp)
    {
    // Only will do something if this operation is PROPFIND,
    // and we have started to parse the response.
    // UI does not allow to cancel LOCK requests.
    // If this would be possible this mechanism should be expanded
    // to also cover LOCK parsing.
    if ((aOp == EWebDavOpPropFindSingle) || 
    (aOp == EWebDavOpPropFindMulti)) 
        {
        if (iPropfindParsingActive)   
            {
            // When XML parsing is cancelled when the request is cancelled, 
            // there is some XML error (invalid token etc.), ignore the error
            TRAP_IGNORE(iPropFindParser->ParseEndL());
            iPropfindParsingActive = EFalse;
            }
        }
    }
    

// ----------------------------------------------------------------------------
// CRsfwDavSession::HttpSession
// ----------------------------------------------------------------------------
//
RHTTPSession& CRsfwDavSession::HttpSession()
    {
    return iHttpSession;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::StringPool
// ----------------------------------------------------------------------------
//
RStringPool CRsfwDavSession::StringPool()
    {
    return iHttpSession.StringPool();
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::Slashify
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::Slashify(TDes& aStr)
    {
    if (aStr.Length() &&
        (aStr[aStr.Length() - 1] != '/') &&
        aStr.Length() < aStr.MaxLength())
        {
        aStr.Append('/');
        }
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::BuildPathLC
// This function constructs a file path from davroot + path
// ----------------------------------------------------------------------------
//
HBufC* CRsfwDavSession::BuildPathLC(const TDesC& aRoot,
                                   const TDesC& aPath,
                                   TBool aEndSlash)
    {
    // 1 is for a possible slash added to the end of the uri...
    HBufC* path = HBufC::NewLC(aRoot.Length() +
                               aPath.Length() +
                               1);
    TPtr pathPtr = path->Des();
    pathPtr.Append(aRoot);
    pathPtr.Append(aPath);
    if (aEndSlash)
        {
        Slashify(pathPtr);
        }
    return path;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::BuildFullPathLC
// This function constructs a file path from davroot + path
// ----------------------------------------------------------------------------
//
HBufC* CRsfwDavSession::BuildFullPathLC(const TDesC& aPath,
                                       TBool aEndSlash)
    {
    return BuildPathLC(iDavRoot, aPath, aEndSlash);
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::BuildUriLC
// This function constructs an URI from hostname + davroot + path
// The URI is first escape encoded and then UTF-8 encoded.
// Note that in addition to returning the uri string, this function
// will also "populate" aUriParser with the full URI
// ----------------------------------------------------------------------------
//
HBufC8* CRsfwDavSession::BuildUriLC(const TDesC& aPath,
                                   TBool aEndSlash,
                                   TUriParser8* aUriParser)
    {
    // 1 is for a possible slash added to the end of the uri...
    HBufC* uri = BuildPathLC(iHostRoot, aPath, aEndSlash);
    HBufC8* utf8Path = EncodeL(*uri);
    CleanupStack::PopAndDestroy(uri);
    CleanupStack::PushL(utf8Path);
    TPtr8 utf8PathPtr = utf8Path->Des();
    if (aUriParser)
        {
        if (aUriParser->Parse(utf8PathPtr) != KErrNone)
            {
            User::Leave(KErrBadName);
            }
        }
    return utf8Path;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::EncodeL
// First escape encode and then UTF-8 encode data.
// ----------------------------------------------------------------------------
//
HBufC8* CRsfwDavSession::EncodeL(const TDesC& aData)
    {
    HBufC8* utf8Data = EscapeUtils::ConvertFromUnicodeToUtf8L(aData);
    CleanupStack::PushL(utf8Data);
    HBufC8* escapedData = EscapeUtils::EscapeEncodeL(*utf8Data, KSpecials8);
    CleanupStack::PopAndDestroy(utf8Data);
    return escapedData;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::IsConnected
// ----------------------------------------------------------------------------
//
TBool CRsfwDavSession::IsConnected()
    {
    return iConnected;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::NextWebDavTransactionId
// ----------------------------------------------------------------------------
//
TUint CRsfwDavSession::NextWebDavTransactionId()
    {
    return ++iCurrentWebDavTransactionId;
    }

// ----------------------------------------------------------------------------
// CRsfwDavSession::SetupConnectionL
// ----------------------------------------------------------------------------
//
void CRsfwDavSession::SetupConnectionL()
    {
    RSocketServ* socketServ;
    RConnection* connection;

    DEBUGSTRING(("SetupConnection - start"));
    User::LeaveIfError(iRsfwConnectionManager->GetConnection(socketServ, 
                                                             connection));
    DEBUGSTRING(("iRsfwConnectionManager->GetConnection called"));                                                        
    // Now bind the HTTP session with the socket server connection
    RStringPool stringPool = iHttpSession.StringPool();
    RHTTPConnectionInfo connInfo = iHttpSession.ConnectionInfo();
    connInfo.SetPropertyL(
        stringPool.StringF(HTTP::EHttpSocketServ, RHTTPSession::GetTable()),
        THTTPHdrVal(socketServ->Handle()));
    connInfo.SetPropertyL(
        stringPool.StringF(HTTP::EHttpSocketConnection,
                           RHTTPSession::GetTable()), 
        THTTPHdrVal(reinterpret_cast<TInt>(connection)));
    DEBUGSTRING(("SetupConnection - done"));
    }
    
//  End of File
