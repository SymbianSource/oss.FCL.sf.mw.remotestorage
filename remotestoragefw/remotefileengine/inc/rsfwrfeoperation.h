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
* Description:  Base class for operation encapsulation
*
*/

#ifndef C_RSFWRFEOPERATION_H
#define C_RSFWRFEOPERATION_H

#include <e32base.h>

#include "rsfwrferequest.h"

/************************
 * Operation Encapsulation 
 ************************/
typedef void (*TRFeRequestFunc)(CRsfwRfeRequest*);


/**
 *  Base class for operation encapsulation
 *
 *
 */
class CRsfwRfeOperation : public CBase
    {
public:
    TBool IsSync() const;
    TInt Function();
    void Set(TInt aOpCode);
public:
    TInt iFunction;
    TBool iIsSync;
    };



#endif