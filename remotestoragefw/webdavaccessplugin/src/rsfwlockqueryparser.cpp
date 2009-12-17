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
* Description:  Parse WebDAV LOCK method response body
 *
*/


// INCLUDE FILES
#include "rsfwlockqueryparser.h"
#include "rsfwdavfileinfo.h"
#include "mdebug.h"

// CONSTANTS
_LIT(KTimeOutTag,"Second-");

// ============================ MEMBER FUNCTIONS ==============================

CRsfwLockQueryParser* CRsfwLockQueryParser::NewLC()
    {
    CRsfwLockQueryParser* self = new(ELeave) CRsfwLockQueryParser;
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

CRsfwLockQueryParser* CRsfwLockQueryParser::NewL()
    {
    CRsfwLockQueryParser* self = NewLC();
    CleanupStack::Pop(self);
    return self;
    }

void CRsfwLockQueryParser::ConstructL()
    {
    /*
      iParseState = ELooking;
    */
    }

CRsfwLockQueryParser::~CRsfwLockQueryParser()
    {
    delete iContentString;
    }

// ----------------------------------------------------------------------------
// CRsfwLockQueryParser::OnStartDocumentL
// This method is a callback to indicate the start of the document.
// @param aDocParam Specifies the various parameters of the document.
// @arg aDocParam.iCharacterSetName The character encoding of the document.
// ----------------------------------------------------------------------------
//
void CRsfwLockQueryParser::OnStartDocumentL(
    const Xml::RDocumentParameters& /* aDocParam */,
    TInt /* aErrCode */)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwLockQueryParser::OnStartDocumentL
// This method is a callback to indicate the end of the document.
// ----------------------------------------------------------------------------
//
void CRsfwLockQueryParser::OnEndDocumentL(TInt aErrCode)
    {
    if (aErrCode)
        {
        User::Leave(aErrCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwLockQueryParser::OnStartElementL
// This method is a callback to indicate an element has been parsed.
// @param aElement is a handle to the element's details.
// @param aAttributes contains the attributes for the element.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwLockQueryParser::OnStartElementL(
    const Xml::RTagInfo& aElement,
    const Xml::RAttributeArray& /* aAttributes */,
    TInt aErrorCode)
    {
    _LIT8(KLockType, "locktype");
    _LIT8(KLockScope, "lockscope");
    _LIT8(KDepth, "depth");
    _LIT8(KTimeout, "timeout");
    _LIT8(KLockToken, "locktoken");

    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }

    switch (iParseState)
        {
    case ELockType:
        // Currently, write is the only acquired type, might change later
        break;

    case ELockScope:
        // Currently, exclusive is the only acquired type, might change later
        break;

    case EDepth:
        break;

    case ETimeout:
        break;

    case ELockToken:
        {
        _LIT8(KHref, "href");
        if (((aElement.LocalName()).DesC()).Compare(KHref) == 0)
            {
            iParseState = EHrefToken;
            }
        }
        break;

    case EHrefToken:
        break;

    case ELooking: // we are trying to find the next interesting tag
        if (((aElement.LocalName()).DesC()).Compare(KLockType) == 0)
            {
            iParseState = ELockType;
            }
        else if (((aElement.LocalName()).DesC()).Compare(KLockScope) == 0)
            {
            iParseState = ELockScope;
            }
        else if (((aElement.LocalName()).DesC()).Compare(KDepth) == 0)
            {
            iParseState = EDepth;
            }
        else if (((aElement.LocalName()).DesC()).Compare(KTimeout) == 0)
            {
            iParseState = ETimeout;
            }
        else if (((aElement.LocalName()).DesC()).Compare(KLockToken) == 0)
            {
            iParseState = ELockToken;
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
// CRsfwLockQueryParser::OnEndElementL
// This method is a callback to indicate that end of element has been reached.
// @param aElement is a handle to the element's details.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwLockQueryParser::OnEndElementL(const Xml::RTagInfo& /* aElement */,
                                     TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
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
    case ELockType:
        break;

    case ELockScope:
        break;

    case EDepth:
        {
        /*
          TPtrC aux;
          aux.Set(aBuf);
          TLex lex(aux);
          lex.Val(iDepth);
        */
        }
        break;

    case ETimeout:
        {
        if (iDavFileInfo)
            {
            _LIT8(KInfinite, "Infinite");
            if (iContentString->Compare(KInfinite) == 0)
                {
                iDavFileInfo->SetTimeout(KMaxTUint);
                }
            else
                {
                // We expect 'Second-x" where x a positive integer
                TInt timeout = 0;
                if (iContentString->Length() > KTimeOutTag().Length())
                    {
                    TPtrC8 aux;
                    aux.Set(*iContentString);
                    TLex8 lex(aux);
                    lex.SkipAndMark(KTimeOutTag().Length());
                    lex.Val(timeout);
                    }
                iDavFileInfo->SetTimeout(timeout);
                }
            }
        }
        break;

    case ELockToken:
        break;

    case EHrefToken:
        if (iDavFileInfo)
            {
            iDavFileInfo->SetLockTokenL(*iContentString);
            }
        break;

    case ELooking:
        break;

    default:
        break;
        }

    delete iContentString;
    iContentString = NULL;
    iParseState = ELooking;
    }

// ----------------------------------------------------------------------------
// CRsfwLockQueryParser::OnContentL
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
void CRsfwLockQueryParser::OnContentL(const TDesC8& aBytes, TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }

    // We want to add to contentstring only if we are in a state
    // where the content is interesting to us
    if ((iParseState == ETimeout) || (iParseState == EHrefToken))
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
// CRsfwLockQueryParser::OnStartPrefixMappingL
// This method is a notification of the beginning of the scope of a prefix-URI
// Namespace mapping.
// This method is always called before corresponding OnStartElementL method.
// @param aPrefix is the Namespace prefix being declared.
// @param aUri is the Namespace URI the prefix is mapped to.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwLockQueryParser::OnStartPrefixMappingL(const RString& /* aPrefix */,
                                             const RString& /* aUri */,
                                             TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwLockQueryParser::OnEndPrefixMappingL
// This method is a notification of end of the scope of a prefix-URI mapping.
// This method is called after the corresponding DoEndElementL method.
// @param aPrefix is the Namespace prefix that was mapped.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwLockQueryParser::OnEndPrefixMappingL(const RString& /* aPrefix */,
                                           TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwLockQueryParser::OnIgnorableWhiteSpaceL
// This method is a notification of ignorable whitespace in element content.
// @param aBytes are the ignored bytes from the document being parsed.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwLockQueryParser::OnIgnorableWhiteSpaceL(const TDesC8& /* aBytes */,
                                              TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwLockQueryParser::OnSkippedEntityL
// This method is a notification of a skipped entity.
// If the parser encounters an external entity it does not need to expand it -
// it can return the entity as aName for the client to deal with.
// @param aName is the name of the skipped entity.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwLockQueryParser::OnSkippedEntityL(const RString& /* aName */,
                                        TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwLockQueryParser::OnProcessingInstructionL
// This method is a receive notification of a processing instruction.
// @param aTarget is the processing instruction target.
// @param aData is the processing instruction data. If empty none was supplied.
// @param aErrorCode is the error code.
// If this is not KErrNone then special action may be required.
// ----------------------------------------------------------------------------
//
void CRsfwLockQueryParser::OnProcessingInstructionL(const TDesC8& /* aTarget */,
                                                const TDesC8& /* aData */,
                                                TInt aErrorCode)
    {
    if (aErrorCode)
        {
        User::Leave(aErrorCode);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwLockQueryParser::OnError
// This method indicates an error has occurred.
// @param aErrorCode is the error code
// ----------------------------------------------------------------------------
//

void CRsfwLockQueryParser::OnError(TInt aErrorCode)
    {
    DEBUGSTRING(("CRsfwLockQueryParser::OnError(%d)", aErrorCode));
    iError = aErrorCode;
    }

// ----------------------------------------------------------------------------
// CRsfwLockQueryParser::GetExtendedInterface
// This method obtains the interface matching the specified uid.
// @return 0 if no interface matching the uid is found.
// Otherwise, the this pointer cast to that interface.
// @param aUid the uid identifying the required interface.
// ----------------------------------------------------------------------------
//
TAny* CRsfwLockQueryParser::GetExtendedInterface(const TInt32 /* aUid */)
    {
    return NULL;
    }

// ----------------------------------------------------------------------------
// CRsfwLockQueryParser::SetFileDavInfo
// Set a pointer to the directory information structure to be filled.
// ----------------------------------------------------------------------------
//
void CRsfwLockQueryParser::SetDavFileInfo(CRsfwDavFileInfo* aDavFileInfo)
    {
    iDavFileInfo = aDavFileInfo;
    }
    
TInt CRsfwLockQueryParser::GetLastError() 
	{
	return iError;
	}

//  End of File
