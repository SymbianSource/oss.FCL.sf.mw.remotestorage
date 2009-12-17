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
* Description:  A node in the LRU list
*
*/


#include "rsfwlrulistnode.h"

const TInt CRsfwLruListNode::iOffset    = _FOFF(CRsfwLruListNode,iLink);


// ----------------------------------------------------------------------------
// CRsfwLruListNode::NewLC
// 
// ----------------------------------------------------------------------------
//
CRsfwLruListNode* CRsfwLruListNode::NewLC(CRsfwFileEntry *aFe, TInt aPriority)
    {
    CRsfwLruListNode* self = new (ELeave) CRsfwLruListNode;
    CleanupStack::PushL(self);
    self->ConstructL(aFe, aPriority);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwLruListNode::NewL
// 
// ----------------------------------------------------------------------------
//
CRsfwLruListNode* CRsfwLruListNode::NewL(CRsfwFileEntry *aFe, TInt aPriority)
    {
    CRsfwLruListNode* self = CRsfwLruListNode::NewLC(aFe, aPriority);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwLruListNode::ConstructL
// 
// ----------------------------------------------------------------------------
//
void CRsfwLruListNode::ConstructL(CRsfwFileEntry *aFe, TInt aPriority)
    {
    iEntryPtr = aFe;
    iLink.iPriority = aPriority;
    }

// ----------------------------------------------------------------------------
// CRsfwLruListNode::~CRsfwLruListNode
// 
// ----------------------------------------------------------------------------
//
CRsfwLruListNode::~CRsfwLruListNode()
    {
    }

