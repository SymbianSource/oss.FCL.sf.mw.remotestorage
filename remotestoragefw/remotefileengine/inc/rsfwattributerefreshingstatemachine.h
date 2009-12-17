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
* Description:  States that need to refresh file or directory attributes
*
*/

#ifndef C_RSFW_ATTRIBUTEREFRESHINGSTATEMACHINE_H
#define C_RSFW_ATTRIBUTEREFRESHINGSTATEMACHINE_H

#include "rsfwrfestatemachine.h"

// FORWARD DECLARATIONS
class CRsfwDirEntAttr;

/**
 *  Parent class for operations that need to refresh file or 
 *	directory attributes.
 *
 */
 class CRsfwAttributeRefreshingStateMachine : public CRsfwRfeStateMachine   
    {
 public:
    ~CRsfwAttributeRefreshingStateMachine(); 
 public:
    // file or directory attributes
    CRsfwDirEntAttr* iDirEntAttr;
    // The old attributes for a cached file or directory.  
    // Comparing the tells whether the actual data must 
    // be fetched from the server.
    CRsfwDirEntAttr* iDirEntAttrOld;    
    };


#endif // C_RSFW_ATTRIBUTEREFRESHINGSTATEMACHINE_H