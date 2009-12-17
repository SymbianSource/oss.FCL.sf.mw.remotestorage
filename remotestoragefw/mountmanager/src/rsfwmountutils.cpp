/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Utility functions for managing mount configurations
*
*/

// INCLUDES

#include <s32file.h>
#include <uri8.h>
#include <utf.h>
#include <ecom/ecom.h>
#include <ecom/implementationinformation.h>
#include <rsfwmountentry.h>

#include "rsfwmountutils.h"
#include "rsfwremoteaccess.h"


// CONSTANTS
// These keywords must match with the entry items
_LIT(KKId, "Fname");
_LIT(KKDrive, "Fdrv");
_LIT(KKUri, "Furi");
_LIT(KKUserName, "Fuid");
_LIT(KKPassword, "Fpsw");

_LIT(KMountMessagePrefix, "//vnd.nokia.s60.mount.config\n");

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// RsfwMountUtils::ImportMountEntryL
// ----------------------------------------------------------------------------
//
EXPORT_C void RsfwMountUtils::ImportMountEntryL(const TDesC& aMsg,
                                            CRsfwMountEntry** aEntry)
    {
    // Expected to start with
    // //vnd.nokia.s60.mount.config\n
    
    TPtrC prefix = aMsg.Left(KMountMessagePrefixLength);
    if (prefix.Compare(KMountMessagePrefix) != 0)
        {
        User::Leave(KErrNotFound);
        }
    
    // It is a mount configuration message
    CRsfwMountEntry* entry = CRsfwMountEntry::NewLC();

    TInt lineNumber = 0;

    // SMS rich text messages may be have trailing characters -
    // that looks like ')' characters
    TInt i = aMsg.Length() - 1;
    while (i > 0)
        {
        TUint8 c = static_cast<TUint8>(aMsg[i]);
        if (c != ')')
            {
            break;
            }
        i--;
        }
    TPtrC msg  = aMsg.Left(i + 1);

    TLex lex(msg);
    while (!lex.Eos())
        {
        lineNumber++;
        lex.SkipSpaceAndMark();
        while (lex.Peek() != '\n'  && !lex.Eos())
            {
            lex.Inc();
            }
        // The marked token is now the whole line
        TPtrC line = lex.MarkedToken();
        ParseLineL(line, *entry);
        if (!lex.Eos())
            {
            // Skip the '\n'
            lex.Inc();
            }
        }

    CleanupStack::Pop(entry);
    *aEntry = entry;
    }

// ----------------------------------------------------------------------------
// RsfwMountUtils::ImportMountEntryFromStreamL
// ----------------------------------------------------------------------------
//
EXPORT_C void RsfwMountUtils::ImportMountEntryFromStreamL(RReadStream& aReadStream,
													      TInt aSize,
                                                          CRsfwMountEntry** aEntry)
    {
    // the string is assumed to be 8-bit in stream, 
    // and should be converted to Unicode
    *aEntry = NULL;
    HBufC8* tempbuf = HBufC8::NewLC(aSize);
    TPtr8 tbufPtr = tempbuf->Des();
    tbufPtr.Zero();
    TRAPD(err, aReadStream.ReadL(tbufPtr));
    
    HBufC* buf = HBufC::NewLC(aSize);
    TPtr bufPtr = buf->Des();
    CnvUtfConverter::ConvertToUnicodeFromUtf8(bufPtr, tbufPtr);
    if ((err == KErrNone) || (err == KErrEof))
        {
        ImportMountEntryL(bufPtr, aEntry);
        }
    CleanupStack::PopAndDestroy(2, tempbuf); // buf, tempbuf 
    }

// ----------------------------------------------------------------------------
// RsfwMountUtils::ExportMountEntryL
// ----------------------------------------------------------------------------
//
EXPORT_C void RsfwMountUtils::ExportMountEntryL(const CRsfwMountEntry& aEntry,
                                                TBool aSendCredentials,
                                                TDes& aBuf)
    {
    aBuf.Copy(KMountMessagePrefix);
    TInt i;
    // smart messaging sends:
    // EMountEntryItemName
    // EMountEntryItemUri
    // if aSendCredentials sends also:
    // EMountEntryItemUserName
    // EMountEntryItemPassword 
    TInt lastToExport;
    if (aSendCredentials) 
        {
        lastToExport = EMountEntryItemPassword; 
        }
    else 
        {
        lastToExport = EMountEntryItemUri;
        }
    
    for (i = EMountEntryItemName; i < lastToExport + 1; i++)
        {
        if (i != EMountEntryItemDrive) 
        	{
        	const HBufC* item = aEntry.Item(i);
        	if (item && item->Length())
            	{
            	aBuf.Append(GetKeyWord(i));
            	aBuf.Append(':');
                const TPtrC ip(*item);
                aBuf.Append(ip);
            	aBuf.Append('\n');
            	}        	
        	}
       }
    }
 
// ----------------------------------------------------------------------------
// RsfwMountUtils::IsFriendlyNameValid
// ----------------------------------------------------------------------------
//
EXPORT_C TBool RsfwMountUtils::IsFriendlyNameValid(const TDesC& aFriendlyName)
    {
    TInt err;
    TBool retValue = ETrue;
    RFs fsSession;
    err = fsSession.Connect();
    if (err)
        {
        return retValue;
        }    
    retValue = fsSession.IsValidName(aFriendlyName);
    fsSession.Close();  
        
    // Some names are acceptable by RFs.IsValidName(), 
    // but not acceptable by RFs.SetDriveName()
    // Those are checked below:
        
    // solely period characters ( e.g. "...")
    if (retValue)
        {
        retValue = EFalse;
        TChar period(46); // period (".") character
        TInt i;
        for (i = 0; i < aFriendlyName.Length(); i++)
            {   
            if (aFriendlyName[i] != period)
                {
                retValue = ETrue;
                break;
                }
            }
        }
        
    // period as the last character ( e.g. "myDrive.")
    if (retValue)
        {
        TChar period(46); // period (".") character
        TInt nameLength = aFriendlyName.Length();
        if (nameLength > 0 && aFriendlyName[nameLength-1] == period)
            {
            retValue = EFalse;
            }
        }    

    return retValue;    
    }

// ----------------------------------------------------------------------------
// RsfwMountUtils::IsDriveAddressValid
// ----------------------------------------------------------------------------
//
EXPORT_C TBool RsfwMountUtils::IsDriveAddressValid(const TDesC8& aDriveAddress)
    {
    // extract the protocol
    TInt err;
    TUriParser8 parser;
    err = parser.Parse(aDriveAddress);
    if (err) 
    		{
    		return EFalse;	
    		}
    TPtrC8 protocol = parser.Extract(EUriScheme);
    
    
    TBuf8<KMaxMatchStringSize> matchString;
    _LIT8(KRemoteMatchPrefix, "remoteaccess/");
    matchString.Copy(KRemoteMatchPrefix);

    // Both "http" and "https" are handled by davaccess module
    _LIT8(KHttps, "https"); 
    if (protocol.Compare(KHttps) == 0)
        {
        _LIT8(KHttp, "http");   
        matchString.Append(KHttp);
        }
    else
        {
        matchString.Append(protocol);
        }
    TEComResolverParams resolverParams;
    resolverParams.SetDataType(matchString);
    // Disable wildcard matching
    resolverParams.SetWildcardMatch(EFalse);    
    
    
    // check whether there is a remote access plugin implementation for that protocol 
    RImplInfoPtrArray implinfo;
    TRAP(err, REComSession::ListImplementationsL(
                             KCRemoteAccessUid,
                             resolverParams,
                             implinfo));
    
    if (err != KErrNone) 
        {
        // if fetching the list of implemenations fail,'
        // (for example a temporary out of memory situation)
        // just accept the string
        return ETrue;
        }
     
    // we assume that protocol used for remote access
    //  represents hierarchical relationships within the namespace.  This
    // "generic URI" syntax consists of a sequence of four main components:
    //      <scheme>://<authority><path>?<query>
    // check that scheme is followed by "://"
    _LIT8(KDelimiter, "://");
    if (aDriveAddress.Length() < protocol.Length()+3) 
        {
        return EFalse;
        }
    
    TPtrC8 test = aDriveAddress.Mid(protocol.Length(), 3);
    if (!(test.Compare(KDelimiter) == 0)) 
        {
        return EFalse;    
        }
    
    TInt count = implinfo.Count();
    implinfo.ResetAndDestroy();
    if (count > 0) 
        {
        return ETrue;
        }
    else 
        {
        return EFalse;
        }
    }

// ----------------------------------------------------------------------------
// RsfwMountUtils::ParseLineL
// ----------------------------------------------------------------------------
//
void RsfwMountUtils::ParseLineL(const TDesC& aLine,
                                CRsfwMountEntry& aEntry)
    {
    TLex lex(aLine);
    // Extract Line i.e "leftToken:RightToken<LF>", then asign to TLex object
    while (lex.Peek() != ':' && lex.Peek() != '\n' && !lex.Eos())
        {
        lex.Inc();
        }

    // We are only interested in lines containing a colon delimeter ':'
    // other text lines are ignored i.e "Welcome !<LF>"
    if(lex.Peek() != ':')
        {
        // It was not a name value pair
        return;
        }
    if (lex.TokenLength() == 0)
        {
        User::Leave(KErrNotFound);
        }
    // Get the keyword
    HBufC* keyWord = lex.MarkedToken().AllocLC();
    // Go past the ':'
    lex.Inc();
    lex.SkipSpaceAndMark();
    while (lex.Peek() != '\n' && lex.Peek() != '\r' && !lex.Eos())
        {
        lex.Inc();
        }

    TInt i;
    // Keyword matching
    // password is the last possible entry that is transferred
    for (i = EMountEntryItemName; i < EMountEntryItemPassword + 1; i++)
        {
        if (GetKeyWord(i).Compare(*keyWord) == 0)
            {
            aEntry.SetItemL(i, lex.MarkedToken());
            }
        }
    CleanupStack::PopAndDestroy(keyWord);
    }

// ----------------------------------------------------------------------------
// RsfwMountUtils::GetKeyWord
// ----------------------------------------------------------------------------
//
const TDesC& RsfwMountUtils::GetKeyWord(TInt iItem)
    {
    switch (iItem)
        {
    case EMountEntryItemName:
        return KKId;
        
    case EMountEntryItemDrive:
        return KKDrive;

    case EMountEntryItemUri:
        return KKUri;

    case EMountEntryItemUserName:
        return KKUserName;

    case EMountEntryItemPassword:
        return KKPassword;

    default:
        return KKId;
        }
    }
// End of File
