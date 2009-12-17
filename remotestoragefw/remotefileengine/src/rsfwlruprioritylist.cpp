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
* Description:  LRU priority list for the file cache
*
*/


#include "rsfwlruprioritylist.h"
#include "rsfwlrulistnode.h"
#include "rsfwfileentry.h"
#include "mdebug.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwfileengine.h"
#include "rsfwfiletable.h"

// ----------------------------------------------------------------------------
// CRsfwLruPriorityList::CRsfwLruPriorityList
// 
// ----------------------------------------------------------------------------
//
CRsfwLruPriorityList::CRsfwLruPriorityList()
    : iHdr(CRsfwLruListNode::iOffset),iIter(iHdr) //construct header & iterator
    {}
    
// ----------------------------------------------------------------------------
// CRsfwLruPriorityList::~CRsfwLruPriorityList
// 
// ----------------------------------------------------------------------------
//
CRsfwLruPriorityList::~CRsfwLruPriorityList()
    {
    CRsfwLruListNode* node;
    
    iIter.SetToFirst();
    node = iIter++;
    while (node)
        {
        node->iLink.Deque();
        delete node;
        node = iIter++;
        }
    }    
  
// ----------------------------------------------------------------------------
// CRsfwLruPriorityList::AddNodeL
// 
// ----------------------------------------------------------------------------
//
void CRsfwLruPriorityList::AddNodeL(CRsfwFileEntry *aFe, TInt aPriority) 
    {
    CRsfwLruListNode* currentNode;
    
    iIter.SetToFirst();
    
    currentNode = iIter++;
    while (currentNode) 
        {
        if (currentNode->iEntryPtr->Fid() == aFe->Fid()) 
            {
            DEBUGSTRING(("LRU list: '%d' already exists on the list",
                         aFe->Fid().iNodeId));
            return;
            }  
        currentNode = iIter++;
        }
      
    // Inserts the specified list element in descending priority order.
    // If there is an existing list element with the same priority, 
    // then the new element is added after the existing element.
    CRsfwLruListNode* newNode = CRsfwLruListNode::NewL(aFe, aPriority);
    iHdr.Add(*newNode);
    DEBUGSTRING(("LRU list: added fid '%d' to the list",
                     aFe->Fid().iNodeId));
    }


// ----------------------------------------------------------------------------
// CRsfwLruPriorityList::RemoveNode
// 
// ----------------------------------------------------------------------------
//
TInt CRsfwLruPriorityList::RemoveNode(CRsfwFileEntry *aFe)
    {
    DEBUGSTRING(("CRsfwLruPriorityList::RemoveNode"));
    // When file is opened, it must be removed from LRU list 
    // as it is not candidate for removal from cache. 
    // Returns KErrNotFound if the file is not found at all
    
    TInt err = KErrNotFound;
    CRsfwLruListNode* currentNode;
    
    iIter.SetToFirst();
    
    currentNode = iIter++;
    while (currentNode) 
        {
        if (currentNode->iEntryPtr->Fid() == aFe->Fid()) 
            {
            currentNode->iLink.Deque();
            delete currentNode;
            err = KErrNone;
            DEBUGSTRING(("LRU list: removed fid '%d' from the list",
                             aFe->Fid().iNodeId));
            break;             
            }  
        currentNode = iIter++;
        }
    return err;
    }

// ----------------------------------------------------------------------------
// CRsfwLruPriorityList::GetAndRemoveFirstEntry
// 
// ----------------------------------------------------------------------------
//
CRsfwFileEntry* CRsfwLruPriorityList::GetAndRemoveFirstEntry() 
    {
    
    CRsfwLruListNode* firstNode = iHdr.First();
    CRsfwFileEntry* firstEntry = NULL;
     
    if (iHdr.IsHead(firstNode)) 
        {
        return NULL; // the head has been reached, and must not be removed  
        }
     
    if (firstNode) 
        {
        firstEntry = firstNode->iEntryPtr;
        firstNode->iLink.Deque();
        delete firstNode;
        }

    DEBUGSTRING(("LRU list: first fid on the list removed, '%d'",
                      firstEntry->Fid().iNodeId));        
    return firstEntry;
    }
    
// ----------------------------------------------------------------------------
// CRsfwFileEntry::ExternalizeL
// ----------------------------------------------------------------------------
//
void CRsfwLruPriorityList::ExternalizeL(RWriteStream& aStream)
    {    
    // start from the end!
    iIter.SetToLast();
    CRsfwLruListNode* currentNode = iIter--;
    while (currentNode) 
        {
        CRsfwFileEntry* currentEntry = currentNode->iEntryPtr;
        if (currentEntry)
            {
            TFid fid = currentEntry->Fid();
            aStream.WriteInt32L(fid.iNodeId);
            aStream.WriteInt32L(fid.iVolumeId);            
            }
        currentNode = iIter--;
        }  
    }

// ----------------------------------------------------------------------------
// CRsfwFileEntry::InternalizeL
// ----------------------------------------------------------------------------
//
void CRsfwLruPriorityList::InternalizeL(RReadStream& aStream, CRsfwVolumeTable* aVolumeTable)
    {
    if ( !aVolumeTable )
        {
        User::Leave(KErrArgument);
        }

    // reset existing list
    iHdr.Reset();

    // get stream size
    MStreamBuf* streamBuf = aStream.Source();
    TInt streamSize = streamBuf->SizeL();    

    TInt i;
    // note i+8 as one entry takes two TInt32 (2 times 4 bytes)
    for ( i = 0; i < streamSize; i = i+8 )
        {
        TFid fid;
        fid.iNodeId = aStream.ReadInt32L();
        fid.iVolumeId = aStream.ReadInt32L();

        // check whether there is no trash in the data being internalized
        if (fid.iVolumeId >= 0 && fid.iVolumeId <= KMaxVolumes && fid.iNodeId > 0)
            {
            // find existing CRsfwFileEntry object based on TFid
            CRsfwVolume* volume = aVolumeTable->iVolumes[fid.iVolumeId];
            if ( volume )
                {
                CRsfwFileEntry* entry = volume->iFileEngine->iFileTable->Root()->Lookup(fid);
                if ( entry )
                    {
                    AddNodeL(entry, ECachePriorityNormal);
                    }
                }            
            }
        else
            {
            DEBUGSTRING(("LRU List: wrong item on the list being internalized!"));
            }
        }
    }

