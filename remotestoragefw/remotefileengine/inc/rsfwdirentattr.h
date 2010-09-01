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
* Description:  Directory entry container
 *
*/


#ifndef CRSFWDIRENTATTR_H
#define CRSFWDIRENTATTR_H

// INCLUDES
#include <e32base.h>

// CONSTANTS

// DATA TYPES

// FORWARD DECLARATIONS

// CLASS DECLARATION
/**
 *  Filesystem object attribute container
 *  The attribute setting operations on this class
 *  do not affect any external objects -
 *  only the internal state of the object will be changed.
 *  Similarily, attribute getting operations only reflect the
 *  state of the object.
 *
 *  @lib rsfwcommon.lib
 *  @since Series 60 3.1
 */
class CRsfwDirEntAttr: public CBase
    {
public:
    enum TDirEntAttrString
        {
        EDirEntAttrStringMimeType = 0,
        EDirEntAttrStringETag,
        EDirEntAttrStringReserved,
        EDirEntAttrStringCount
        };

public:
    /**
     * Two-phased constructor
     */
    IMPORT_C static CRsfwDirEntAttr* NewLC();

    /**
     * Two-phased constructor.
     * The attribute values are zeroed
     */
    IMPORT_C static CRsfwDirEntAttr* NewL();

    CRsfwDirEntAttr();

    IMPORT_C ~CRsfwDirEntAttr();

    /**
     * Gets file or directory attribute bits
     * (for SymbianOS standard bit definitions, see f32file.h)
     * @return attribute bits
     */
    IMPORT_C TUint Att() const;

    /**
     * Sets file or directory attribute bits
     * (for SymbianOS standard bit definitions, see f32file.h)
     * @param aAtt attribute bits
     */
    IMPORT_C void SetAtt(TUint aAtt);

    /**
     * Sets the given file or directory attribute bits to 1
     * (for SymbianOS standard bit definitions, see f32file.h)
     * @param aFlags attribute bits
     */
    IMPORT_C void SetAttFlags(TUint aFlags);

    /**
     * Resets the given file or directory attribute bits to 0
     * (for SymbianOS standard bit definitions, see f32file.h)
     * @param aFlags attribute bits
     *   (those bits are cleared that are set in aFlags)
     */
    IMPORT_C void ResetAttFlags(TUint aFlags);

    /**
     * Returns the size of the filesystem object
     * @return size in bytes
     */
    IMPORT_C TInt Size() const;

    /**
     * Sets the size of the filesystem object
     * @param aSize size in bytes
     */
    IMPORT_C void SetSize(TInt aSize);

    /**
     * Returns the last modified time of the filesystem object
     * @return last modified time
     */
    IMPORT_C TTime Modified() const;

    /**
     * Sets the last modified time of the filesystem object
     * @param aModified last modified time
     */
    IMPORT_C void SetModified(const TTime& aModified);

    /**
     * Returns the UID3 of the filesystem object
     * @return UID value
     */
    IMPORT_C const TUid& Uid();

    /**
     * Sets the UID3 time of the filesystem object
     * @param aUid UID value
     */
    IMPORT_C void SetUid(TUid aUid);

    /**
     * Returns a string value at the given index
     * @param aIndex index
     * @return string value
     */
    IMPORT_C const TDesC8* StringValue(TInt aIndex) const;

    /**
     * Sets a string value at the given index
     * @param aIndex index
     * @param string value
     */
    IMPORT_C void SetStringValueL(TInt aIndex, const TDesC8& aString);

    /**
     * Gets the MIME type
     * @return MIME type string
     */
    IMPORT_C const TDesC8* MimeType() const;

    /**
     * Sets the MIME type
     * @param aMimeType MIME type string
     */
    IMPORT_C void SetMimeTypeL(const TDesC8& aMimeType);

    /**
     * Gets the ETag
     * @return ETag string
     */
    IMPORT_C const TDesC8* ETag() const;

    /**
     * Sets the ETag type
     * @param aETag ETag type string
     */
    IMPORT_C void SetETagL(const TDesC8& aETag);

private:
    void ConstructL();

private:
    TUint iAtt;                                    // attribute bits
    TInt iSize;                                    // file size in bytes
    TTime iModified;                               // last modified         
    TUid iUid;                                     // Symbian data-type (UID3)
    HBufC8* iStringValues[EDirEntAttrStringCount]; // string values
    };


#endif // CRSFWDIRENTATTR_H

// End of File
