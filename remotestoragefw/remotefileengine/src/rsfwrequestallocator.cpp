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


#include "rsfwrequestallocator.h"
#include "rsfwrfesyncoperation.h"
#include "rsfwrfeasyncoperation.h"
#include "rsfwrfemessagerequest.h"
#include "mdebug.h"


// ----------------------------------------------------------------------------
// RsfwRequestAllocator::GetSyncOperation
// ----------------------------------------------------------------------------
// 
CRsfwRfeSyncOperation* RsfwRequestAllocator::GetSyncOperation(CRsfwRfeRequest* aRequest,
                                                      TInt aCaller) 
    {
    CRsfwRfeSyncOperation* pO = NULL;
    pO = new CRsfwRfeSyncOperation();
    if (pO)
        {
        pO->Set(aRequest, aCaller);
        }
    return pO;
    }

// ----------------------------------------------------------------------------
// RsfwRequestAllocator::GetAsyncOperation
// ----------------------------------------------------------------------------
// 
CRsfwRfeAsyncOperation* RsfwRequestAllocator::GetAsyncOperation(CRsfwRfeRequest* aRequest,
                                                        TInt aCaller) 
    {
    CRsfwRfeAsyncOperation* pO = NULL;
    pO = new CRsfwRfeAsyncOperation();
    if (pO) 
        {
        TRAPD(err, pO->SetL(aRequest, aCaller));
        if (err) 
            {
        	DEBUGSTRING(("Setting the operation failed with error %d!!!",err));
            delete pO;
            pO = NULL;
            }
        }
    return pO;
    }

// ----------------------------------------------------------------------------
// RsfwRequestAllocator::GetMessageRequest
// allocate requests
// Return a pointer to a message request of type specificied by aCaller and initialised
// ----------------------------------------------------------------------------
// 
CRsfwRfeMessageRequest* RsfwRequestAllocator::GetMessageRequest(
    const RMessage2& aMessage,
    CRsfwRfeSession* aSession)
    {
    CRsfwRfeMessageRequest* pM = NULL;
    
    pM = new CRsfwRfeMessageRequest();
        
    if (pM)
        {
        TRAPD(err, pM->SetL(aMessage,aSession));
        if (err)
        {
        	delete pM;
        	pM = NULL;
        }
        return pM;
        }
    else 
        {
        return NULL;
        }
    }
   
// ----------------------------------------------------------------------------
// RsfwRequestAllocator::FreeRequest
// ----------------------------------------------------------------------------
//     
void RsfwRequestAllocator::FreeRequest(CRsfwRfeRequest* aRequest)
    {
    delete aRequest;
    }


