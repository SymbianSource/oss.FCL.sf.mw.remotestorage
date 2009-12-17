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
* Description:  class for restoring dormant mounts asynchronously when server starts
*
*/


#ifndef C_RSFWDORMANTMOUNTLOADER_H
#define C_RSFWDORMANTMOUNTLOADER_H

// INCLUDES
#include <e32base.h>
#include "rsfwvolumetable.h"


// how many microseconds after server startup
// we start to restore the dormant mounts.
#define KDormantLoaderDelay 1000000 // 1s

// CLASS DECLARATION

/**
*  Crsfwdormantmountloader class
*
*  This class is for doing stuff asynchronously when loading dormant 
*  mounts at server startup. When server is started, dormant mounts
*  are loaded. However, if there is something that takes more time
*  it can be done synchronously while client is waiting for server to start.
*  
*  Currently only such operation is saving "dirty" files to the local cache
*/
class CRsfwDormantMountLoader : public CActive                                       
    {
 public: // Constructors and destructor

        /**
        * Symbian OS two-phased constructor
        * @return Pointer to this component.
        */
        IMPORT_C static CRsfwDormantMountLoader* NewL(CRsfwVolumeTable *aTheTable);

        /**
        * C++ default destructor.
        */
        virtual ~CRsfwDormantMountLoader();

 private:

        /**
        * C++ default constructor.
        */
        CRsfwDormantMountLoader();

        /**
        * Symbian OS default constructor.
        */
        void ConstructL(CRsfwVolumeTable *aTheTable);


 private: // New functions

        void ResolveDirtyFilesL();
        
   
private:    // Functions from base classes

        /**
        * Handles an active object’s request completion event.
        */
        void RunL();  

 		/**
    	*Implements cancellation of an outstanding request.
		*/
		void DoCancel();

 		/**
    	* Called in case RunL() leaves
		*/
        TInt RunError(TInt aError);
        
private:    // Data
        RTimer  iTimer;
        CRsfwVolumeTable* iVolumeTable;
	  
    };

#endif  // REMOTEWAITNOTEMANAGER_H

// End of File