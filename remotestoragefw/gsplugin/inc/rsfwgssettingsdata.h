/*
* Copyright (c) 2005 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Rsfw GS plugin data holding class for Rsfw setting list
*
*/


#ifndef CRSFWGSSETTINGSDATA_H
#define CRSFWGSSETTINGSDATA_H

// INCLUDE FILES
#include <e32base.h>

#include "rsfwgsplugin.hrh"


// CONSTANTS
const TInt KDefaultInactivityTimeout        = 600; // 5min

/**
*  CRsfwGsSettingsData holds single remote drive configuration
*/
class CRsfwGsSettingsData : public CBase
    {
    public:

        /**
        * Default 1st phase factory method.
        * Creates an instance of CRsfwGsSettingsData
        */
      static CRsfwGsSettingsData* NewL();
      
        /**
        * Default 1st phase factory method.
        * Creates an instance of CRsfwGsSettingsData, leaves it on stack
        */
        static CRsfwGsSettingsData* NewLC();
        
        /**
        * Destructor
        */
      virtual ~CRsfwGsSettingsData();
      
        /**
        * Resets all data to initial values
        */
      void Reset();
      
    private:

        /**
        * 2nd Phase constructor
        */
      void ConstructL();
      
        /**
        * C++ Constructor
        */
      CRsfwGsSettingsData();
      
    public:
        
      // Buffer holding the remote drive friendly name
      TBuf<KMaxFriendlyNameLength>  iSettingName;

      // Access point number
      TInt32                          iAccessPoint;
      
      // Buffer holding the access point number as a descriptor
      TBuf<KMaxAccessPointDesLength>  iAccessPointDes;
      
      // Buffer holding the URL
      TBuf<KMaxURLLength>          iURL;

        // Buffer holding the User ID
      TBuf<KMaxUserIDLength>          iUserID;
      
      // Buffer holding the password
      TBuf<KMaxPasswordLength>        iPassword;
      
      // Buffer holding the access point name
      TBuf<KMaxAccessPointNameLength> iAccessPointName;
      
      // Inactivity timeout
      TBuf<KMaxInactivityTimeoutString> iInActivityTimeout;
      
      // drive letter
      TChar iDriveLetter;
    };

#endif // CRSFWGSSETTINGSDATA_H