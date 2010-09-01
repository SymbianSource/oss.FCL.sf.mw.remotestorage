/*
* Copyright (c) 2005-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Operation and request allocation and deletion
*
*/

#ifndef RSFWTRequestType_H
#define RSFWTRequestType_H

#include <e32def.h>

class CRsfwRfeSyncOperation;
class CRsfwRfeAsyncOperation;
class CRsfwRfeMessageRequest;
class CRsfwRfeRequest;
class RMessage2;
class CRsfwRfeSession;

/**
 *  Operation and request allocation and deletion
 */
class RsfwRequestAllocator
    {
public:
    static CRsfwRfeSyncOperation* GetSyncOperation(CRsfwRfeRequest* aRequest,
                                               TInt aCaller);
    static CRsfwRfeAsyncOperation* GetAsyncOperation(CRsfwRfeRequest* aRequest,
                                                 TInt aCaller);
    static CRsfwRfeMessageRequest* GetMessageRequest(const RMessage2& aMessage,
                                                 CRsfwRfeSession* aSession);
    // request gets ownership of its operation, and deletes it
    static void FreeRequest(CRsfwRfeRequest* aRequest);
    };
    
#endif