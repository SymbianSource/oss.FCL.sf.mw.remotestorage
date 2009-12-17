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
* Description:  Encapsulates an asynchronous operation
*
*/

#ifndef C_RSFWRFEASYNCOPERATION_H
#define C_RSFWRFEASYNCOPERATION_H

#include "rsfwrfeoperation.h"

class CRsfwRfeStateMachine;
class CRsfwRfeRequest;
class TRsfwMountConfig;

/**
 *  Encapsulates an asynchronous operation.
 *
 *  Async. operations are implemented as state machines.
 *
 */
class CRsfwRfeAsyncOperation : public CRsfwRfeOperation
    {
public:
    ~CRsfwRfeAsyncOperation();
public:
    CRsfwRfeStateMachine* Implementation();
    void SetImplementation(CRsfwRfeStateMachine*);
    void SetL(CRsfwRfeRequest* aRequest, TInt aOpCode);
private:
    void VerifyMountConfigL(TRsfwMountConfig& aMountConfig);
private:
    CRsfwRfeStateMachine* iImplementation; 
    };

#endif