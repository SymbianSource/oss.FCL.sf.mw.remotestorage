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
* Description:  Base class for request encapsulation.
*
*/


#include <f32file.h>
#include "rsfwrferequest.h"
#include "rsfwinterface.h"
#include "rsfwrfeoperation.h"
#include "rsfwrfesyncoperation.h"
#include "rsfwrfeasyncoperation.h"
#include "rsfwrfestatemachine.h"
#include "rsfwrequestallocator.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwfileengine.h"

TParse dummyP;
RMessage2 dummyM;


// ----------------------------------------------------------------------------
// CRsfwRfeRequest::~CRsfwRfeRequest
// ----------------------------------------------------------------------------
// 
CRsfwRfeRequest::~CRsfwRfeRequest()
    {
    delete iInArgs;
    delete iOutArgs;
    if (iOperation) 
        {
        if (Operation()->IsSync()) 
            {
            delete (CRsfwRfeSyncOperation *)iOperation;
            iOperation = NULL;
            }
        else 
            {
            delete (CRsfwRfeAsyncOperation *)iOperation;
            iOperation = NULL;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRfeRequest::Destroy
// ----------------------------------------------------------------------------
// 
void CRsfwRfeRequest::Destroy()
    {
    CRsfwVolume* volume = iVolume;
    CRsfwVolumeTable* volumeTable = iVolumeTable;

    RsfwRequestAllocator::FreeRequest(this);

    if (volume && volume->iFileEngine)
        {
        // Signal the engine of operation completion
        volume->iFileEngine->OperationCompleted();
        }
    else
        {
        volumeTable->OperationCompleted(NULL);
        }
    }
    
// ----------------------------------------------------------------------------
// CRsfwRfeRequest::Dispatch
// ----------------------------------------------------------------------------
//     
void CRsfwRfeRequest::Dispatch()
    {
    if (Operation()->IsSync()) 
        {
        TRAPD(leaveValue,
              ((CRsfwRfeSyncOperation * )Operation())->DoRequestL(this));
        CompleteAndDestroy(leaveValue);  
        }
    else 
        {
        // run the operation state machine
        // start from the initial state
        ((CRsfwRfeAsyncOperation *)Operation())->Implementation()->
            ReEnterCurrentState();
        }
    }

// ----------------------------------------------------------------------------
// CRsfwRfeRequest::Src
// ----------------------------------------------------------------------------
//     
TParse& CRsfwRfeRequest::Src()
    {
    return(dummyP);
    }

// ----------------------------------------------------------------------------
// CRsfwRfeRequest::Dest
// ----------------------------------------------------------------------------
// 
TParse& CRsfwRfeRequest::Dest()
    {
    return(dummyP);
    }

// ----------------------------------------------------------------------------
// CRsfwRfeRequest::Operation
// ----------------------------------------------------------------------------
//     
CRsfwRfeOperation* CRsfwRfeRequest::Operation() 
    {return(iOperation);}

// ----------------------------------------------------------------------------
// CRsfwRfeRequest::SetOperation
// ----------------------------------------------------------------------------
// 
void CRsfwRfeRequest::SetOperation(CRsfwRfeOperation* aCaller) 
    {
    iOperation = aCaller;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeRequest::RequestType
// ----------------------------------------------------------------------------
// 
TRequestType CRsfwRfeRequest::RequestType() 
    {
    return iRequestType;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeRequest::SetRequestType
// ----------------------------------------------------------------------------
// 
void CRsfwRfeRequest::SetRequestType(TRequestType aRequestType) 
    {
    iRequestType = aRequestType;
    }


