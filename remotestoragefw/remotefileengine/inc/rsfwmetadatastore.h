/*
* Copyright (c) 2004-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Keeps metadata persistent
*
*/


#ifndef C_RSFWMETADATASTORE_H
#define C_RSFWMETADATASTORE_H

// INCLUDES
#include "rsfwcontrol.h"
#include "rsfwpermanentstore.h"

// CLASS DECLARATION
class CRsfwMetaDataStore: public CRsfwPermanentStore
    {

public:
    static CRsfwMetaDataStore* NewL(const TDesC& aPath);
    static CRsfwMetaDataStore* NewLC(const TDesC& aPath);
    void GetMountConfigL(TRsfwMountConfig& aMountConfig);
    void SetMountConfigL(const TRsfwMountConfig& aMountConfig);

private:
    void ConstructL(const TDesC& aPath);
    };

#endif // METADATASTORE_H

// End of File
