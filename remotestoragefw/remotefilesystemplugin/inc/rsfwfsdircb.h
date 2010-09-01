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


#ifndef CRSFWFSDIRCB_H
#define CRSFWFSDIRCB_H

//  INCLUDES
#include <f32fsys.h>
#include <s32file.h>
//#include "rsfwsession.h"
#include "rsfwinterface.h"

// CONSTANTS

// DATA TYPES

// FORWARD DECLARATIONS

// CLASS DECLARATIONS

/**
 *  Classes that a plug-in file system must implement. A plug-in 
 *  filesystem must implement CFileSystem, which is a factory class for 
 *  a file system. That class must create objects derived from CMountCB, 
 *  CFileCB, CDirCB and CFormatCB. These are defined in f32fsys.h
 *
 *  @lib eremotefs.fsy
 *  @since Series 60 3.2
 */
class CRsfwFsDirCB : public CDirCB
    {
public:   // Constructors and destructor
    
    /**
     * Static constructor.
     */      
    static CRsfwFsDirCB* NewL();
        
    /**
     * Destructor.
     */
    ~CRsfwFsDirCB();
    
public:   // New functions
    
    /**
     * Prepares the class to read directory entries from a local cache container file
     * @since Series 60 3.2
     * @param aPath path of the directory container in the local cache
     * @param aName the entries to be read from the directory, for example, "*"
     * @return 
     */
    void SetDirL( const TDesC& aPath, 
                  const TDesC& aName );
    
public:   // Functions from base classes
    
    /**
     * From CDirCB Gets information from the first suitable entry in the directory,
     *             starting from the current read position.
     * @since Series 60 3.2
     * @param anEntry Entry information object.
     * @return 
     */
    void ReadL( TEntry& anEntry );
        
private:
    
    /**
     * C++ default constructor.
     */
    CRsfwFsDirCB();
        
public:     // Data

    // the Fid of this dir 
    TFid iThisFid; 

private:    // Data
    
    // stream attached to the local cache container file
    RFileReadStream iDirContReadStream;
        
    // the entries to be read from the directory, for example, "*"
    HBufC* iMatch;
        
    // whether the cache container includes the contents the directory
    TBool iHasBeenFetched;
        
    // which entry position to read next
    TInt iEntryPos;   
          
    // at which stream position to read next
    TStreamPos iStreamPos;   
        
    // where to read if we must re-read the last entry to be read
    TStreamPos iPendingPos;
        
    }; 

#endif // CRSFWFSDIRCB_H

// End of File

