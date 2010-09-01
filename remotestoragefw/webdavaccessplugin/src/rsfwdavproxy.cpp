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
* Description:  ECOM proxy for DavAccess
 *
*/


// INCLUDE FILES
//#include <e32std.h>
#include <implementationproxy.h>
#include "rsfwdavaccess.h"

// CONSTANTS
// Map the interface UIDs to implementation factory functions
const TImplementationProxy ImplementationTable[] = 
    {
        {{0x101F9769}, (TProxyNewLPtr)CRsfwDavAccess::NewL}
    };

// ========================== OTHER EXPORTED FUNCTIONS ========================

// ----------------------------------------------------------------------------
// ImplementationGroupProxy
// Exported proxy for instantiation method resolution
// ----------------------------------------------------------------------------
//
EXPORT_C const TImplementationProxy*
ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
    
    return ImplementationTable;
    }

//  End of File
