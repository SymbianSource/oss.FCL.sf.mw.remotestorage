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
* Description:  WebDAV Lock method response body parser
 *
*/


#ifndef CRSFWLOCKQUERYPARSER_H
#define CRSFWLOCKQUERYPARSER_H

// INCLUDES
#include <xml/contenthandler.h>
#include <xml/attribute.h> // needed for RAttributeArray

// FORWARD DECLARATIONS
class CRsfwDavFileInfo;

// CLASS DECLARATION

/**
 *  WebDAV Lock operation response body parser
 *
 *  @lib davaccess.lib
 *  @since Series 60 3.1
 */

class CRsfwLockQueryParser: public CBase, public Xml::MContentHandler
    {
    // DATA TYPES
    enum TLockType
        {
        EWriteLock  
        };
    
    enum TLockScope
        {
        ESharedLock,
        EExclLock
        };
    
    enum TState
        {
        ELooking,
        ELockToken,
        ELockScope,
        EDepth,
        ETimeout,
        ELockType,
        EHrefToken
        };

public: // Constructors and destructor
    /**
     * Two-phased constructor.
     */
    static CRsfwLockQueryParser* NewL();
    static CRsfwLockQueryParser* NewLC();

    /**
     * Destructor.
     */
    virtual ~CRsfwLockQueryParser();
    
public: // Functions from base classes
    // From Xml::MContentHandler
    void OnStartDocumentL(const Xml::RDocumentParameters& aDocParam,
                          TInt aErrorCode);
    void OnEndDocumentL(TInt aErrorCode);
    void OnStartElementL(const Xml::RTagInfo& aElement,
                         const Xml::RAttributeArray& aAttributes,
                         TInt aErrorCode);
    void OnEndElementL(const Xml::RTagInfo& aElement, TInt aErrorCode);
    void OnContentL(const TDesC8& aBytes, TInt aErrorCode);
    void OnStartPrefixMappingL(const RString& aPrefix,
                               const RString& aUri,
                               TInt aErrorCode);
    void OnEndPrefixMappingL(const RString& aPrefix, TInt aErrorCode);
    void OnIgnorableWhiteSpaceL(const TDesC8& aBytes, TInt aErrorCode);
    void OnSkippedEntityL(const RString& aName, TInt aErrorCode);
    void OnProcessingInstructionL(const TDesC8& aTarget,
                                  const TDesC8& aData,
                                  TInt aErrorCode);
    void OnError(TInt aErrorCode);
    TAny* GetExtendedInterface(const TInt32 aUid);  

public: // New functions
    /**
       Set file information container to be filled
       @param aFileInfo file info
    */
    void SetDavFileInfo(CRsfwDavFileInfo* aDavFileInfo); 
    
    TInt GetLastError();

private:
    void ConstructL();

private: // Data
    TState         iParseState;
    CRsfwDavFileInfo*  iDavFileInfo;
    HBufC8*        iContentString;
    TInt iError;
    };

#endif // CRSFWLOCKQUERYPARSER_H

// End of File
