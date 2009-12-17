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


#include "rsfwattributerefreshingstatemachine.h"
#include "rsfwdirentattr.h"  

// ----------------------------------------------------------------------------
// CRsfwAttributeRefreshingStateMachine::~CRsfwAttributeRefreshingStateMachine
// ----------------------------------------------------------------------------
//  
CRsfwAttributeRefreshingStateMachine::~CRsfwAttributeRefreshingStateMachine() 
    {
    delete iDirEntAttr;
    delete iDirEntAttrOld;
    }
    

