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
* Description:  Remote File System Plug-in implementation
*
*/


#ifndef CRSFWFSFORMATCB_H
#define CRSFWFSFORMATCB_H

//  INCLUDES
#include <f32fsys.h>

/**
 *  Classes that a plug-in file system must implement. A plug-in 
 *  filesystem must implement CFileSystem, which is a factory class for 
 *  a file system. That class must create objects derived from CMountCB, 
 *  CFileCB, CDirCB and CFormatCB. These are defined in f32fsys.h
 *
 *  @lib eremotefs.fsy
 *  @since Series 60 3.2
 */

// ---------------------------------------------------------------------
// CRsfwFsFormatCB
// ---------------------------------------------------------------------
class CRsfwFsFormatCB : public CFormatCB
    {
public:  // Constructors and destructor 

    /**
     * Constructor.
     */
    CRsfwFsFormatCB();
        
    /**
     * Destructor.
     */
    ~CRsfwFsFormatCB();
        
public:  // Functions from base classes¨
    /**
     * From CFormatCB Performs a formatting step on the drive.
     * @since Series 60 3.2
     * @return 
     */
    void DoFormatStepL();
    };

#endif // CRSFWFSFORMATCB_H


