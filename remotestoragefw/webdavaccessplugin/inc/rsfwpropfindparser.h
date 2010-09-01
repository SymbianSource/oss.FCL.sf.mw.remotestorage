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
* Description:  WebDAV PropFind method response body parser
 *
*/


#ifndef CRSFWPROPFINDPARSER_H
#define CRSFWPROPFINDPARSER_H

// INCLUDES
#include <xml/contenthandler.h>
#include <xml/attribute.h> // needed for RAttributeArray

#include "rsfwdavfileinfo.h"
#include "rsfwdirent.h"

// CONSTANTS
const TInt KMaxNameSpaces = 10;

// FORWARD DECLARATIONS
class CRsfwDirEnt;

// CLASS DECLARATION
/**
 *  WebDAV Lock operation response body parser.
 *  Parses directory listing from WebDAV-server, which we got 
 *  as XML-body response to PROPFIND-query.
 *
 *  FOR METADATA:
 *  This comes from upper lever and are used to store relevant metadata
 *  from directory listing.
 *  Currently we want to extract the following to the entry
 *       displayname
 *       <creationdate>2002-12-19T13:51:16Z</creationdate>
 *       <getlastmodified>Thu, 19 Dec 2002 13:51:16 GMT</getlastmodified>
 *       <getcontentlength>2324</getcontentlength>;
 *
 *  FOR CACHE CONCISTENCY:
 *  We need to remember the ETag of searched directory
 *
 *  @lib davaccess.lib
 *  @since Series 60 3.1
 */

class CRsfwPropFindParser : public CBase, public Xml::MContentHandler
    {
// EName = parsing is inside <displayname> </displayname> etc., 
// ELooking = parsing is outside all tags of interest
    enum TState
        {
        EName,
        EResponse,
        ENameInDisplayName,
        EDate,
        EModified,
        ELength,
        EResourceType,
        EContentType,
        EETag,
        ELooking
        };

public: // Constructors and destructor
    /**
     * Two-phased constructor.
     */
    static CRsfwPropFindParser* NewL();
    static CRsfwPropFindParser* NewLC();

    /**
     * Destructor.
     */
    virtual ~CRsfwPropFindParser();

public: // Functions from base classes
// Symbian parser
    // From XML::MContentHandler
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
    // setters for the parser
    /**
       Set directory entry to be filled
       @param aDirEntArray directory entry array
    */
    void SetDirEntArray(RPointerArray<CRsfwDirEnt>* aDirEntArray);

    /**
       Set target directory
       @param aPropFindPath Propfind resource path
       @param aDepth Propfind depth
    */
    void SetTargetDirectory(const TDesC& aPropFindPath, TInt aDepth);
    
    /**
     	Returns the error code if there was an error during parsing of
     	the response.
    */
    TInt GetLastError();
    

private:
    void ConstructL();
    void ClearDirEntryL();
    HBufC* DecodeL(const TDesC8& aData);

private: // Data
    // Internal variables
    RPointerArray<CRsfwDirEnt>* iDirEntArray;  // not owned by us 
    const TDesC*            iPropFindPath; // path
    HBufC8*                 iContentString;
    TInt                    iDepth;
    CRsfwDirEnt*                iDirEntry;     // metadata entry currently read
    TState                  iParseState;   // internal state
    TBool                   iCurrentIsParent;
    TInt                    iError;        // indicates a processing error
    };

#endif // CRSFWPROPFINDPARSER_H

// End of File
