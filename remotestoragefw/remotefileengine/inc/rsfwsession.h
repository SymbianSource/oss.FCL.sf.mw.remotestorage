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
* Description:  Defines the standard Symbian IPC for using Access API
 *
*/


#ifndef RRSFWSESSION_H
#define RRSFWSESSION_H

//  INCLUDES
#include <e32base.h>
//#include <f32file.h>
//#include <s32strm.h>

//  INTERNAL INCLUDES
//#include "rsfwcontrol.h"
#include "rsfwcommon.h"
//#include "rsfwinterface.h"
class TFid;
class TEntry;
class TDirEntAttr;

enum TRequestCodes
    {
    ERequestPending
    };


// CLASS DECLARATION
/**
 *  This class provides the client-side interface to the server session.
 *
 *  @lib rsfwsession.dll
 *  @since Series 60 3.1
 */
class RRsfwSession : public RSessionBase
    {
public: // Constructors and destructor
    /**
     * Constructor.
     */
    IMPORT_C RRsfwSession();
    
public: // New functions
    /**
     * Connect to the server and create a session.
     * @since Series 60 3.1
     * @param aServerName Server name.
     * @return Standard error code.
     */    
    IMPORT_C TInt Connect();
    
     /**
     * Close the session
     * @since Series 60 3.1
     */    
    IMPORT_C void Close();    
    
    /**
     * Get the server version number.
     * @since Series 60 3.1
     * @return The version number of the server.
     */            
    IMPORT_C TVersion Version() const;
        
     /**
     * Issue request to rename or replace a file or a directory
     * @since Series 60 3.1
     * @param aSourceFid   Fid of the source file's parent directory
     * @param aSourceName  The name of the object to be renamed
     *                   (for dirs must have a trailing backslash)
     * @param aDestFid     Fid of the destination directory
     * @param aDestName    The name of the target object
     *                  (for dirs must have a trailing backslash)
     * @return
     */
    IMPORT_C TInt MoveFids( TFid aSourceFid, 
                             const TDesC& aSourceName, 
                             TFid aDestFid, 
                             const TDesC& aDestName,
                             TBool aOverWrite );
    
   /**
     * Issue request to set entry details for a specified file or directory.
     * @since Series60 3.1
     * @param aFid   Fid of the target file or directory
     * @param aTime  A reference to the time object holding the new universal
     *               modified time for aName.
     * @param aSetAttMask   Attribute mask for setting the entry's attributes.
     * @param aClearAttMask Attribute mask for clearing the entry's attributes.
     * @return
     */                              
    IMPORT_C TInt SetEntry( TFid aFid,
                             const TTime& aTime,
                             TUint aSetAttMask,
                             TUint aClearAttMask);
                             
    /**
     * Issue request to flush an entry from the cache.
     * @since Series 60 3.1
     * @param aFid The fid of the file or directory to be flushed.
     * @return
     */  
    IMPORT_C TInt FlushCache( TFid& aFid );                  
                             
    /**
     * Issue request to raise the cache priority of an already
     * cached file.
     * @since Series 60 3.1
     * @param aFid The fid of the file.
     * @return
     */      
    IMPORT_C TInt SetHighCachePriority( TFid& aFid );   
    
    /**
     * Issue request to fetch file or directory attributes.
     * @since Series 60 3.1
     * @param aFileFid     Fid of the file.
     * @param aAttributes  On success, contains the file attributes 
     *                     (TEntry::iName is not filled)
     * @return 
     */                                   
    IMPORT_C TInt GetAttributes( TFid aFileFid, 
                                 TEntry& aAttributes );    
                         
     /**
     * Issue request to open this fid and return the path of the container file
     * Note that this is "OPEN" operation, e.g. if the mode is relevant a lock
     * will be acquired and will be refreshed until the file is closed.
     * @since Series 60 3.1
     * @param aFid        Fid of the file or directory.
     * @param aContainer  Pointer to descriptor which will, on success, contain 
     *                    the full path of the container file.
     * @param aAttr       aAttr.iAtt: Open mode, controls acquiring locks etc. 
     *                    on return, contains size etc.
     * @param aTrueOpen   Whether File Server opens the file, or ReadSection was called in
     *                    which case we also mimic the sequence of opening a file  
     *                    (file is not candidate for removing from cache if it has been
     *                     opened by the File Server).         
     * @return 
     */             
    IMPORT_C TInt OpenByPath( TFid aFid, 
                              TDes& aContainerPath, 
                              TDirEntAttr* aAttr, 
                              TBool aTrueOpen);   
     
    /**
     * Issue request to initialize the remote mount.
     * @since Series 60 3.1
     * @param   Upon successfull return, will contain the Fid of the root directory
     * @return 
     */
    IMPORT_C TInt RfeInit(TFid& aRootFid);         
    
    
    /**
     * Issue request to make a directory.
     * @since Series 60 3.1
     * @param aParentFid Fid of the parent directory.
     * @param aDirName   The name of the new directory.
     * @return
     */ 
    IMPORT_C TInt MakeDirectory( TFid aParentFid, 
                                 const TDesC& aDirName );
                                    
    /**
     * Issue request to remove a directory.
     * @since Series 60 3.1
     * @param aParentFid Fid of the parent directory.
     * @param aDirName   The name of the directory to be removed.
     * @return
     */
    IMPORT_C TInt RemoveDirectory( TFid aParentFid, 
                                   const TDesC& aDirName );
                              

     /**
     * Issue request to create and open a file.
     * Note that this is "OPEN" operation, e.g. if the mode is relevant a lock
     * will be acquired and will be refreshed until the file is closed.
     * @since Series 60 3.1
     * @param aParentFid Fid of the parent directory.
     * @param aFileName  The name of the new file.
     * @param aMode      The mode in which the file will be opened.
     * @param aExcl      Boolean indicating whether it is ok to overwrite an existing file.
     *                   (ETrue = exclusive = no overwriting)
     * @param aNewFid       Upon successful return, contains the fid of the new file
     * @return 
     */        
    IMPORT_C TInt CreateFile( TFid aParentFid, 
                      const TDesC& aFileName, 
                      TUint aMode, 
                      TUint aExcl,
                      TFid& aNewFid);

    /**
     * Issue request to remove a file.
     * @since Series 60 3.1
     * @param aParentFid Fid of the parent directory.
     * @param aFileName   The name of the file to be removed.
     * @return
     */
    IMPORT_C TInt RemoveFile( TFid aParentFid, 
                      const TDesC& aFileName );


    /**
     * finds the fid of a file system object
     * @since Series 60 3.1
     * @param aParentFid fid of the parent directory
     * @param aName name of the child to be looked up
     * @param aNodeType is the type of the child, 
     *                  KNodeTypeUnknown,KNodeTypeFile or KNodeTypeDir
     *      (also a trailing backslash in the name indicates KNodeTypeDir)
     * @param aFid upon successful return contains the fid 
     *        of the object to be looked up
     * @return
     */        
    IMPORT_C TInt Lookup( TFid aParentFid, 
                  const TDesC& aName, 
                  TUint aNodeType,
                  TFid& aFid );
                  
     /**
     * Tells Remote File Engine that a file has been closed
     * @since Series 60 3.1
     * @param aFid Fid of the file that was closed
     * @param aFlags whether the file has been changed
     * @return 
     */
    IMPORT_C void CloseFile( const TFid aFid, 
                 TUint aFlags); 
                 
            
     /**
     * Tells Remote File Engine to write a file that has been 
     * modified back to the server. However, File Server does
     * not close the file.
     * @since Series 60 3.1
     * @param aFid Fid of the file to be flushed
     * @param aFirstByte the first byte to be flushed
     * @param aLastByte the first byte to be flushed
     * @param aTotalSize the full size of the file
     * @return 
     */
    IMPORT_C TInt Flush( const TFid aFid,
                         TInt aFirstByte,
                         TInt aDataLength,
                         TInt aTotalSize );
        
    /**
     * Issue requet to fetch and cache a file or a directory
     * @since Series 60 3.1
     * @param aFileFid the fid of the file to be cached
     * @param aFirstByte the first byte to be cached (0 for dirs)
     * @param aLastByte the last byte to be cached (0 for dirs)
     * @param aCachedBytes upon succesful return,
     *          the number of bytes cached after this fetch
     * @return
     */
    IMPORT_C TInt Fetch( TFid aFileFid, 
                 TInt aFirstByte, 
                 TInt aLastByte,
                 TInt& aCachedBytes );                      


    /**
     * Issue request to fetch data without permanent caching.
     * @since Series 60 3.1
     * @param aFileFid the fid of the file to be fetched
     * @param aFirstByte the first byte to be fetched
     * @param aLastByte the last byte to be fetched
     * @param aTempFileName the path of the temp file 
     *        where the data will be stored
     * @param aUseTempPath FALSE if the caching mode was such
     *                     that the data was anyway stored into
     *                     the normal cache file.
     *        
     * @return 
     */    
    IMPORT_C TInt FetchData( TFid aFileFid, 
                      TInt aFirstByte, 
                      TInt aLastByte,
                      TDes& aTempFileName,
                      TBool& aUseTempPath);

    /**
     * Queries from Remote File Engine whether there is enough local cache 
     * space to write aBytes of data
     * @since Series 60 3,1
     * @param aFid   fid of the file whose data will be written
     * @param aBytes the number of bytes to be written
     * @return a boolean value indicating whether the data can be written
     */
    IMPORT_C TInt OkToWrite( TFid aFid, 
                            TUint aBytes,
                            TBool& aOkToWrite );

private:
    static TInt StartServer( const TDesC& aServerName );
    static TInt CreateServerProcess( const TDesC& aServerName );
    TInt SendRequest(TInt aOpCode, TInt aDrive, TIpcArgs aArgs);
    };

#endif // RRSFWSESSION_H

// End of File
