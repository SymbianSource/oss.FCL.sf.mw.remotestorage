/*
* Copyright (c) 2003-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  inlines for file table data structure
*
*/


// ----------------------------------------------------------------------------
// CRsfwFileTable::Volume
// ----------------------------------------------------------------------------
// 
inline CRsfwVolume* CRsfwFileTable::Volume()
    {
    return iVolume;
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::Root
// ----------------------------------------------------------------------------
//
inline CRsfwFileEntry* CRsfwFileTable::Root()
    {
    return iRootFep;
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::Permanence
// ----------------------------------------------------------------------------
//
inline const TBool CRsfwFileTable::Permanence() const
    {
    return iPermanence;
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::OpenFileCount
// ----------------------------------------------------------------------------
//
inline TInt CRsfwFileTable::OpenFileCount()
    {
    return iOpenFileCount;
    }

// ----------------------------------------------------------------------------
// CRsfwFileTable::UpdateOpenFileCount
// ----------------------------------------------------------------------------
//    
inline void CRsfwFileTable::UpdateOpenFileCount(TInt aDelta)
    {
    iOpenFileCount += aDelta;
    if (iOpenFileCount < 0)
        {
        iOpenFileCount = 0;
        }
    }