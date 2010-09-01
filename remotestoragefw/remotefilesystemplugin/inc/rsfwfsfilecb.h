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


#ifndef CRSFWFSFILECB_H
#define CRSFWFSFILECB_H

//  INCLUDES
#include <f32fsys.h>
#include "rsfwinterface.h"

/**
 *  Classes that a plug-in file system must implement. A plug-in 
 *  filesystem must implement CFileSystem, which is a factory class for 
 *  a file system. That class must create objects derived from CMountCB, 
 *  CFileCB, CDirCB and CFormatCB. These are defined in f32fsys.h
 *
 *  @lib eremotefs.fsy
 *  @since Series 60 3.2
 */
class CRsfwFsFileCB : public CFileCB
    {
public:  // Constructors and destructor
    
    /**
     * Constructor.
     */
    CRsfwFsFileCB();
        
    /**
     * Destructor.
     */
    ~CRsfwFsFileCB();
        
public:  // New functions
    
    /**
     * Prepares the class to read file from a local cache file
     * @since Series 60 3.2
     * @param aPath path of the cache file
     * @return 
     */
    void SetContainerFileL( const TDesC& aPath);
        
public:  // Functions from base class
    
    /**
     * From CFileCB Renames the file with the full file name provided.
     * @since Series 60 3.2
     * @param aNewName The new full name of the file.
     * @return 
     */
    void RenameL( const TDesC& aNewName );

    /**
     * From CFileCB Reads from an open file
     * @since Series 60 3.2
     * @param aPos     Represents a position relative to the start of the file
     *                 where ReadL() starts to read.
     * @param aLength  On entry, specifies the number of bytes to be read
     *                 from the file. On return, contains the number of bytes
     *                 read, but this is not valid if the function leaves.
     * @param aDes     Pointer to a descriptor into which the data is written.
     * @param aMessage The client request. 
     * @return 
     */
    void ReadL( TInt aPos,
                TInt& aLength,
                const TAny* aDes, 
                const RMessagePtr2& aMessage );
                    
    /**
     * From CFileCB Writes to an open file
     * @since Series 60 3.2
     * @param aPos     Represents a position relative to the start of the file
     *                 where WriteL() starts to write.
     * @param aLength  Specifies the number of bytes to be written to the file.
     *                 On return, the number of bytes written, but this is not
     *                 valid if the function leaves.
     * @param aDes     Pointer to a descriptor containing the data to be written
     *                 to the file.
     * @param aMessage The client request.
     * @return 
     */                  
    void WriteL( TInt aPos,
                 TInt& aLength,
                 const TAny* aDes,
                 const RMessagePtr2& aMessage );
                     
    /**
     * From CFileCB Extends or truncates the file by re-setting the file size.
     * @since Series 60 3.2
     * @param aSize The new file size in number of bytes.
     * @return 
     */
    void SetSizeL( TInt aSize );
        
    /**
     * From CFileCB Sets the attribute mask and the modified time of the file.
     * @since Series 60 3.2
     * @param aTime The new modified time, if the modified flag is set in aMask.
     * @param aMask Bit mask containing bits set (to 1) that are to be set (to 1)
     *              in iAtt.
     * @param aVal  Bitmask containing bits set (to 1) that are to be unset (to 0)
     *              in iAtt.
     * @return
     */    
    void SetEntryL(const TTime& aTime,TUint aMask,TUint aVal);
        
        
    /**
     * From CFileCB Flushes, to disk, the cached information necessary for
     * the integrity of recently written data, such as the file size.
     * @since Series 60 3.2
     * @return
     */      
    void FlushDataL();
        
    /**
     * From CFileCB Flushes, to disk, all cached file data (e.g. attributes, 
     * modification time, file size). 
     * @since Series 60 3.2
     * @return
     */      
    void FlushAllL();
    
  
public:  // Data
    // the fid of this file   
    TFid iThisFid; 
    
    // the fid of the parent, needed for rename
    TFid iParentFid;
        
    // If flush returned an error, we do not attempt to write the file to server in close()
    // either, unless new data is written to the cache file. We assume that the application
    // has handled the flush error, also this is required for example to make File Manager UI
    // to work correctly (if user presses cancel when flushing), writing should be cancelled.
    TBool iLastFlushFailed;

      
private:   // Data
    // open file handle on the cache file
    RFile iContFile;
    
    // the path of the local cache
    TBuf<KMaxPath> iCachePath;
                 
    // cached size bookkeeping
    TInt iCachedSize;

    // indicates whether a file was fetched to the local cache before writing
    TBool iFetchedBeforeWriting;
    
    // some varibles needed when flushing a big file
    // in continuous parts (currently used in File
    // Manager copy).
    
    // how much already has been flushed
    TInt iFlushedSize;

    // how much has been written to since the file was opened
    TInt iWrittenSize;
    
    // the total size of the file, reported by client
    // via RFile::SetSize()
    TInt iReportedSize;
    
    // does the server support writing the file partially
    // assumed to be true unless we get KErrNotSupported
    TBool iPartialWriteSupported;
    };  

#endif // CRSFWFSFILECB_H


