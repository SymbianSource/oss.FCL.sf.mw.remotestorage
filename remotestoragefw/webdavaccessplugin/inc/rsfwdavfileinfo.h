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
* Description:  Cache for file metadata
 *
*/


#ifndef CRSFWDAVFILEINFO_H
#define CRSFWDAVFILEINFO_H

// INCLUDES
#include <e32base.h>

// CLASS DECLARATION

class TRsfwDavFileInfoFlags
    {
public:
    enum TDavFileInfoFlag
        {
        EUnlockPending   = 0x01
        };
    };

// CLASS DECLARATION

/**
 *  WebDAV file information object
 *  Used by DAV access module to handle file locking
 *  This information is thus internal to WebDAV access module
 *  and separated from metadata that is passed to Remote File Engine
 *
 *  @lib davaccess.lib
 *  @since Series 60 3.1
 */

class CRsfwDavFileInfo : public CBase
    {
public: // Constructors and destructor
    /**
     * Two-phased constructor.
     */
    static CRsfwDavFileInfo* NewL();
   
    /**
     * Destructor.
     */
    ~CRsfwDavFileInfo();

public: // New functions

    /**
     * Get name
     * @return name
     */
    HBufC* Name();

    /**
     * Set name
     * @param aName name
     */
    void SetNameL(const TDesC& aName);
    
    /**
     * Get lock token
     * @return lock token
     */
    HBufC8* LockToken();

    /**
     * Set lock token
     * @param aLockToken lock token
     */
    void SetLockTokenL(const TDesC8& aLockToken);

    /**
     * Clear lock token
     */
    void ResetLockToken();

     /**
     * Get lock timeout
     * @return lock timeout
     */
    TUint Timeout();

    /**
     * Set timeout
     * @param aTimeout timeout
     */
    void SetTimeout(TUint aTimeout);

    /**
     * Check if a flag is set
     * @param aFlag flag mask (only supports a single bit)
     * @return ETrue if the flag is set
     */
    TBool IsFlag(TUint aFlag);

    /**
     * Set a flag bit
     * @param aFlag flag bit to be set
     */
    void SetFlag(TUint aFlag);

    /**
     * Clear a flag bit
     * @param flag bit to be cleared
     */
    void ResetFlag(TUint aFlag);

private:
    void SetL(HBufC*& aDst, const TDesC& aSrc);
    void SetL(HBufC8*& aDst, const TDesC8& aSrc);
    
private: // Data
    // These are used when constructing messages, thus 8 bit
    HBufC*  iName;
    HBufC8* iLockToken;
    
    // Active lock info:
    // lock timeout
    TUint iTimeout;
    // lock flags (currently always write lock)
    TUint iFlags;
    };

#endif // CRSFWDAVFILEINFO_H

// End of File
