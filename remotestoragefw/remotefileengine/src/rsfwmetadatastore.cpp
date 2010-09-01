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


// INCLUDE FILES
#include <s32mem.h>

#include "rsfwmetadatastore.h"
#include "mdebug.h"

// CONSTANTS
const TUint32 KMetaDataStoreVersion        = 0x010101; // current version
const TInt KMaxExternalizedMountConfigSize = 512;
const TInt KDefaultMetaDataBlockSize       = 128;

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// CRsfwMetaDataStore::NewL
// ----------------------------------------------------------------------------
//
CRsfwMetaDataStore* CRsfwMetaDataStore::NewL(const TDesC& aPath)
    {
    DEBUGSTRING(("CRsfwMetaDataStore::NewL"));
    CRsfwMetaDataStore* self = CRsfwMetaDataStore::NewLC(aPath);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwMetaDataStore::NewLC
// ----------------------------------------------------------------------------
//    
CRsfwMetaDataStore* CRsfwMetaDataStore::NewLC(const TDesC& aPath)
    {
    DEBUGSTRING(("CRsfwMetaDataStore::NewLC"));
    CRsfwMetaDataStore* self = new (ELeave) CRsfwMetaDataStore();
    CleanupStack::PushL(self);
    self->ConstructL(aPath);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwMetaDataStore::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwMetaDataStore::ConstructL(const TDesC& aPath)
    {
    DEBUGSTRING(("CRsfwMetaDataStore::ConstructL"));
    CRsfwPermanentStore::ConstructL(aPath,
                                KMaxExternalizedMountConfigSize,
                                KDefaultMetaDataBlockSize);
    }

// ----------------------------------------------------------------------------
// CRsfwMetaDataStore::GetMountConfigL
// ----------------------------------------------------------------------------
//
void CRsfwMetaDataStore::GetMountConfigL(TRsfwMountConfig& aMountConfig)
    {
    // Load the configuration information
    DEBUGSTRING(("CRsfwMetaDataStore::GetMountConfigL"));
    const HBufC8* header = Header();
    if (header)
        {
        RMemReadStream stream(header->Ptr(), header->Length());
        CleanupClosePushL(stream);
        TUint32 version = stream.ReadUint32L();
        if (version != KMetaDataStoreVersion)
            {
            DEBUGSTRING(("metadata store version 0x%x differs from 0x%x",
                         version,
                         KMetaDataStoreVersion));
            User::Leave(KErrCorrupt);
            }
        aMountConfig.InternalizeL(stream);
        CleanupStack::PopAndDestroy(&stream); // stream
        }
    else
        {
        User::Leave(KErrNotFound);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMetaDataStore::SetMountConfigL
// ----------------------------------------------------------------------------
//
void CRsfwMetaDataStore::SetMountConfigL(const TRsfwMountConfig& aMountConfig)
    {
    // Store the configuration information
    HBufC8* buf = HBufC8::NewLC(KMaxExternalizedMountConfigSize);
    TPtr8 ptr = buf->Des();
    TUint8* data = const_cast<TUint8 *>(ptr.Ptr());
    RMemWriteStream stream(data, KMaxExternalizedMountConfigSize);
    CleanupClosePushL(stream);

    // Dump the externalized data in the memory buffer
    stream.WriteUint32L(KMetaDataStoreVersion);
    aMountConfig.ExternalizeL(stream);
    MStreamBuf* streamBuf = stream.Sink();
    TInt dataLen = streamBuf->TellL(MStreamBuf::EWrite).Offset();
    DEBUGSTRING(("mount config data len = %d,", dataLen));
    stream.CommitL();
    TPtrC8 header(data, dataLen);
    SetHeaderL(header);

    CleanupStack::PopAndDestroy(2, buf); // stream, buf
    }

// End of File
