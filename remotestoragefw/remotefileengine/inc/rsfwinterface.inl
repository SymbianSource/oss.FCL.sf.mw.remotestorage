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
* Description:  inlines for rsfw interface
 *
*/



inline TBool TFid::operator==(const TFid& aFid) const
    {
    return ((iNodeId == aFid.iNodeId) && (iVolumeId == aFid.iVolumeId));
    }

// End of File
