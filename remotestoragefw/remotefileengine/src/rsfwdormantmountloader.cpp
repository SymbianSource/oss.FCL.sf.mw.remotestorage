/*
* Copyright (c) 2005 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  class for restoring dormant mounts asynchronously when server starts
*
*/


// INCLUDE FILES
#include "rsfwdormantmountloader.h"
#include "rsfwvolume.h"
#include "rsfwfileengine.h"
#include "rsfwfiletable.h"
#include "mdebug.h"


// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CRsfwDormantMountLoader::CRsfwDormantMountLoader
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CRsfwDormantMountLoader::CRsfwDormantMountLoader() :
    CActive( CActive::EPriorityUserInput )
    {
    
    }

// -----------------------------------------------------------------------------
// CRsfwDormantMountLoader::ConstructL
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CRsfwDormantMountLoader::ConstructL(CRsfwVolumeTable *aTheTable)
    {
    DEBUGSTRING(("CRsfwDormantMountLoader::ConstructL"));
    iVolumeTable = aTheTable;
    CActiveScheduler::Add( this );
    iTimer.CreateLocal();
    iTimer.After(iStatus, KDormantLoaderDelay);
    SetActive();
    }

// -----------------------------------------------------------------------------
// CRsfwDormantMountLoader::NewL()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
EXPORT_C CRsfwDormantMountLoader* CRsfwDormantMountLoader::NewL(CRsfwVolumeTable *aTheTable)
    {
    DEBUGSTRING(("CRsfwDormantMountLoader::NewL"));
    CRsfwDormantMountLoader* self = new ( ELeave ) CRsfwDormantMountLoader();

    CleanupStack::PushL( self );
    self->ConstructL(aTheTable);
    CleanupStack::Pop();

    return self;
    }

// -----------------------------------------------------------------------------
// Destructor
// -----------------------------------------------------------------------------
//
EXPORT_C CRsfwDormantMountLoader::~CRsfwDormantMountLoader()
    {
    DEBUGSTRING16(("CRsfwDormantMountLoader::~CRsfwDormantMountLoader"));
    Deque();
    iTimer.Close();
    }
    
    
// -----------------------------------------------------------------------------
// CRsfwDormantMountLoader::SaveDirtyFiles
// -----------------------------------------------------------------------------
//   
 void CRsfwDormantMountLoader::ResolveDirtyFilesL()
    {
    DEBUGSTRING16(("CRsfwDormantMountLoader::ResolveDirtyFilesL"));   
    for (int i = 0; i < KMaxVolumes; i++) 
        {
        if (iVolumeTable) 
            { 
            DEBUGSTRING16(("CRsfwDormantMountLoader::ResolveDirtyFilesL drive %d", i)); 
            CRsfwVolume* volume = iVolumeTable->iVolumes[i];
            if (volume && volume->iFileEngine && volume->iFileEngine->iFileTable) 
                {
                DEBUGSTRING16(("drive contains active remote mount")); 
                volume->iFileEngine->iFileTable->ResolveDirtyFilesL();
                }
            }
        }
    }
    
// -----------------------------------------------------------------------------
// CRsfwDormantMountLoader::RunL
// Handles an active object’s request completion event.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwDormantMountLoader::RunL()
    {
    DEBUGSTRING(("CRsfwDormantMountLoader::RunL"));
    ResolveDirtyFilesL();
    iVolumeTable->DeleteTheMarker();
    iVolumeTable->iDormantMountRestorePending = EFalse;
    // "Simulate" operation completion (which may result in RFE shutdown)
    iVolumeTable->OperationCompleted(NULL);
    }
    
// ----------------------------------------------------------------------------
// CRsfwDormantMountLoader::RunError
// ----------------------------------------------------------------------------
//
TInt CRsfwDormantMountLoader::RunError(TInt /*aError*/)
    {
    DEBUGSTRING16(("CRsfwDormantMountLoader::RunError"));		
    return KErrNone;
	}    
    
// -----------------------------------------------------------------------------
// CRsfwDormantMountLoader::DoCancel
// -----------------------------------------------------------------------------
//  
void CRsfwDormantMountLoader::DoCancel() 
	{
	DEBUGSTRING(("CRsfwDormantMountLoader::RunL"));	
	}  


// -----------------------------------------------------------------------------

//  End of File 
