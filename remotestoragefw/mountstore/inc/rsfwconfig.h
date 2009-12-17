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
* Description:  Rsfw operational parameter config
 *                (not for mount configurations)
 *
*/


#ifndef CRSFWCONFIG_H
#define CRSFWCONFIG_H

// INCLUDES
#include <e32base.h>

// CONSTANTS
const TUid KCRUidRsfwCtrl         = { 0x101F9775 };
const TInt KMaxRsfwConfItemLength = 128;

namespace RsfwConfigKeys
    {
    const TUint KRsfwDefaultDrive        =  1;
    const TUint KCacheDirectoryPath      =  2;
    const TUint KMaxEntryCount           =  3;
    const TUint KMaxCacheSize            =  4;
    const TUint KFileCacheValidity       =  5;
    const TUint KDirCacheValidity        =  6;
    const TUint KCachingMode             =  7;
    const TUint KRecognizerLimit         =  8;
    const TUint KAudMpegLimit            =  9;
    const TUint KImgJpegLimit            = 10;
    const TUint KRsfwReserved1           = 11;
    const TUint KLockTimeout             = 12;
    const TUint KInactivityTimeout       = 13;
    }

// FORWARD DECLARATIONS
class CRepository;

// CLASS DECLARATION

/**
 *  Configuration API for Rsfw operation
 *
 *  @lib rsfwconfig.lib
 *  @since Series 60 3.1
 */

class CRsfwConfig: public CBase
    {
public:
public: // Constructors and destructor
    /**
     * Two-phased constructor.
     */
    IMPORT_C static CRsfwConfig* NewL(TUid aRepositoryUid);

    /**
     * Destructor.
     */
    virtual ~CRsfwConfig();

public: // New functions
    /**
       Sets numeric value.
       @param aId parameter id
       @return error code
    */
    IMPORT_C TInt Set(TUint aId, TInt& aValue);

    /**
       Sets string value.
       @param aId parameter id
       @return error code
    */
    IMPORT_C TInt Set(TUint aId, TDes& aValue);

    /**
       Gets numeric value.
       @param aId parameter id
       @return error code
    */
    IMPORT_C TInt Get(TUint aId, TInt& aValue);

    /**
       Gets string value.
       @param aId parameter id
       @return error code
    */
    IMPORT_C TInt Get(TUint aId, TDes& aValue);

    /**
       Checks whether a string value is True.
       Gets boolean value: return true iff string starts with '1' | 'Y' | 'T'
       (case-insensitive).
       If the repository value is missing returns False.
       @param aId parameter id
       @return boolean
    */
    IMPORT_C TBool IsTrue(TUint aId);

    /**
       Checks whether a string value is False.
       Gets boolean value: returns true iff string starts with '0' | 'N' | 'F'
       (case-insensitive)
       If the repository or value is missing returns False.
       @param aId parameter id
       @return boolean
    */
    IMPORT_C TBool IsFalse(TUint aId);

private:
    void ConstructL(TUid aRepositoryId);

private: // Data
    CRepository* iRepository;
    };

#endif // CRSFWCONFIG_H

// End of File
