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
* Description:  Mounting utilities
 *
*/


#ifndef RSFWMOUNTUTILS_H
#define RSFWMOUNTUTILS_H

#include <e32std.h>

// CONSTANTS

/// BIO message UID
const TUid KUidBIOMountConfMsg     = {0x1bdeff02};
const TInt KMountMessagePrefixLength = 29; // length of "//vnd.nokia.s60.mount.config\n"

// FORWARD DECLARATIONS
class CRsfwMountEntry;

// CLASS DECLARATION
/** ImportMountEntry is used from smart messaging bio control.
ExportMountEntryL is used from general settings plugin.
Both functions call GetKeyWord(), so only common thing
are actually the keywords used 
Note that because this is only used for smart messaging,
currently exports and imports only mount name and address
*/


class RsfwMountUtils
    {
public: // New functions
     /**
     * Imports a mount configuration from a descriptor
     *
     * @param aMsg descriptor to be imported
     *		memory is reserved by this function, but ownership transferred to the caller
     * @param aEntry returned mount entry
     * @return nothing
     */
    IMPORT_C static void ImportMountEntryL(const TDesC& aMsg,
                                           CRsfwMountEntry** aEntry);
     /**
     * Imports a mount configuration from a stream
     *
     * @param aReadStream stream data to be imported
     *  function assumes that the stream contains 8-bit data
     * @param aSize data size
     * @param aEntry returned mount entry
      *		memory is reserved by this function, but ownership transferred to the caller
     * @return nothing
     */
    IMPORT_C static void ImportMountEntryFromStreamL(RReadStream& aReadStream,
                                                    TInt aSize,
                                                   CRsfwMountEntry** aEntry);
     /**
     * Exports a mount configuration to a descriptor.
     *
     * @param aEntry mount entry to be exported
     * @param aSendCredentials controls whether username/passwd is exported
     * @param aBuf returned descriptor     
     * @return nothing
     */
    IMPORT_C static void ExportMountEntryL(const CRsfwMountEntry& aEntry,
                                           TBool aSendCredentials,
                                           TDes& aBuf);
     /**
     * Returns ETrue if the friendly name for a remote drive is valid
     * Used from MountEntry and the General Settings plugin
     * Currently just calls RFs::IsDriveNameValid() as 
     * the friendly name is stored to RFs::DriveName()
     * @since S60 3.2
     * @param aFriendlyName remote drive friendly name
     */
    IMPORT_C static TBool IsFriendlyNameValid(const TDesC& aFriendlyName);       
    
     /**
     * Returns ETrue if the address (URL) for a remote drive is valid
     * Used from MountEntry and the General Settings plugin
     * @since S60 3.2
     * @param aFriendlyName remote drive friendly name
     */
    IMPORT_C static TBool IsDriveAddressValid(const TDesC8& aDriveAddress);                                
   
    
private:
    static void ParseLineL(const TDesC& aLine, CRsfwMountEntry& aEntry);
    static const TDesC& GetKeyWord(TInt aIndex);
    };

#endif // RSFWMOUNTUTILS_H

// End of File
