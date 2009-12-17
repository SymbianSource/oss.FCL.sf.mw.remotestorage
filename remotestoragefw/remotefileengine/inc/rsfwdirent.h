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


#ifndef CRSFWDIRENT_H
#define CRSFWDIRENT_H

// INCLUDES
#include <e32base.h>

// CONSTANTS

// DATA TYPES

// FORWARD DECLARATIONS
class CRsfwDirEntAttr;

// CLASS DECLARATION
/**
 *  Directory entry container
 *
 *  @lib rsfwcommon.lib
 *  @since Series 60 3.1
 */
class CRsfwDirEnt: public CBase
    {
public:

    /**
     * Two-phased constructor
     * @param aName name of the entry
     * @param aAttr attributes - if this parameter is NULL,
     *   an attribute object with attribute values set to zero will be created.
     *   The entry takes ownership of the attribute object.
     */
    IMPORT_C static CRsfwDirEnt* NewLC(const TDesC& aName, CRsfwDirEntAttr* aAttr);

    /**
     * Two-phased constructor
     * @param aName name of the entry
     * @param aAttr attributes - if this parameter is NULL,
     *   an attribute object with attribute values set to zero will be created.
     *   The entry takes ownership of the attribute object.
     */
    IMPORT_C static CRsfwDirEnt* NewLC(const TDesC8& aName, CRsfwDirEntAttr* aAttr);

    /**
     * Two-phased constructor.
     * @param aName name of the entry
     * @param aAttr attributes - if this parameter is NULL,
     *   an attribute object with attribute values set to zero will be created.
     *   The entry takes ownership of the attribute object.
     */
    IMPORT_C static CRsfwDirEnt* NewL(const TDesC& aName, CRsfwDirEntAttr* aAttr);

    /**
     * Two-phased constructor.
     * @param aName name of the entry
     * @param aAttr attributes - if this parameter is NULL,
     *   an attribute object with attribute values set to zero will be created.
     *   The entry takes ownership of the attribute object.
     */
    IMPORT_C static CRsfwDirEnt* NewL(const TDesC8& aName, CRsfwDirEntAttr* aAttr);

    CRsfwDirEnt();

    IMPORT_C ~CRsfwDirEnt();

    /**
     * Gets the name of the filesystem object
     * @return pointer to the name
     */
    IMPORT_C const HBufC* Name() const;

    /**
     * Gets the name of the filesystem object
     * @param aName name
     */
    IMPORT_C void GetName(TDes& aName) const;

    /**
     * Gets the name of the filesystem object
     * @param aName name
     */
    IMPORT_C void GetName(TDes8& aName) const;

    /**
     * Sets the name of the filesystem object
     * @param aName name
     */
    IMPORT_C void SetNameL(const TDesC& aName);

    /**
     * Sets the name of the filesystem object
     * @param aName name
     */
    IMPORT_C void SetNameL(const TDesC8& aName);

    /**
     * Returns a pointer to the filesystem object's attributes
     * @return pointer to the attributes
     */
    IMPORT_C CRsfwDirEntAttr* Attr() const;

    /**
     * Returns a pointer to the filesystem object's attributes
     * The ownership to the attribute object is transferred to the caller
     * @return pointer to the attributes
     */
    IMPORT_C CRsfwDirEntAttr* ExtractAttr();

    /**
     * Sets the attribute object for the directory entry.
     * The pre-existing attribute object is deleted (if owned by the entry).
     * The directory entry gets ownership of the new attribute object.
     * If the aAttr parameter is NULL,
     * an attribute object with default values is created.
     * @param pointer to the attribute object
     */
    IMPORT_C void SetAttrL(CRsfwDirEntAttr* aAttr);

private:
    void ConstructL(const TDesC& aName, CRsfwDirEntAttr* aAttr);
    void Construct8L(const TDesC8& aName, CRsfwDirEntAttr* aAttr);

private:
    HBufC*              iName;         // name
    CRsfwDirEntAttr*    iAttr;         // attributes
    TBool               iNotOwnAttr;   // we do not own the attributes (extracted)
    };

#endif // CRSFWDIRENT_H

// End of File
