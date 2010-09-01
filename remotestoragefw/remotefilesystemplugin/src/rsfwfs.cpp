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
* Description:  Allows creating a Remote Storage FW file system plug-in from
*              : a factory function.
*
*/



// INCLUDE FILES
#include "rsfwfilesystem.h"
#include "rsfwsession.h"
#include "rsfwinterface.h"


// -----------------------------------------------------------------------------
// CreateFileSystem 
// Implements factory function for creating new file systems.
// Allows creating a Remote Storage FW file system plug-in from this DLL.
// Returns: CRemoteFsFileSystem*: pointer to the Remote Storage FW file system 
//                                plug-in main class
// -----------------------------------------------------------------------------
//
extern "C"
    {
    EXPORT_C CFileSystem* CreateFileSystem()
        {
        
        return(CRsfwFileSystem::New());
        
        }
    }

