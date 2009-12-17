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

#ifndef C_RSFWLRULISTNODE_H
#define C_RSFWLRULISTNODE_H

#include <e32base.h>

class CRsfwFileEntry;

/** Priority of the cached entry */
enum TCachePriority
    {
    ECachePriorityNormal
    };

class CRsfwLruListNode : public CBase
    {
public:
    static CRsfwLruListNode* NewLC(CRsfwFileEntry* aFe, TInt aPriority); 
    static CRsfwLruListNode* NewL(CRsfwFileEntry* aFe, TInt aPriority);
    virtual ~CRsfwLruListNode();
public:   
    static const TInt iOffset;
private:
    void ConstructL(CRsfwFileEntry* aFe, TInt aPriority);
    
private:
    TPriQueLink iLink;
    CRsfwFileEntry* iEntryPtr;
    friend class CRsfwLruPriorityList;
    };
    
#endif