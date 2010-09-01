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


#ifndef CRSFWFILESYSTEM_H
#define CRSFWFILESYSTEM_H

#include <f32fsys.h>


/**
 *  Classes that a plug-in file system must implement.
 *
 *  A plug-in filesystem must implement CFileSystem, which is a factory 
 *  class for a file system. That class must create objects derived from 
 *  CMountCB, CFileCB, CDirCB and CFormatCB. These are defined in f32fsys.h
 *
 *  @lib eremotefs.fsy
 *  @since Series 60 3.2
 */
class CRsfwFileSystem : public CFileSystem
    {
public: // Constructors and destructor
    /**
     * Static constructor.
     */
    static CRsfwFileSystem* New();
        
    /**
     * Constructor.
     */    
    CRsfwFileSystem();
        
    /**
     * Destructor.
     */      
    ~CRsfwFileSystem();
        
public: // Functions from base class
    /**
     * From CFileSystem Installs the file system.
     * @since Series 60 3.2
     * @return
     */
    TInt Install();   
        
    /**
     * From CFileSystem Creates a new mount control block.
     * @since Series 60 3.2
     * @return A pointer to the new mount object.
     */    
    CMountCB* NewMountL() const;
        
    /**
     * From CFileSystem Creates a new file control block.
     * @since Series 60 3.2
     * @return A pointer to the new file object.
     */          
    CFileCB* NewFileL() const;
        
    /**
     * From CFileSystem Creates a new directory control block.
     * @since Series 60 3.2
     * @return A pointer to the new directory object.
     */            
    CDirCB* NewDirL() const;
        
    /**
     * From CFileSystem Creates a new volume format control block.
     * @since Series 60 3.2
     * @return  A pointer to the new volume format object.
     */        
    CFormatCB* NewFormatL() const;
        
    /**
     * From CFileSystem Retrieves drive information.
     * @since Series 60 3.2
     * @param anInfo       On return, contains the drive information.
     * @param aDriveNumber The drive number.
     * @return  
     */        
    void DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const;
          
    /**
     * From CFileSystem Returns the default path for the file system. 
     * @since Series 60 3.2
     * @param aPath  On return, contains the default path.
     * @return KErrNone or an appropriate error code when the default path
     *         cannot be supplied.
     */              
    TInt DefaultPath(TDes& aPath) const;


    /**
     * From CFileSystem Does clean up before the filesystem is destroyed.
     * @since Series 60 3.2
     * @return  An error code.
     */        
    TInt Remove();
        
private:
    /**
     * Creates a new mount control block.
     * @since Series 60 3.2
     * @return A pointer to the new mount object.
     */  
    CMountCB* NewRemoteFsMountL();

public:   // Data       
    // unique id of the file system 
    TUint iUniqueID; 

    };

#endif // CRSFWFILESYSTEM_H


