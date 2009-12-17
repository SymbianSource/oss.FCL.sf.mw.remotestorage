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
* Description:  Access Protocol plug-in loader
 *
*/


// INCLUDES
#include "rsfwremoteaccess.h"
#include <ecom.h>


// ----------------------------------------------------------------------------
// CRsfwRemoteAccess* CRsfwRemoteAccess::NewL
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwRemoteAccess* CRsfwRemoteAccess::NewL(const TDesC8& aProtocol)
    {
    // Set up the interface find for the default resolver
    TBuf8<KMaxMatchStringSize> matchString;
    _LIT8(KRemoteMatchPrefix, "remoteaccess/");
    matchString.Copy(KRemoteMatchPrefix);

    // Both "http" and "https" are handled by davaccess module
    _LIT8(KHttps, "https"); 
    if (aProtocol.Compare(KHttps) == 0)
        {
        _LIT8(KHttp, "http");   
        matchString.Append(KHttp);
        }
    else
        {
        matchString.Append(aProtocol);
        }
    TEComResolverParams resolverParams;
    resolverParams.SetDataType(matchString);
    // Disable wildcard matching
    resolverParams.SetWildcardMatch(EFalse);

    return REINTERPRET_CAST(CRsfwRemoteAccess*,
        REComSession::CreateImplementationL(
            KCRemoteAccessUid,
            _FOFF(CRsfwRemoteAccess, iDtor_ID_Key),
            resolverParams));
    }

// ----------------------------------------------------------------------------
// CRsfwRemoteAccess::~CRsfwRemoteAccess
// ----------------------------------------------------------------------------
//
EXPORT_C CRsfwRemoteAccess::~CRsfwRemoteAccess()
    {
    // Inform the framework that this specific 
    // instance of the interface has been destroyed.
    REComSession::DestroyedImplementation(iDtor_ID_Key);
    }

// ----------------------------------------------------------------------------
// CRsfwRemoteAccess:: GetQuotaAndSizeL
// ----------------------------------------------------------------------------
//    
EXPORT_C TInt CRsfwRemoteAccess:: GetQuotaAndSizeL(TInt& aQuota, TInt& aSize)
    {
    aQuota = KMountReportedSize;
    aSize = KMountReportedFreeSize;
    return 0;
    }

// End of File
