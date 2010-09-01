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
* Description:  File server interface class representing a format 
*                operation on a disk. Not supported for remote drives.
*
*/


// INCLUDE FILES
#include "rsfwfsformatcb.h"

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CRsfwFsFormatCB::CRsfwFsFormatCB
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CRsfwFsFormatCB::CRsfwFsFormatCB()
    {
    }

// Destructor
CRsfwFsFormatCB::~CRsfwFsFormatCB()
    {
    }

// -----------------------------------------------------------------------------
// CRsfwFsFormatCB::DoFormatStepL
// Performs a formatting step on the drive. The step performed should depend on 
// the values of iMode and iCurrentStep. It can be assumed that there are no 
// resources open on the mount, that the media is formattable, and that the media 
// is not write protected.
//
// If iMode == EQuickFormat, then only meta data is to be written.
// This should be carried out in a single step, with iCurrentStep set
// to zero on completion.
//
// If iMode != EQuickFormat, then the format step performed by
// this function should depend on iCurrentStep. When the function
//  returns with iCurrentStep set to zero, the formatting of the drive is complete.
//
// implementation: not supported for remote drives
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsFormatCB::DoFormatStepL()
    {
    iCurrentStep = 0;
    User::Leave(KErrNotSupported);
    }

// End of File
