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
* Description:  data struct for a volume
*
*/

#ifndef C_RSFWVOLUME_H
#define C_RSFWVOLUME_H

#include "rsfwcontrol.h"


class CRsfwFileEngine;
class CRsfwVolumeTable;

/** default volume inactivity timeout, in seconds */
const TInt KDefaultInactivityTimeout        = 600;  

class CRsfwVolume: public CBase
    {
public:
    ~CRsfwVolume();
    TRsfwMountInfo* MountInfo();
    void GetMountInfo(TRsfwMountInfo& aMountInfo);
    void OperationCompleted();
    void ConnectionStateChanged(TInt aConnectionState);
    
public:
    TRsfwMountInfo    iMountInfo;        // mount configuration information
    CRsfwFileEngine*  iFileEngine;       // remote file engine
    CRsfwVolumeTable* iVolumeTable;      // backpointer to volume table 
    };


#endif