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
* Description:  Parse WebDAV PROPFIND method response body
 *
*/


// INCLUDE FILES
#include <f32file.h>
#include <escapeutils.h>
#include <tinternetdate.h>

#include "rsfwpropfindparser.h"
//#include "rsfwdirent.h"
#include "rsfwdirentattr.h"
#include "mdebug.h"
#include "uri8.h"

// ============================ MEMBER FUNCTIONS ==============================
CRsfwPropFindParser* CRsfwPropFindParser::NewLC()
    {
    CRsfwPropFindParser* self = new (ELeave) CRsfwPropFindParser;
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

CRsfwPropFindParser* CRsfwPropFindParser::NewL()
    {
    CRsfwPropFindParser* self = NewLC();
    CleanupStack::Pop(self);
    return self;
    }

void CRsfwPropFindParser::ConstructL()
    {
    ClearDirEntryL();
    iCurrentIsParent = EFalse;
    }

CRsfwPropFindParser::~CRsfwPropFindParser()
    {
    delete iDirEntry;
    delete iContentString;
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::OnStartDocumentL
// This method is a callback to indicate the start of the document.
// @param aDocParam Specifies the various parameters of the document.
// @arg aDocParam.iCharacterSetName The character encoding of the document.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::OnStartDocumentL(
    const Xml::RDocumentParameters& /* aDocParam */,
    TInt aErrCode)
    {
    iError = KErrNone;    // discard the old error
    if (!aErrCode)
        {
        iParseState = ELooking;
        }
    else
        {
        User::Leave(aErrCode);
        }
    
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::OnStartDocumentL
// This method is a callback to indicate the end of the document.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::OnEndDocumentL(TInt aErrCode)
    {
    if (aErrCode)
        {
        User::Leave(aErrCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::OnStartElementL
// This method is a callback to indicate an element has been parsed.
// @param aElement is a handle to the element's details.
// @param aAttributes contains the attributes for the element.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::OnStartElementL(
    const Xml::RTagInfo& aElement,
    const Xml::RAttributeArray& /* aAttributes */,
    TInt aErrorCode)
    {
    _LIT8(KResponseHeader, "response");
    _LIT8(KContentType,    "getcontenttype");
    _LIT8(KDate,           "creationdate");
    _LIT8(KModified,       "getlastmodified");
    _LIT8(KLength,         "getcontentlength");
    _LIT8(KResourceType,   "resourcetype");
    _LIT8(KEtag,           "getetag");

    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }

    switch (iParseState)
        {
    case EName:
        break;

    case EResponse:
        {
        _LIT8(KHRef, "href");
        if (((aElement.LocalName()).DesC()).Compare(KHRef) == 0)
            {
            // href that follows response tag is the name of the file
            iParseState = EName;
            }
        }
        break;

    case EModified:
        break;

    case ELength:
        break;

    case EDate:
        break;

    case EResourceType:
        {
        _LIT8(KCollection, "collection");
        if (((aElement.LocalName()).DesC()).Compare(KCollection) == 0)
            {
            iDirEntry->Attr()->SetAtt(KEntryAttDir);
            }
        }
        break;

    case EContentType:
        break;

    case EETag:
        break;

    case ELooking: // we are trying to find the next interesting tag
        if (((aElement.LocalName()).DesC()).Compare(KModified) == 0)
            {
            iParseState = EModified;
            }
        else if (((aElement.LocalName()).DesC()).Compare(KLength) == 0)
            {
            iParseState = ELength;
            }
        else if (((aElement.LocalName()).DesC()).Compare(KDate) == 0)
            {
            iParseState = EDate;
            }
        else if (((aElement.LocalName()).DesC()).Compare(KResourceType) == 0)
            {
            iParseState = EResourceType;
            }
        else if (((aElement.LocalName()).DesC()).Compare(KContentType) == 0)
            {
            iParseState = EContentType;
            }
        else if (((aElement.LocalName()).DesC()).Compare(KResponseHeader) == 0)
            {
            iParseState = EResponse;
            }
        else if (((aElement.LocalName()).DesC()).Compare(KEtag) == 0)
            {
            iParseState = EETag;
            }
        else
            {
            // lint
            }
        break;

    default:
        break;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::OnEndElementL
// This method is a callback to indicate that end of element has been reached.
// @param aElement is a handle to the element's details.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::OnEndElementL(const Xml::RTagInfo& aElement,
                                    TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }

    // If we reached </response> we can fill next entry in iDirEntArray
    _LIT8(KResponse, "*response");
    if (((aElement.LocalName()).DesC()).Match(KResponse) != KErrNotFound)
        {
        // Save the entry if depth = 0 or depth = 1 and
        // this is not the parent directory
        switch (iDepth)
            {
        case 0:
            iDirEntArray->Append(iDirEntry);
            // ownership transferred
            iDirEntry = NULL;
            break;

        case 1:
            if (!iCurrentIsParent)
                {
                iDirEntArray->Append(iDirEntry);
                // ownership transferred
                iDirEntry = NULL;
                }
            break;

        default:
            break;
            }

        if (iCurrentIsParent)
            {
            // We have 'seen' the end tag of parent directory
            iCurrentIsParent = EFalse;
            }

        // In any case going through an entry is complete,
        // reset iDirEntry
        ClearDirEntryL();
        delete iContentString;
        iContentString = NULL;
        iParseState = ELooking;
        }

    // otherwise we will continue reading
    // if we have some interesting content
    if ((iParseState != ELooking) && !iContentString)
        {
        iParseState = ELooking;
        return;
        }

    switch (iParseState)
        {
    case EName:
        {
        // Figure out whether the entry we are currently reading
        // is the directory itself.
        // The directory itself is the first one in the reply that
        // comes from server,
        // and the last one that our XML-parser passes to us
        
        // if the name is fully qualified URI, only take the path
        TPtrC8 uriPtr = iContentString->Des();
        TPtrC8 pathPtr = uriPtr;
        TUriParser8 uriParser;
        if (uriParser.Parse(uriPtr) == KErrNone) 
        {
        	pathPtr.Set(uriParser.Extract(EUriPath));
        }
        
        
        HBufC* name = DecodeL(pathPtr);
        CleanupStack::PushL(name);
        
        if (name->Compare(*iPropFindPath) == 0)
            {
            iCurrentIsParent = ETrue;
            }
        else
            {
            TPtrC namePtr = name->Des();
            if ((namePtr.Length() > 1) &&
                (namePtr[namePtr.Length() - 1] == '/'))
                {
                // strip off trailing '/'
                namePtr.Set(namePtr.Left(namePtr.Length() - 1));
                }

            TInt pos = namePtr.LocateReverse('/');
            // Shouldn't be negative as
            // the path should always start with '/'
            if ((pos >= 0) && (namePtr.Length() > 1))
                {
                namePtr.Set((namePtr.Right(namePtr.Length() - (pos + 1))));
                }
            iDirEntry->SetNameL(namePtr);
            }
        CleanupStack::PopAndDestroy(name);
        }
        break;

    case EModified:
        {
        // Webdav sends dates in RFC 822 format
        // (e.g., "Thu, 19 Dec 2002 13:51:16 GMT").
        // We parse them as 8 bit data.
        TInternetDate inetDate;
        inetDate.SetDateL(*iContentString);
        iDirEntry->Attr()->SetModified(inetDate.DateTime());
        }
        break;

    case ELength:
        {
        // Convert to int
        TLex8 lex(*iContentString);
        TInt len;
        User::LeaveIfError(lex.Val(len));
        iDirEntry->Attr()->SetSize(len);
        }
        break;

    case EETag:
        // etag is stored for files
        if (!(iDirEntry->Attr()->Att() & KEntryAttDir)) 
            {
            iDirEntry->Attr()->SetETagL(*iContentString);
            }
        
        break;

    case EContentType:
        {
        iDirEntry->Attr()->SetMimeTypeL(*iContentString);
        }
        break;

    default:
        break;
        }

    delete iContentString;
    iContentString = NULL;
    iParseState = ELooking;
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::OnContentL
// This method is a callback that sends the content of the element.
// Not all the content may be returned in one go.
// The data may be sent in chunks.
// When an OnEndElementL is received there is no more content to be sent.
// @param aBytes is the raw content data for the element.
// The client is responsible for converting the data to the
// required character set if necessary.
// In some instances the content may be binary and must not be converted.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
//
void CRsfwPropFindParser::OnContentL(const TDesC8& aBytes, TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }

    // We want to add to contentstring only if we are in a state
    // where the content is interesting to us
    if ((iParseState == EName) || (iParseState == EModified) ||
        (iParseState == ELength) || (iParseState ==EETag) ||
        (iParseState == EContentType))
        {
        if (!iContentString)
            {
            iContentString = HBufC8::NewL(aBytes.Length());
            TPtr8 string = iContentString->Des();
            string.Append(aBytes);
            }
        else
            {
            iContentString =
                iContentString->ReAllocL(iContentString->Length() +
                                         aBytes.Length());
            TPtr8 string = iContentString->Des();
            string.Append(aBytes);
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::OnStartPrefixMappingL
// This method is a notification of the beginning of the scope of a prefix-URI
// Namespace mapping.
// This method is always called before corresponding OnStartElementL method.
// @param aPrefix is the Namespace prefix being declared.
// @param aUri is the Namespace URI the prefix is mapped to.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::OnStartPrefixMappingL(const RString& /* aPrefix */,
                                            const RString& /* aUri */,
                                            TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::OnEndPrefixMappingL
// This method is a notification of end of the scope of a prefix-URI mapping.
// This method is called after the corresponding DoEndElementL method.
// @param aPrefix is the Namespace prefix that was mapped.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::OnEndPrefixMappingL(const RString& /* aPrefix */,
                                          TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::OnIgnorableWhiteSpaceL
// This method is a notification of ignorable whitespace in element content.
// @param aBytes are the ignored bytes from the document being parsed.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::OnIgnorableWhiteSpaceL(const TDesC8& /* aBytes */,
                                             TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::OnSkippedEntityL
// This method is a notification of a skipped entity.
// If the parser encounters an external entity it does not need to expand it -
// it can return the entity as aName for the client to deal with.
// @param aName is the name of the skipped entity.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::OnSkippedEntityL(const RString& /* aName */,
                                       TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::OnProcessingInstructionL
// This method is a receive notification of a processing instruction.
// @param aTarget is the processing instruction target.
// @param aData is the processing instruction data. If empty none was supplied.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::OnProcessingInstructionL(const TDesC8& /* aTarget */,
                                               const TDesC8& /* aData */,
                                               TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::OnError
// This method indicates an error has occurred.
// @param aErrorCode is the error code
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::OnError(TInt aErrorCode)
    {
    DEBUGSTRING(("CRsfwPropFindParser::OnError(%d)", aErrorCode));
    iError = aErrorCode;
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::GetExtendedInterface
// This method obtains the interface matching the specified uid.
// @return 0 if no interface matching the uid is found.
// Otherwise, the this pointer cast to that interface.
// @param aUid the uid identifying the required interface.
// ----------------------------------------------------------------------------
//
TAny* CRsfwPropFindParser::GetExtendedInterface(const TInt32 /* aUid */)
    {
    return NULL;
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::SetDirEntArray
// Set a pointer to the directory entry array to be filled.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::SetDirEntArray(RPointerArray<CRsfwDirEnt>* aDirEntArray)
    {
    iDirEntArray = aDirEntArray;
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::SetTargetDirectory
// Set the directory for which PROPFIND was targeted.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::SetTargetDirectory(const TDesC& aPropFindPath,
                                         TInt aDepth)
    {
    iPropFindPath = &aPropFindPath;
    iDepth = aDepth;
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::ClearDirEntryL()
// Clear directory entry for later use.
// ----------------------------------------------------------------------------
//
void CRsfwPropFindParser::ClearDirEntryL()
    {
    delete iDirEntry;
    iDirEntry = NULL;
    TPtrC noName;
    iDirEntry = CRsfwDirEnt::NewL(noName, NULL);
    // Will be changed to directory, if we ran into ´<collection> tag
    iDirEntry->Attr()->SetAtt(KEntryAttNormal);
    }

// ----------------------------------------------------------------------------
// CRsfwPropFindParser::DecodeL()
// First UTF-8 decode and then escape decode data.
// ----------------------------------------------------------------------------
//
HBufC* CRsfwPropFindParser::DecodeL(const TDesC8& aData)
    {
    HBufC8* utf8Data = EscapeUtils::EscapeDecodeL(aData);
    CleanupStack::PushL(utf8Data);
    HBufC* data = NULL;
    // if converting to unicode fails, just return the escapedecoded string.
    TRAPD(err, data = EscapeUtils::ConvertToUnicodeFromUtf8L(*utf8Data));
    if (err) 
        {
        data = HBufC::NewMaxL(utf8Data->Length());
        TPtr dataPtr = data->Des();
        dataPtr.Copy(*utf8Data);
        }
    CleanupStack::PopAndDestroy(utf8Data);
    return data;    

    }
    
TInt CRsfwPropFindParser::GetLastError() 
	{
	return iError;
	}
	

//  End of File
