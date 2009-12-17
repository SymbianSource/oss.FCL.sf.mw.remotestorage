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


#ifndef CRSFWFSMOUNTCB_H
#define CRSFWFSMOUNTCB_H

//  INCLUDES
#include <f32fsys.h>
#include "rsfwsession.h"
#include "rsfwinterface.h"

class CRsfwFileSystem;

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
class CRsfwFsMountCB : public CMountCB
    {
public:   // Constructors and destructor
    
    /**
     * Static constructor.
     */      
    static CRsfwFsMountCB* NewL();
            
    /**
     * Destructor.
     */  
    ~CRsfwFsMountCB();
    
public:   // New functions
    
                          
    /**
     * Renames a file.
     * @since Series 60 3.2
     * @param aDirFid fid of the parent directory of the file to be renamed
     * @param aSourceName A reference to a descriptor containing the name
     *                    of the file to be renamed.
     * @param aNewName A reference to a descriptor containing the new full entry
     *                 name for the entry to be renamed.
     * @return 
     */                          
    void RenameFidL( TFid aDirFid,
                     const TDesC& aSourceName, 
                     const TDesC& aNewName );
                     
        
    /**
     * Returnes a handle to the Remote File Engine session
     * @since Series 60 3.1
     * @return a session handle.
     */        
    RRsfwSession* RSessionL();
             
        
public:   // Functions from base classes
        
    /**
     * From CMountCB Sets the mount control block properties.
     * @since Series 60 3.2
     * @param aForceMount Indicates whether the properties of a corrupt
     *                    volume should be stored.
     * @return
     */        
    void MountL( TBool aForceMount );
        
    /**
     * From CMountCB Checks whether the mount control block represents the current mount on
     * the associated drive.
     * @since Series 60 3.2
     * @return KErrNone if the mount represented by this object is found to be
     the current mount;
     KErrGeneral if this object is found not to represent
     the current mount;
     otherwise one of the other sytem wide error codes.
    */                
    TInt ReMount( );
        
    /**
     * From CMountCB Carries out any clean-up necessary for a volume dismount. 
     * @since Series 60 3.2
     * @return 
     */                        
    void Dismounted( );
        
    /**
     * From CMountCB Gets volume information.
     * @since Series 60 3.2
     * @param aVolume On return, a reference to the filled volume
     *                information object.
     * @return 
     */                                
    void VolumeL( TVolumeInfo& aVolume ) const;
        
        
    /**
     * From CMountCB Sets the volume name for the mount.
     * @since Series 60 3.2
     * @param aName A reference to a descriptor containing the new volume name.
     * @return 
     */                                        
    void SetVolumeL( TDes& aName );
    
    /**
     * From CMountCB Creates a new directory on the mount.
     * @since Series60 3.2
     * @param aName A reference to a descriptor containing the full name of
     the directory to be created.
     * @return
     */
    void MkDirL( const TDesC& aName );
        
    /**
     * From CMountCB Removes the directory specified by aName from the volume.
     * @since Series60 3.2
     * @param aName A reference to a descriptor containing the full name of
     the directory to be removed.
     * @return
     */      
    void RmDirL( const TDesC& aName );
        
    /**
     * From CMountCB Deletes the specified file from the mount.
     * @since Series60 3.2
     * @param aName A reference to a descriptor containing the full path name
     of the file that will be removed.
     * @return
     */            
    void DeleteL( const TDesC& aName );
        
    /**
     * From CMountCB Renames or moves a single file or directory on the mount.
     * @since Series60 3.2
     * @param anOldName A reference to a descriptor containing the full entry
     *                  name of the entry to be renamed.
     * @param anNewName A reference to a descriptor containing the new full entry
     *                 name for the entry to be renamed.
     * @return
     */              
    void RenameL( const TDesC& anOldName,
                  const TDesC& aNewName );
          
    /**
     * From CMountCB Replaces one file on the mount with another.
     * @since Series60 3.2
     * @param anOldName A reference to a descriptor containing the full file name
     *                  of the file to replace the file specified by anNewName
     * @param anNewName A reference to a descriptor containing the new full file
     *                  name for the entry to be replaced.
     * @return
     */          
    void ReplaceL( const TDesC& anOldName,
                   const TDesC& aNewName );
        
        
    /**
     * From CMountCB Gets the entry details for the specified file or directory.
     * @since Series60 3.2
     * @param aName   A reference to a descriptor containing the full name of
     *                the entry whose details are required.
     * @param anEntry On return, a reference to the filled entry object.
     * @return
     */                
    void EntryL( const TDesC& aName,
                 TEntry& anEntry ) const;
        
    /**
     * From CMountCB Sets entry details for a specified file or directory.
     * @since Series60 3.2
     * @param aName  A reference to a descriptor containing the full name of
     *               the entry to be updated.
     * @param aTime  A reference to the time object holding the new universal
     *               modified time for aName.
     * @param aSetAttMask   Attribute mask for setting the entry's attributes.
     * @param aClearAttMask Attribute mask for clearing the entry's attributes.
     * @return
     */
    void SetEntryL( const TDesC& aName,
                    const TTime& aTime,
                    TUint aMask,
                    TUint aVal );
        
    /**
     * From CMountCB Opens a new or existing file on the mount.
     * @since Series 60 3.2
     * @param aName  The full name of the file that will be opened.
     * @param aMode  The file share mode. The following share modes are available:
     *               EFileShareExclusive;
     *               EFileShareReadersOnly;
     *               EFileShareAny;
     *               EFileStream;
     *               EFileStreamText;
     *               EFileRead;
     *               EFileWrite.
     * @param anOpen Indicates how the file will be opened. It can be one of
     *               the following:
     *               EFileOpen;
     *               EFileCreate;
     *               EFileReplace.
     * @param aFile  Pointer to the file control block which will, on success,
     *               represent the open file.
     * @return
     */
    void FileOpenL( const TDesC& aName,
                    TUint aMode,
                    TFileOpen anOpen,
                    CFileCB* aFile );
        
    /**
     * From CMountCB Opens a directory on the mount.
     * @since Series 60 3.2
     * @param aName A reference to a descriptor containing the full name of
     the directory that will be opened.
     * @param aDir  Points to a directory control block which will, on success,
     represent the open directory.
     * @return 
     */        
    void DirOpenL(  const TDesC& aName,
                    CDirCB* aDir  );
  
    /**
     * From CMountCB Gets the short name of the file or directory with 
     *               the given full name.
     * @since Series 60 3.2
     * @param aLongName  A reference to a descriptor containing the full name
     *                   of the entry.
     * @param aShortName On return, a reference to a descriptor containing
     *                   the short name of the entry.
     * @return
     */    
    void GetShortNameL( const TDesC& aLongName,
                        TDes& aShortName );
        
    /**
     * From CMountCB Gets the long name of the file or directory associated with
     *               the given short name.
     * @since Series 60 3.2
     * @param aShorName  A reference to a descriptor containing the short name
     *                   of the entry.
     * @param aLongName  On return, a reference to a descriptor containing
     *                   the long name of the entry.
     * @return
     */      
    void GetLongNameL( const TDesC& aShortName,
                       TDes& aLongName );
    
    /**
     * From CMountCB Reads a specified section of the file, regardless of the file's lock state.
     * @since Series 60 3.2
     * @param aName    A reference to a descriptor containing the full name of
     *                 the file to be read from
     * @param aPos     The byte position to start reading from.
     * @param aTrg     A pointer to the buffer into which data is to be read.
     * @param aLength  The length of data to be read, in bytes.
     * @param aMessage Client message.
     * @return 
     */
    void ReadSectionL( const TDesC& aName,
                       TInt aPos,TAny* aTrg,
                       TInt aLength,
                       const RMessagePtr2& aMessage );
                          
    /**
     * From CMountCB Reads the specified length of data from the specified position on
     *               the volume directly into the client thread.
     * @since Series 60 3.2
     * @param aPos     Start position in the volume for the read operation,
     *                 in bytes.
     * @param aLength  The number of bytes to be read.
     * @param aTrg     A pointer to the buffer into which data is to be read.
     * @param anOffset The offset at which to start adding data to the read buffer.
     * @param aMessage Client message.
     * @return 
     */    
    void RawReadL( TInt64 aPos,
                   TInt aLength,
                   const TAny* aTrg,
                   TInt anOffset,
                   const RMessagePtr2& aMessage ) const;
                      
    /**
     * From CMountCB  Writes a specified length of data from the client thread to the volume
     *                at the specified position.
     * @since Series 60 3.2
     * @param aPos     Start position in the volume for the write operation,
     *                 in bytes.
     * @param aLength  The number of bytes to be written.
     * @param aSrc     Pointer to the buffer from which data will be written.
     * @param anOffset The offset in the buffer at which to start writing data.
     * @param aMessage Client message.
     * @return 
     */                    
    void RawWriteL( TInt64 aPos,
                    TInt aLength,
                    const TAny* aSrc,
                    TInt anOffset,
                    const RMessagePtr2& aMessage );
                       
    /**
    Checks the integrity of the file system on the volume and returns an
    appropriate error value. 
    
    @return KErrNone if the file system is stable; otherwise one of
            the other system wide error codes.
    */
	TInt CheckDisk();
	
private:
    
    /**
     * C++ default constructor.
     */
    CRsfwFsMountCB();

                                                                                         
    /**
     * Gets entry details for a file or directory
     * @since Series 60 3.2
     * @param aName        Name of the file or directory.
     * @param anEntry      On success, contains the entry details.
     * @return
     */
    void RemoteFsEntryL( const TDesC& aName,
                         TEntry& anEntry );
                             
    /**
     * Fetches fid for a file or directory.
     * @since Series 60 3.2
     * @param aPath        The full path of the file or directory.
     * @param aNodeType    Type of the node (i.e. file or directory).
     * @return
     */  
    TFid FetchFidL( const TDesC& aPath, 
                    TUint aNodeType );
                        
                        

  
public:     // Data
    // Server to which this mount is connected.
    // The pointer is stored as volumename to CMountCB, 
    // which takes of the desc. ownership and deletes it.
    HBufC* iServerName;
            
    // File Server session used to access the local cache.
    RFs iFsSession;
        
private:    // Data
    // Pointer to filesystem object.    
    CRsfwFileSystem* iRemoteFs;

    // Session to Remote File Engine
    RRsfwSession* iSession;
             
    // Root Fid, can be different for different mounts
    TFid iRootFid; 
    };

#endif // CRSFWFSMOUNTCB_H

// End of File

