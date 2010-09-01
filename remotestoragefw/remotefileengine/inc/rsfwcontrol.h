/*
* Copyright (c) 2005-2007 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Remote File Engine Control API
*
*/



#ifndef RRSFWCONTROL_H
#define RRSFWCONTROL_H

//  INCLUDES
#include <f32file.h>
#include <e32def.h>
#include <s32strm.h>
#include <rsfwmountman.h>

// CONSTANTS

// Number of message slots to reserve for this client server session.
const TUint KDefaultMessageSlots = 4;

// Mounting options
const TUint KMountFlagNull             = 0x00;  // nothing
const TUint KMountFlagInteractive      = 0x01;  // do user prompting
const TUint KMountFlagAskPassword      = 0x02;  // ask the user for password
const TUint KMountFlagOffLine          = 0x04;  // mount in disconnected mode
const TUint KMountFlagAsync            = 0x10;  // mount asynchronously
const TUint KMountFlagAllocDriveLetter = 0x20;  // find a free drive letter
const TUint KMountFlagMountAtRfeOnly   = 0x40;  // mount only at the RFE

// Mount states
const TUint KMountStateDormant = 0x01; // only persistent metada on disk
const TUint KMountStateMounted = 0x02;

// Data types

enum TMountControl
    {
    EMountControlPermanence
    };
    
enum TRfeError
    {
    EServerNotAvailable = 3
    };


// CLASS DECLARATIONS
    
class RRsfwControl : public RSessionBase
    {
public: // Constructors and destructor
    /**
     * Constructor.
     */
    IMPORT_C RRsfwControl();
    
public: // New functions
    /**
     * Connect to the server and create a session.
     * @since Series 60 3.1
     * @param aServerName Server name.
     * @return Standard error code.
     */    
    IMPORT_C TInt Connect();
        
    /**
     * Get the server version number.
     * @since Series 60 3.1
     * @return The version number of the server.
     */            
    IMPORT_C TVersion Version();
        
    /**
     * Mount a remote drive - synchronous version
     * @since Series 60 3.1
     * @param aDriveLetter - letter of the  drive to be mounted
     * @return error code.
     */    
    IMPORT_C TInt Mount(TInt aDriveLetter);
    
    
     /**
     * Mount a remote drive - synchronous version
     * @since Series 60 3.1
     * @param aMountConfig mount configuration information
     *   The following TRsfwMountConfig::iFlags are used in this function:
     *     KMountFlagOffLine         mount in disconnected mode
     * @param aStatus returned status code.
     * @return error code.
     */    
    IMPORT_C TInt Mount(const TRsfwMountConfig& aMountConfig);
    
    /**
     * Mount a remote drive - asynchronous version
     * @since Series 60 3.1
     * @param aMountConfig mount configuration information
     *   The following TRsfwMountConfig::iFlags are used in this function:
     *     KMountFlagOffLine         mount in disconnected mode
     * @param aStatus returned status code.
     * @return error code.
     */    
    IMPORT_C void Mount(const TRsfwMountConfig& aMountConfig,
                        TRequestStatus& aStatus);
    
    /**
     * Dismount a remote drive by referring to the id of the volume in RFE
     * @since Series 60 3.1
     * @param aVolumeId volume identifier (can be found in TRsfwMountInfo)
     * @return error code.
     */    
    IMPORT_C TInt DismountByVolumeId(TInt aVolumeId);

    /**
     * Dismount a remote drive by referring to the drive letter
     * @since Series 60 3.1
     * @param aDriveLetter drive letter (can be found in TRsfwMountInfo)
     * @return error code.
     */    
    IMPORT_C TInt DismountByDriveLetter(TChar aDriveLetter);

    /**
     * Get information about the specified drive
     * @since Series 60 3.1
     * @param aDriveLetter drive letter.
     * @param aMountInfo to be filled.
     * @return error code.
     */    
    IMPORT_C TInt GetMountInfo(const TChar& aDriveLetter,
                               TRsfwMountInfo& aMountInfo);

    /**
     * Set mount connection state
     * @since Series 60 3.1
     * @param aDriveLetter drive letter.
     * @param aState connection state:
     *    KMountStronglyConnected or KMountNotConnected.
     * @return error code.
     */    
    IMPORT_C TInt SetMountConnectionState(const TChar& aDriveLetter,
                                          TUint aState);       


     /**
     * Refresh a remote directory
     *
     * Ensures that contents of a remote directory are up to date.
     * Synchronous variant deletes the currently cached version.
     * Note that this function intentionally does not return directory
     * contents. All data should be read through the File Server instead.
     *
     * @param aPath the remote path
     * @return KErrArgument Path refers to a file
     *         KErrNotFound path is not found from cache
     */
    IMPORT_C TInt RefreshDirectory(const TDesC& aPath);
 
     /**
     * Cancels an active remote file upload or download
     * 
     * @param aFile file name
     * @return The number of remote operations cancelled
     *         or one of the system wide error codes.
     */    
    IMPORT_C TInt CancelRemoteTransfer(const TDesC& aFile);
    

     /**
     * Mount a remote drive - "blind request" version
     * @since Series 60 3.1
     * @param aDriveLetter - letter of the  drive to be mounted
     * @return error code.
     */    
    IMPORT_C TInt MountBlind(TInt aDriveLetter);



private:
    static TInt StartServer(const TDesC& aServerName);
    static TInt CreateServerProcess(const TDesC& aServerName);
    TInt SendRequest(TInt aOpCode,
                     TIpcArgs aArgs);
    void SendRequest(TInt aOpCode,
                     TIpcArgs aArgs,
                     TRequestStatus& aStatus);

private:
    TIpcArgs iArgs;
    TPckgBuf<TRsfwMountConfig> iPckgBufMountConfig; // for asynchronous ipc
    };       


#endif  // RRSFWCONTROL_H
 
// End of File 
