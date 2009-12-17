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
* Description:  Definitions that are common to the Access API and Control API
 *
*/

#ifndef RSFWCOMMON_H
#define RSFWCOMMON_H

// INCLUDES
#include <e32std.h>

// names needed when starting the server.
_LIT(KRfeSemaphoreName, "RfeSemaphore");
// the name of the server, used in Connect()
_LIT(KRfeServerName, "remotefe");

// special values for IAP selection
_LIT(KIapDefaultPreferences, "*");
_LIT(KIapAskUser,            "?");

// the server version
// A version must be specified when creating a session with the server
const TUint KRfeMajorVersionNumber = 0;
const TUint KRfeMinorVersionNumber = 1;
const TUint KRfeBuildVersionNumber = 1;

// DATA TYPES
// opcodes used in message passing between client and server
// opcodes used in message passing between client and server
enum TRfeRqst
    {
    ERfeRequest,
    EAsynchRequest,
    EMount,
    EMountByDriveLetter,
    EDismountByVolumeId,
    EDismountByDriveLetter,
    EGetMountList,
    EGetMountInfo,
    ESetMountConnectionState,
    EDirRefresh,
    ECancelAll,
    ERenameReplace,
    ESetAttr,
    EFsIoctl,
    EGetAttr,
    EOpenByPath,
    EFsRoot,
    EMkDir,
    ERemoveDir,
    ECreateFile,
    ERemove,
    ELookUp,
    EClose,
    EFlush,
    EFetch,
    EFetchData,
    EOkToWrite,
    EMaxRfeOperations
    };

#endif  // RSFWCOMMON_H

// End of File
