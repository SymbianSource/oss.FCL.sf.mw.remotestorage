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
* Description:  New file attributes defined in the subsystem
 *
*/


#ifndef RSFW_FILE_H
#define RSFW_FILE_H

// CONSTANTS
// Other file attributes are defined in \epoc32\include\f32file.h and
// f32\inc\common.h
// KEntryAttCachePriorityHigh is reserved for possible "briefcase" use cases
const TUint KEntryAttCachePriorityHigh=0x10000; // bit 16


// A file attribute that marks the file as having been modified.
// from common.h
const TUint KEntryAttModified=0x20000000;

#endif // RSFW_FILE_H

// End of File
