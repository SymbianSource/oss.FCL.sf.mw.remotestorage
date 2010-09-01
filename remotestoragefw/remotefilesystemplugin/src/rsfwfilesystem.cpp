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
* Description:  Factory class for a file system. Allows creating
*               objects derived from CMountCB, CFileCB, CDirCB and CFormatCB.
*
*/


#include "rsfwfilesystem.h"
#include <f32ver.h>
#include "rsfwfsfilecb.h"
#include "rsfwfsdircb.h"
#include "rsfwfsmountcb.h"
#include "rsfwfsformatcb.h"
#include "rsfwinterface.h"

// ============================ MEMBER FUNCTIONS ===============================

// static constructor
CRsfwFileSystem* CRsfwFileSystem::New()
    {
    // non-leaving new, NULL returned for failed creation
    CRsfwFileSystem *self = new CRsfwFileSystem;
    return self;      
    }

// -----------------------------------------------------------------------------
// CRsfwFileSystem::CRsfwFileSystem
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CRsfwFileSystem::CRsfwFileSystem()
    {
    }

// Destructor
CRsfwFileSystem::~CRsfwFileSystem()
    {
    }
  
// -----------------------------------------------------------------------------
// CRsfwFileSystem::Install
// Installs the file system.

// The function sets the name of the file system object through a call
// to CObject::SetName(), thus making it accessible, internally, 
// using FileSystems->FindByFullName(). This enables the file server
// to find and handle installed file systems. The function also sets 
// unique identifier for the file system and the file system version.
// The version is determined by the file system implementation.
// It is used in calls to CFileSystem::QueryVersionSupported().
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CRsfwFileSystem::Install()
    {
    
    iVersion = TVersion(KF32MajorVersionNumber,
                            KF32MinorVersionNumber,
                            KF32BuildVersionNumber);

    TTime timeID;
    timeID.HomeTime();
    iUniqueID = I64LOW(timeID.Int64());

    return (SetName(&KRemoteFSName));
  
    }

// -----------------------------------------------------------------------------
// CRsfwFileSystem::Remove
// This is called just before the file system object is destroyed, and allows
// any clean up to be carried out.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CRsfwFileSystem::Remove()
    {
    
    return KErrNone;  
  
    }

// -----------------------------------------------------------------------------
// CRsfwFileSystem::NewMountL
// Creates a new remote mount control block, a CMountCB derived object. 
// On success, a pointer to the new mount object is returned,
// otherwise the function leaves.
//
// This function is defined as a const function in the base class CFileSystem.
// However, we need to pass to the mount class modifiable pointers to 
// the shared memory chunks used in the parameter passing. That's why we need 
// to cast away const.
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CMountCB* CRsfwFileSystem::NewMountL() const
    {
    
    return const_cast<CRsfwFileSystem*>(this)->NewRemoteFsMountL();
  
    }

 
// -----------------------------------------------------------------------------
// CRsfwFileSystem::NewFileL
// Creates a new remote file control block, i.e. a CFileCB derived object.
// On success, a pointer to the new file object is returned,
// otherwise the function leaves.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CFileCB* CRsfwFileSystem::NewFileL() const
    {
    return new(ELeave) CRsfwFsFileCB();
    }


// -----------------------------------------------------------------------------
// CRsfwFileSystem::NewDirL
// Creates a new remote directory control block, i.e. a CDirCB derived object.
// On success, a pointer to the new directory control block is returned,
// otherwise the function leaves.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CDirCB* CRsfwFileSystem::NewDirL() const
    {
    return(CRsfwFsDirCB::NewL());
    }
   
// -----------------------------------------------------------------------------
// CRsfwFileSystem::NewFormatL
// Creates a new remote volume format control block, i.e. a CFormatCB derived object.
// On success, a pointer to the new volume format control block is returned,
// otherwise the function leaves.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CFormatCB* CRsfwFileSystem::NewFormatL() const
    {
    return new(ELeave) CRsfwFsFormatCB();
    }

// -----------------------------------------------------------------------------
// CRsfwFileSystem::DriveInfo
// Retrieves drive information.
// The function assumes that we are not handling different local drives, and 
// sets anInfo.iMediaAtt, anInfo.iDriveAtt and anInfo.iType to values "sensible"
// for remote drives discarding the specified drive number. For local drives 
// the function would obtain the necessary information by calling the appropriate 
// TBusLocalDrive::Caps() function using the argument aDriveNumber.

// Note that and anInfo.iBatteryState will already have been
// set by the calling function.
    
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFileSystem::DriveInfo(
    TDriveInfo& anInfo,
    TInt /* aDriveNumber */) const
    {
    
    anInfo.iType = EMediaRemote;
    anInfo.iMediaAtt = KMediaAttVariableSize; 
    anInfo.iDriveAtt = KDriveAttRemote;

    }

// -----------------------------------------------------------------------------
// CRsfwFileSystem::NewRemoteFsMountL
// Creates a new remote mount control block, a CMountCB derived object. 
// On success, a pointer to the new mount object is returned,
// otherwise the function leaves.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
CMountCB* CRsfwFileSystem::NewRemoteFsMountL()
    {
    
    return CRsfwFsMountCB::NewL();
  
    }

// -----------------------------------------------------------------------------
// CRsfwFileSystem::DefaultPath
// Returns the default path for the file system. 
// Always returns "C:\\".
//
// Each session with the file server has a current session path.
// When a new session is opened, its session path is set to the default path
// of the file server. At file server start-up, this default path is set to the 
// default path returned by the local file system. 
// The default implementation in the base class raises an "Fserv fault" 31 panic.
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CRsfwFileSystem::DefaultPath(
    TDes& aPath) const
    {

    _LIT(KDefaultPath, "C:\\");
    aPath = KDefaultPath;
    return (KErrNone);
  
    }
 
