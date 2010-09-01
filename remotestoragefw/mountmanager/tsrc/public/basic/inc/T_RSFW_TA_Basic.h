/*
* Copyright (c) 2005-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Header file for EUnit test app for MountMan API
*
*/


#ifndef __T_MOUNTMAN_H__
#define __T_MOUNTMAN_H__

//  EXTERNAL INCLUDES
#include <CEUnitTestSuiteClass.h>
#include <rsfwmountman.h>

//  INTERNAL INCLUDES


//  FORWARD DECLARATIONS
class CDesC16Array;

#include <e32def.h>
#ifndef NONSHARABLE_CLASS
    #define NONSHARABLE_CLASS(x) class x
#endif

//  CLASS DEFINITION
/**
 *
 * EUnitWizard generated test class. 
 *
 */
NONSHARABLE_CLASS( T_MountMan )
     : public CEUnitTestSuiteClass
    {
    public:     // Constructors and destructors

        /**
         * Two phase construction
         */
        static T_MountMan* NewL();
        static T_MountMan* NewLC();
        /**
         * Destructor
         */
        ~T_MountMan();

    private:    // Constructors and destructors

        T_MountMan();
        void ConstructL();

    public:     // From observer interface

        

    private:    // New methods

         void SetupL();

         void Teardown();         
         
         // CRsfwMountEntry API
         
         void TestSetMountEntryL();
         
         void TestSetItemL();
         
         void TestSetItem2L();
         
         void TestClearL();
         
         void TestCloneL();
         
         // CRsfwMountMan API
                                 
         void TestAddMountEntryL();
         
         void TestAddMountEntry2L();

         void TestAddMountEntry3L();
         
         void TestEditMountEntryL();
         
         void TestEditMountEntry2L();
         
         void TestEditMountEntry3L();
         
         void TestDeleteMountEntryL();
         
         void TestDeleteMountEntry2L();
         
         void TestDeleteMountEntry3L();
         
         void TestMountEntryL();
         
         void TestMountEntry2L();
         
         void TestGetAllDrivesL();
         
         void TestGetRemoteMountListL();
         
         void TestGetMountNamesL();
         
         void TestGetMountInfoL();
         
         void TestIsAppOnBlackListL();
         
         void TestSetMountConnectionStateL();
         
         void TestRefreshDirectoryL();
         
         void TestCancelRemoteTransferL();
         
         void TestSetMountConnectionStateBlindL();

         // TRsfwMountConfig API

         void TestExternalizeAndInternalizeL();
        
         // utility functions
         
         TInt AddMountEntryL(TInt aIndex,
                             const TDesC& aName,
                             TChar aDriveLetter,
                             const TDesC& aUri,
                             const TDesC& aUserName,
                             const TDesC& aPassword,
                             const TDesC& aIap);

         TInt EditMountEntryL(TInt aIndex,
                             const TDesC& aName,
                             TChar aDriveLetter,
                             const TDesC& aUri,
                             const TDesC& aUserName,
                             const TDesC& aPassword,
                             const TDesC& aIap);        
         
         void ClearAllMountsL(); 
         
         TChar DriveToChar(TInt aDrive);
         
         TInt CompareArrays(CDesC16Array* aArray1, CDesC16Array* aArray2);

         TInt CheckEntryL(const CRsfwMountEntry* aEntry,
                          TInt aIndex,
                          const TDesC& aName,
                          TChar aDriveLetter,
                          const TDesC& aUri,
                          const TDesC& aUserName,
                          const TDesC& aPassword,
                          const TDesC& aIap);

    private:    // Data
        CRsfwMountMan* iMountMan;
        RFs iFs;
        EUNIT_DECLARE_TEST_TABLE; 

    };	    

#endif      //  __T_MOUNTMAN_H__

// End of file
