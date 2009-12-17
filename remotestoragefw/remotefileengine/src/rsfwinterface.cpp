/*
* Copyright (c) 2002-2004 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  RSFW interface
 *
*/


#include "rsfwinterface.h"
#include <e32std.h>


// ----------------------------------------------------------------------------
// TDirEntAttr::ExternalizeL
// ----------------------------------------------------------------------------
//
EXPORT_C void TDirEntAttr::ExternalizeL(RWriteStream& aStream) const
    {
    aStream.WriteUint32L(iAtt);
    aStream.WriteInt32L(iSize);
    aStream.WriteUint32L(iUid3.iUid);
    aStream.WriteUint32L(I64HIGH(iModified.Int64()));
    aStream.WriteUint32L(I64LOW(iModified.Int64()));
    }

// ----------------------------------------------------------------------------
// TDirEntAttr::InternalizeL
// ----------------------------------------------------------------------------
//
EXPORT_C void TDirEntAttr::InternalizeL(RReadStream& aStream)
    {
    iAtt = aStream.ReadUint32L();
    iSize = aStream.ReadInt32L();
    iUid3.iUid = aStream.ReadUint32L();
    TInt highTime = aStream.ReadUint32L();
    TInt lowTime = aStream.ReadUint32L();
    iModified = MAKE_TINT64(highTime, lowTime);
    }

// ----------------------------------------------------------------------------
// TDirEntAttr::Clear
// ----------------------------------------------------------------------------
//
EXPORT_C void TDirEntAttr::Clear()
    {
    iAtt = 0;
    iSize = 0;
    iModified = 0;
    iUid3.iUid = 0;
    }

// ----------------------------------------------------------------------------
// TDirEnt::ExternalizeL
// ----------------------------------------------------------------------------
//  
EXPORT_C void TDirEnt::ExternalizeL(RWriteStream& aStream) const
    {
    iAttr.ExternalizeL(aStream);
    aStream << iName;
    }

// ----------------------------------------------------------------------------
// TDirEnt::InternalizeL
// ----------------------------------------------------------------------------
//    
EXPORT_C void TDirEnt::InternalizeL(RReadStream& aStream)
    {
    iAttr.InternalizeL(aStream);
    aStream >> iName;
    }

// ----------------------------------------------------------------------------
// TDirEnt::Clear
// ----------------------------------------------------------------------------
//
EXPORT_C void TDirEnt::Clear()
    {
    iAttr.Clear();
    iName.SetLength(0);
    }


// End of File
