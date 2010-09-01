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
* Description:  LRU priority list for cache entries
*
*/

#ifndef C_RSFWLRUPRIORITYLIST_H
#define C_RSFWLRUPRIORITYLIST_H

#include <e32base.h>
#include <s32strm.h>

class CRsfwLruListNode;
class CRsfwFileEntry;
class CRsfwVolumeTable;

class CRsfwLruPriorityList : public CBase
    {
public:
    CRsfwLruPriorityList();
    virtual ~CRsfwLruPriorityList();
   
    void AddNodeL(CRsfwFileEntry *aFe, TInt aPriority);
    TInt RemoveNode(CRsfwFileEntry *aFe);
    CRsfwFileEntry* GetAndRemoveFirstEntry();
    void ExternalizeL(RWriteStream& aStream);
    void InternalizeL(RReadStream& aStream, CRsfwVolumeTable* aVolumeTable);
    
private:
    TPriQue<CRsfwLruListNode> iHdr;
    TDblQueIter<CRsfwLruListNode> iIter;
    };


#endif