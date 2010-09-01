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
* Description:  Encapsulates synchronous operation
*
*/

#ifndef C_RSFWRFESYNCOPERATION_H
#define C_RSFWRFESYNCOPERATION_H

#include "rsfwrfeoperation.h"



/**
 *  Encapsulates a synchronous operation
 *
 *  Synchronous operation can be implemented as a simple function pointer.
 *
 */
 class CRsfwRfeSyncOperation : public CRsfwRfeOperation 
    {
public:
    void DoRequestL(CRsfwRfeRequest* aRequest);
    void Set(CRsfwRfeRequest* aRequest, TInt aCaller);
private:
    TRFeRequestFunc iDoRequestL;
    };

#endif

