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
* Description:  Source file for EUnit test app for MountMan API
*
*/


// CLASS HEADER
#include "T_RSFW_TA_Basic.h"

// INCLUDES
#include <EUnitMacros.h>
#include <rsfwmountentry.h>
#include <rsfwmountentryitem.h>
#include <f32file.h>
#include <badesca.h>
#include <ecom.h>
#include "rsfwcontrol.h"
#include <s32file.h>

// CONSTRUCTION
T_MountMan* T_MountMan::NewL()
    {
    T_MountMan* self = T_MountMan::NewLC();
    CleanupStack::Pop();

    return self;
    }

T_MountMan* T_MountMan::NewLC()
    {
    T_MountMan* self = new( ELeave ) T_MountMan();
    CleanupStack::PushL( self );

    self->ConstructL();

    return self;
    }

// Destructor (virtual by CBase)
T_MountMan::~T_MountMan()
    {
    iFs.Close();
    REComSession::FinalClose();  
    }

// Default constructor
T_MountMan::T_MountMan()
    {
    }

// Second phase construct
void T_MountMan::ConstructL()
    {
    // The ConstructL from the base class CEUnitTestSuiteClass must be called.
    // It generates the test case table.
    CEUnitTestSuiteClass::ConstructL();
    
    iFs.Connect();    
    }

//  METHODS

// ----------------------------------------------------------------------------
// T_MountMan::SetupL
// ----------------------------------------------------------------------------
//
void T_MountMan::SetupL(  )
    {
	iMountMan = CRsfwMountMan::NewL(0,NULL);
    }

// ----------------------------------------------------------------------------
// T_MountMan::Teardown
// ----------------------------------------------------------------------------
//
void T_MountMan::Teardown(  )
    {
    delete iMountMan;
    iMountMan = NULL;
    }

///////////////////////////////////////////////////////////////////////////////
////// CRsfwMountEntry  ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


// ----------------------------------------------------------------------------
// T_MountMan::TestSetMountEntryL
// Tests that it is not possible to create an entry with too long attributes
// ----------------------------------------------------------------------------
//
void T_MountMan::TestSetMountEntryL( )
    {
    TInt err;
    CRsfwMountEntry* mountEntry;
    mountEntry = CRsfwMountEntry::NewLC();
        
    // too long friendly name
    TBuf<KMaxMountNameLength+1> longName;
    longName.FillZ(longName.MaxLength());
    TRAP(err, mountEntry->SetItemL(EMountEntryItemName, longName));
    EUNIT_ASSERT(err == KErrArgument);

    // too long URI
    TBuf<KMaxMountUriLength+1> longUri;
    longUri.FillZ(longUri.MaxLength());
    TRAP(err, mountEntry->SetItemL(EMountEntryItemUri, longUri));
    EUNIT_ASSERT(err == KErrArgument);

    // too long username
    TBuf<KMaxMountUserNameLength+1> longUserName;
    longUserName.FillZ(longUserName.MaxLength());
    TRAP(err, mountEntry->SetItemL(EMountEntryItemUserName, longUserName));
    EUNIT_ASSERT(err == KErrArgument);

    // too long password
    TBuf<KMaxMountPasswordLength+1> longPassword;
    longPassword.FillZ(longPassword.MaxLength());
    TRAP(err, mountEntry->SetItemL(EMountEntryItemPassword, longPassword));
    EUNIT_ASSERT(err == KErrArgument);
                                      
    // too long IAP
    TBuf<KMaxMountConfItemLength+1> longIap;
    longIap.FillZ(longIap.MaxLength());
    TRAP(err, mountEntry->SetItemL(EMountEntryItemIap, longIap));
    EUNIT_ASSERT(err == KErrArgument);
                                      
    CleanupStack::PopAndDestroy(mountEntry);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestSetItemL
// Tests CRsfwMountEntry::SetItemL(TInt aIndex, const TDesC8& aValue);
// ----------------------------------------------------------------------------
//
void T_MountMan::TestSetItemL( )
    {
    TInt err;
    CRsfwMountEntry* mountEntry;
    mountEntry = CRsfwMountEntry::NewLC();

    // set entry items one by one
    TRAP(err, mountEntry->SetItemL(EMountEntryItemName, _L8("name")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemUri, _L8("http://url")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemUserName, _L8("username")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemPassword, _L8("password")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemIap, _L8("iap")));
    EUNIT_ASSERT(err == KErrNone);

    // check the values
    const HBufC* item = mountEntry->Item(EMountEntryItemName);
    EUNIT_ASSERT((*item).Compare(_L("name")) == KErrNone);
    item = mountEntry->Item(EMountEntryItemUri);
    EUNIT_ASSERT((*item).Compare(_L("http://url")) == KErrNone);
    item = mountEntry->Item(EMountEntryItemUserName);
    EUNIT_ASSERT((*item).Compare(_L("username")) == KErrNone);
    item = mountEntry->Item(EMountEntryItemPassword);
    EUNIT_ASSERT((*item).Compare(_L("password")) == KErrNone);
    item = mountEntry->Item(EMountEntryItemIap);
    EUNIT_ASSERT((*item).Compare(_L("iap")) == KErrNone);
    
    CleanupStack::PopAndDestroy(mountEntry);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestSetItemL
// Tests CRsfwMountEntry::SetItemL(TInt aIndex, const TDesC& aValue);
// ----------------------------------------------------------------------------
//
void T_MountMan::TestSetItem2L( )
    {
    TInt err;
    CRsfwMountEntry* mountEntry;
    mountEntry = CRsfwMountEntry::NewLC();

    // set entry items one by one
    TRAP(err, mountEntry->SetItemL(EMountEntryItemName, _L("name")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemUri, _L("http://url")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemUserName, _L("username")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemPassword, _L("password")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemIap, _L("iap")));
    EUNIT_ASSERT(err == KErrNone);

    // check the values
    const HBufC* item = mountEntry->Item(EMountEntryItemName);
    EUNIT_ASSERT((*item).Compare(_L("name")) == KErrNone);
    item = mountEntry->Item(EMountEntryItemUri);
    EUNIT_ASSERT((*item).Compare(_L("http://url")) == KErrNone);
    item = mountEntry->Item(EMountEntryItemUserName);
    EUNIT_ASSERT((*item).Compare(_L("username")) == KErrNone);
    item = mountEntry->Item(EMountEntryItemPassword);
    EUNIT_ASSERT((*item).Compare(_L("password")) == KErrNone);
    item = mountEntry->Item(EMountEntryItemIap);
    EUNIT_ASSERT((*item).Compare(_L("iap")) == KErrNone);
    
    CleanupStack::PopAndDestroy(mountEntry);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestClearL
// Tests CRsfwMountEntry::Clear
// ----------------------------------------------------------------------------
//
void T_MountMan::TestClearL( )
    {
    TInt err;
    CRsfwMountEntry* mountEntry;
    mountEntry = CRsfwMountEntry::NewL();
    CleanupStack::PushL(mountEntry);

    // set entry items one by one
    TRAP(err, mountEntry->SetItemL(EMountEntryItemName, _L("name")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemUri, _L("http://url")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemUserName, _L("username")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemPassword, _L("password")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemIap, _L("iap")));
    EUNIT_ASSERT(err == KErrNone);

    // clear the values
    mountEntry->Clear();

    // check whether the values are NULL
    const HBufC* item = mountEntry->Item(EMountEntryItemName);
    EUNIT_ASSERT(item == NULL);
    item = mountEntry->Item(EMountEntryItemUri);
    EUNIT_ASSERT(item == NULL);
    item = mountEntry->Item(EMountEntryItemUserName);
    EUNIT_ASSERT(item == NULL);
    item = mountEntry->Item(EMountEntryItemPassword);
    EUNIT_ASSERT(item == NULL);
    item = mountEntry->Item(EMountEntryItemIap);
    EUNIT_ASSERT(item == NULL);
    
    CleanupStack::PopAndDestroy(mountEntry);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestCloneL
// Tests CRsfwMountEntry::Clone
// ----------------------------------------------------------------------------
//
void T_MountMan::TestCloneL( )
    {
    TInt err;
    CRsfwMountEntry* mountEntry;
    mountEntry = CRsfwMountEntry::NewL();
    CleanupStack::PushL(mountEntry);

    // set entry items one by one
    TRAP(err, mountEntry->SetItemL(EMountEntryItemName, _L("name")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemUri, _L("http://url")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemUserName, _L("username")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemPassword, _L("password")));
    EUNIT_ASSERT(err == KErrNone);
    TRAP(err, mountEntry->SetItemL(EMountEntryItemIap, _L("iap")));
    EUNIT_ASSERT(err == KErrNone);

    // clone the entry
    CRsfwMountEntry* clonedEntry = mountEntry->CloneL();
    CleanupStack::PushL(clonedEntry);
    
    // check the values in cloned entry
    const HBufC* item = clonedEntry->Item(EMountEntryItemName);
    EUNIT_ASSERT((*item).Compare(_L("name")) == KErrNone);
    item = clonedEntry->Item(EMountEntryItemUri);
    EUNIT_ASSERT((*item).Compare(_L("http://url")) == KErrNone);
    item = clonedEntry->Item(EMountEntryItemUserName);
    EUNIT_ASSERT((*item).Compare(_L("username")) == KErrNone);
    item = clonedEntry->Item(EMountEntryItemPassword);
    EUNIT_ASSERT((*item).Compare(_L("password")) == KErrNone);
    item = clonedEntry->Item(EMountEntryItemIap);
    EUNIT_ASSERT((*item).Compare(_L("iap")) == KErrNone);
    
    CleanupStack::PopAndDestroy(clonedEntry);
    CleanupStack::PopAndDestroy(mountEntry);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestAddMountEntryL
// Tests that it is possible to add 9 mount entries, but not more
// ----------------------------------------------------------------------------
//
void T_MountMan::TestAddMountEntryL( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();

    TInt err;    
    err = AddMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);
    err = AddMountEntryL(1, 
                         _L("drivename1"), 
                         DriveToChar(EDriveL),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrNone);
    err = AddMountEntryL(2, 
                         _L("drivename2"), 
                         DriveToChar(EDriveM),
                         _L("http://url2.com"), 
                         _L("userName2"), 
                         _L("password2"), 
                         _L("iap2"));
    EUNIT_ASSERT(err == KErrNone);    
    err = AddMountEntryL(3, 
                         _L("drivename3"), 
                         DriveToChar(EDriveN),
                         _L("http://url3.com"), 
                         _L("userName3"), 
                         _L("password3"), 
                         _L("iap3"));
    EUNIT_ASSERT(err == KErrNone);    
    err = AddMountEntryL(4, 
                         _L("drivename4"), 
                         DriveToChar(EDriveO),
                         _L("http://url4.com"), 
                         _L("userName4"), 
                         _L("password4"), 
                         _L("iap4"));
    EUNIT_ASSERT(err == KErrNone);    
    err = AddMountEntryL(5, 
                         _L("drivename5"), 
                         DriveToChar(EDriveP),
                         _L("http://url5.com"), 
                         _L("userName5"), 
                         _L("password5"), 
                         _L("iap5"));
    EUNIT_ASSERT(err == KErrNone);	
    err = AddMountEntryL(6, 
                         _L("drivename6"), 
                         DriveToChar(EDriveQ),
                         _L("http://url6.com"), 
                         _L("userName6"), 
                         _L("password6"), 
                         _L("iap6"));
    EUNIT_ASSERT(err == KErrNone);	    
    err = AddMountEntryL(7, 
                         _L("drivename7"), 
                         DriveToChar(EDriveR),
                         _L("http://url7.com"), 
                         _L("userName7"), 
                         _L("password7"), 
                         _L("iap7"));
    EUNIT_ASSERT(err == KErrNone);	    
    err = AddMountEntryL(8, 
                         _L("drivename8"), 
                         DriveToChar(EDriveS),
                         _L("http://url8.com"), 
                         _L("userName8"), 
                         _L("password8"), 
                         _L("iap8"));
    EUNIT_ASSERT(err == KErrNone);	   
    // 10th drive should not pass! 
    err = AddMountEntryL(9, 
                         _L("drivename9"), 
                         DriveToChar(EDriveT),
                         _L("http://url9.com"), 
                         _L("userName9"), 
                         _L("password9"), 
                         _L("iap9"));
    EUNIT_ASSERT(err == KErrInUse);	    
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestAddMountEntry2L
// Tests that it is not possible to add two entries with the same name
// ----------------------------------------------------------------------------
//
void T_MountMan::TestAddMountEntry2L( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();

    TInt err;
    err = AddMountEntryL(0, 
                         _L("thesamename"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);
    err = AddMountEntryL(1, 
                         _L("thesamename"), 
                         DriveToChar(EDriveL),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrInUse);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestAddMountEntry3L
// Tests that it is not possible to add two entries with the same drive letter
// ----------------------------------------------------------------------------
//
void T_MountMan::TestAddMountEntry3L( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();

    TInt err;
    err = AddMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);
    err = AddMountEntryL(1, 
                         _L("drivename1"), 
                         DriveToChar(EDriveK),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrInUse);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestEditMountEntryL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestEditMountEntryL( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL(); 
    
    TInt err;    
    err = AddMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);

    err = EditMountEntryL(0, 
                         _L("differentName"), 
                         DriveToChar(EDriveK),
                         _L("http://different.com"), 
                         _L("differentUserName"), 
                         _L("differentPassword"), 
                         _L("differentIap"));
    EUNIT_ASSERT(err == KErrNone);
    
    const CRsfwMountEntry* entry = NULL;
    TRAP (err, entry = iMountMan->MountEntryL(DriveToChar(EDriveK)));
    EUNIT_ASSERT(err == KErrNone);
    
    err = CheckEntryL(entry,
                      0, 
                      _L("differentName"), 
                      DriveToChar(EDriveK),
                      _L("http://different.com"), 
                      _L("differentUserName"), 
                      _L("differentPassword"), 
                      _L("differentIap"));
    EUNIT_ASSERT(err == KErrNone);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestEditMountEntry2L
// tests that nonexising mount cannot be edited
// ----------------------------------------------------------------------------
//
void T_MountMan::TestEditMountEntry2L( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();

    TInt err;
    // try to edit nonexisting mount
    err = EditMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url1.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNotFound);    
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestEditMountEntry3L
// tests that it is not allowed to change the name into the one
// that is already in use
// ----------------------------------------------------------------------------
//
void T_MountMan::TestEditMountEntry3L( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();
    
    TInt err;    
    err = AddMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);

    err = AddMountEntryL(1, 
                         _L("drivename1"), 
                         DriveToChar(EDriveL),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrNone);
    
    // change the name of the 1st mount into the one used by the 2nd one
    err = EditMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveL),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrInUse);    
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestDeleteMountEntryL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestDeleteMountEntryL( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();
    
    TInt err;    
    err = AddMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);

    err = AddMountEntryL(1, 
                         _L("drivename1"), 
                         DriveToChar(EDriveL),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrNone);
    
    TRAP (err, iMountMan->DeleteMountEntryL(_L("drivename0")));
    EUNIT_ASSERT(err == KErrNone);

    TDriveList expectedList;
    expectedList.Append(DriveToChar(EDriveL));
    
    TDriveList returnedList;    
    TRAP(err,iMountMan->GetRemoteMountListL(returnedList));
    EUNIT_ASSERT(err == KErrNone);
    
    err = expectedList.Compare(returnedList);    
    EUNIT_ASSERT(err == KErrNone);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestDeleteMountEntry2L
// Tests that deleting nonexisting mount entry does not hurt
// ----------------------------------------------------------------------------
//
void T_MountMan::TestDeleteMountEntry2L( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();

    TInt err;
    TRAP (err, iMountMan->DeleteMountEntryL(_L("drivename0")));
    EUNIT_ASSERT(err == KErrNone);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestDeleteMountEntry3L
// tests DeleteMountEntryL(const TDesC& aName);
// ----------------------------------------------------------------------------
//
void T_MountMan::TestDeleteMountEntry3L( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();
    
    TInt err;    
    err = AddMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);

    err = AddMountEntryL(1, 
                         _L("drivename1"), 
                         DriveToChar(EDriveL),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrNone);
    
    TRAP (err, iMountMan->DeleteMountEntryL(DriveToChar(EDriveK)));
    EUNIT_ASSERT(err == KErrNone);

    TDriveList expectedList;
    expectedList.Append(DriveToChar(EDriveL));
    
    TDriveList returnedList;    
    TRAP(err,iMountMan->GetRemoteMountListL(returnedList));
    EUNIT_ASSERT(err == KErrNone);
    
    err = expectedList.Compare(returnedList);    
    EUNIT_ASSERT(err == KErrNone);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestMountEntryL
// tests MountEntryL(TChar aDriveLetter)
// ----------------------------------------------------------------------------
//
void T_MountMan::TestMountEntryL( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL(); 
    
    TInt err;    
    err = AddMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);

    err = AddMountEntryL(1, 
                         _L("drivename1"), 
                         DriveToChar(EDriveL),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrNone);

    const CRsfwMountEntry* entry = NULL;
    TRAP (err, entry = iMountMan->MountEntryL(DriveToChar(EDriveK)));
    EUNIT_ASSERT(err == KErrNone);
    
    err = CheckEntryL(entry,
                      0, 
                      _L("drivename0"), 
                      DriveToChar(EDriveK),
                      _L("http://url0.com"), 
                      _L("userName0"), 
                      _L("password0"), 
                      _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);       

    TRAP (err, entry = iMountMan->MountEntryL(DriveToChar(EDriveL)));
    EUNIT_ASSERT(err == KErrNone);
    
    err = CheckEntryL(entry,
                      1, 
                      _L("drivename1"), 
                      DriveToChar(EDriveL),
                      _L("http://url1.com"), 
                      _L("userName1"), 
                      _L("password1"), 
                      _L("iap1"));
    EUNIT_ASSERT(err == KErrNone); 
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestMountEntryL
// tests MountEntryL(const TDesC& aName)
// ----------------------------------------------------------------------------
//
void T_MountMan::TestMountEntry2L( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL(); 
    
    TInt err;    
    err = AddMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);

    err = AddMountEntryL(1, 
                         _L("drivename1"), 
                         DriveToChar(EDriveL),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrNone);

    const CRsfwMountEntry* entry = NULL;
    TRAP (err, entry = iMountMan->MountEntryL(_L("drivename0")));
    EUNIT_ASSERT(err == KErrNone);
    
    err = CheckEntryL(entry,
                      0, 
                      _L("drivename0"), 
                      DriveToChar(EDriveK),
                      _L("http://url0.com"), 
                      _L("userName0"), 
                      _L("password0"), 
                      _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);       

    TRAP (err, entry = iMountMan->MountEntryL(_L("drivename1")));
    EUNIT_ASSERT(err == KErrNone);
    
    err = CheckEntryL(entry,
                      1, 
                      _L("drivename1"), 
                      DriveToChar(EDriveL),
                      _L("http://url1.com"), 
                      _L("userName1"), 
                      _L("password1"), 
                      _L("iap1"));
    EUNIT_ASSERT(err == KErrNone); 
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestGetAllDrivesL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestGetAllDrivesL( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();    

    TInt err;        
    // add some remote drives
    err = AddMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);
    err = AddMountEntryL(1, 
                         _L("drivename1"), 
                         DriveToChar(EDriveL),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrNone);
    err = AddMountEntryL(2, 
                         _L("drivename2"), 
                         DriveToChar(EDriveM),
                         _L("http://url2.com"), 
                         _L("userName2"), 
                         _L("password2"), 
                         _L("iap2"));
    EUNIT_ASSERT(err == KErrNone);                     
    
    // prepare expected list
    TDriveList expectedList;

    // first populate the list with local drives
    TDriveList driveList;
    err = iFs.DriveList(driveList);
    EUNIT_ASSERT(err == KErrNone);
    TInt i;
    for ( i = EDriveC; i <= EDriveZ; i++) 
        {
        if ( (driveList[i]) && (!(driveList[i] & KDriveAttRemote)) )
            {
            TChar driveLetter;
            err = iFs.DriveToChar(i, driveLetter);
            EUNIT_ASSERT(err == KErrNone);
            expectedList.Append(driveLetter);
            }
        }
    // append remote drive that just has been added
    expectedList.Append(DriveToChar(EDriveK));
    expectedList.Append(DriveToChar(EDriveL));
    expectedList.Append(DriveToChar(EDriveM));

    TDriveList returnedList;    
    iMountMan->GetAllDrivesL(returnedList);
    
    err = expectedList.Compare(returnedList);
    EUNIT_ASSERT(err == KErrNone);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestGetRemoteMountListL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestGetRemoteMountListL( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();    

    TDriveList expectedList;
    TDriveList returnedList;

    TInt err;    
    err = AddMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);
    expectedList.Append(DriveToChar(EDriveK));
    err = AddMountEntryL(1, 
                         _L("drivename1"), 
                         DriveToChar(EDriveL),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrNone);
    expectedList.Append(DriveToChar(EDriveL));
    err = AddMountEntryL(2, 
                         _L("drivename2"), 
                         DriveToChar(EDriveM),
                         _L("http://url2.com"), 
                         _L("userName2"), 
                         _L("password2"), 
                         _L("iap2"));
    EUNIT_ASSERT(err == KErrNone);                     
    expectedList.Append(DriveToChar(EDriveM));
    
    TRAP(err,iMountMan->GetRemoteMountListL(returnedList));
    EUNIT_ASSERT(err == KErrNone);
    
    err = expectedList.Compare(returnedList);
    EUNIT_ASSERT(err == KErrNone);
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestGetMountNamesL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestGetMountNamesL( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();

    TInt err;
    // add some remote drives
    err = AddMountEntryL(0, 
                         _L("drivename0"), 
                         DriveToChar(EDriveK),
                         _L("http://url0.com"), 
                         _L("userName0"), 
                         _L("password0"), 
                         _L("iap0"));
    EUNIT_ASSERT(err == KErrNone);
    err = AddMountEntryL(1, 
                         _L("drivename1"), 
                         DriveToChar(EDriveL),
                         _L("http://url1.com"), 
                         _L("userName1"), 
                         _L("password1"), 
                         _L("iap1"));
    EUNIT_ASSERT(err == KErrNone);
    err = AddMountEntryL(2, 
                         _L("drivename2"), 
                         DriveToChar(EDriveM),
                         _L("http://url2.com"), 
                         _L("userName2"), 
                         _L("password2"), 
                         _L("iap2"));
    EUNIT_ASSERT(err == KErrNone);
    
    // prepare expected list of names
    CDesC16Array* expectedNames = new (ELeave) CDesC16ArraySeg(4);
    CleanupStack::PushL(expectedNames);
    expectedNames->AppendL(_L("drivename0"));
    expectedNames->AppendL(_L("drivename1"));
    expectedNames->AppendL(_L("drivename2"));
    
    CDesC16Array* names = new (ELeave) CDesC16ArraySeg(4);
    CleanupStack::PushL(names);
    TRAP (err, iMountMan->GetMountNamesL(names));

    if (err == KErrNone)
        {
        err = CompareArrays(names, expectedNames);
        }
    CleanupStack::PopAndDestroy(names);
    CleanupStack::PopAndDestroy(expectedNames);
    EUNIT_ASSERT(err == KErrNone);
    }   

// ----------------------------------------------------------------------------
// T_MountMan::TestIsAppOnBlackListL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestIsAppOnBlackListL( )
    {
    const TUid app1 = TUid::Uid(0x00000000);
    const TUid app2 = TUid::Uid(0x11111111);    

    TBool isOnList;

    isOnList = iMountMan->IsAppOnBlackList(app1);
    EUNIT_ASSERT(isOnList == EFalse);

    isOnList = iMountMan->IsAppOnBlackList(app2);
    EUNIT_ASSERT(isOnList == EFalse);        
    }     

// ----------------------------------------------------------------------------
// T_MountMan::TestGetMountInfoL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestGetMountInfoL( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();

    TInt err;
    // add remote drive
    err = AddMountEntryL(0, 
                         _L("drivename"), 
                         DriveToChar(EDriveK),
                         _L("http://testurl"),
                         _L("username"), 
                         _L("password"), 
                         _L(""));                    
    EUNIT_ASSERT(err == KErrNone);
    
    TRsfwMountInfo mountInfo;
    TRAP (err, iMountMan->GetMountInfo(DriveToChar(EDriveK), mountInfo));  
    EUNIT_ASSERT(err == KErrNone);
    
    // check as much mount info as possible at this point
    EUNIT_ASSERT(mountInfo.iMountConfig.iDriveLetter == DriveToChar(EDriveK));
    EUNIT_ASSERT(mountInfo.iMountConfig.iName.Compare(_L("drivename")) == KErrNone);
    EUNIT_ASSERT(mountInfo.iMountConfig.iUri.Compare(_L("http://testurl")) == KErrNone);
    EUNIT_ASSERT(mountInfo.iMountConfig.iUserName.Compare(_L("username")) == KErrNone);
    EUNIT_ASSERT(mountInfo.iMountConfig.iPassword.Compare(_L("password")) == KErrNone);    
    EUNIT_ASSERT(mountInfo.iMountStatus.iVolumeId == EDriveK);
    EUNIT_ASSERT(mountInfo.iMountStatus.iMountState == KMountStateDormant);
    EUNIT_ASSERT(mountInfo.iMountStatus.iConnectionState == KMountNotConnected);
    } 

// ----------------------------------------------------------------------------
// T_MountMan::TestSetMountConnectionStateL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestSetMountConnectionStateL( )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();

    TInt err;
    // add remote drive
    err = AddMountEntryL(0, 
                         _L("drivename"), 
                         DriveToChar(EDriveK),
                         _L("http://url"), 
                         _L("username"), 
                         _L("password"), 
                         _L(""));
    EUNIT_ASSERT(err == KErrNone);
    
    // set to not connected
    TRAP (err, iMountMan->SetMountConnectionState(DriveToChar(EDriveK), KMountNotConnected));
    EUNIT_ASSERT(err == KErrNone);

    // get mount info
    TRsfwMountInfo mountInfo;
    TRAP (err, iMountMan->GetMountInfo(DriveToChar(EDriveK), mountInfo));  
    EUNIT_ASSERT(err == KErrNone);

    // check mount status
    EUNIT_ASSERT(mountInfo.iMountStatus.iVolumeId == EDriveK);
    EUNIT_ASSERT(mountInfo.iMountStatus.iMountState == KMountStateDormant);
    EUNIT_ASSERT(mountInfo.iMountStatus.iConnectionState == KMountNotConnected);
    }  

// ----------------------------------------------------------------------------
// T_MountMan::TestRefreshDirectoryL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestRefreshDirectoryL(  )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();
    
    TInt err = iMountMan->RefreshDirectory(_L("K:\\nonexisting\\"));
    EUNIT_ASSERT(err == KErrNotFound);    
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestCancelRemoteTransferL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestCancelRemoteTransferL(  )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();
    
    TInt err = iMountMan->CancelRemoteTransfer(_L("K:\\nonexisting\\"));
    EUNIT_ASSERT(err == KErrNotFound);    
    }

// ----------------------------------------------------------------------------
// T_MountMan::TestSetMountConnectionStateBlindL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestSetMountConnectionStateBlindL(  )
    {
    // be sure no mount entries are in CenRep
    ClearAllMountsL();

    TInt err;
    // add remote drive
    err = AddMountEntryL(0, 
                         _L("drivename"), 
                         DriveToChar(EDriveK),
                         _L("http://url"), 
                         _L("username"), 
                         _L("password"), 
                         _L(""));
    EUNIT_ASSERT(err == KErrNone);
    
    // set to not connected
    TRAP (err, iMountMan->SetMountConnectionStateBlind(DriveToChar(EDriveK), KMountNotConnected));
    EUNIT_ASSERT(err == KErrNone);    
    }


///////////////////////////////////////////////////////////////////////////////
////// CRsfwMountEntry  ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// T_MountMan::TestExternalizeAndInternalizeL
// Tests TRsfwMountConfig::ExternalizeL & TRsfwMountConfig::InternalizeL
// ----------------------------------------------------------------------------
//
void T_MountMan::TestExternalizeAndInternalizeL(  )
    {
    TInt err;
    TRsfwMountConfig mountConfig;
    
    mountConfig.iDriveLetter = DriveToChar(EDriveK);
    mountConfig.iName.Copy(_L("name"));
    mountConfig.iUri.Copy(_L("uri"));
    mountConfig.iUserName.Copy(_L("username"));
    mountConfig.iPassword.Copy(_L("password"));
    mountConfig.iAuxData.Copy(_L("auxdata"));
    mountConfig.iFlags = KMountFlagNull;
    mountConfig.iInactivityTimeout = 10;
    
    RFile f;
    CleanupClosePushL(f);
    err = f.Replace(iFs, _L("C:\\mountmantest.txt"), EFileShareAny | EFileWrite);
    EUNIT_ASSERT(err == KErrNone);
    RFileWriteStream wStream(f);
    CleanupClosePushL(wStream);
    mountConfig.ExternalizeL(wStream);
    CleanupStack::PopAndDestroy(&wStream);
    CleanupStack::PopAndDestroy(&f);
    
    f.Open(iFs, _L("C:\\mountmantest.txt"), EFileShareAny | EFileRead);
    CleanupClosePushL(f);
    RFileReadStream rStream(f);
    CleanupClosePushL(rStream);
    TRsfwMountConfig anotherMountConfig;
    anotherMountConfig.InternalizeL(rStream);
    CleanupStack::PopAndDestroy(&rStream);
    CleanupStack::PopAndDestroy(&f);
    
    EUNIT_ASSERT(anotherMountConfig.iDriveLetter == DriveToChar(EDriveK));
    EUNIT_ASSERT(anotherMountConfig.iName.Compare(_L("name")) == KErrNone);
    EUNIT_ASSERT(anotherMountConfig.iUri.Compare(_L("uri")) == KErrNone);
    EUNIT_ASSERT(anotherMountConfig.iUserName.Compare(_L("username")) == KErrNone);
    EUNIT_ASSERT(anotherMountConfig.iPassword.Compare(_L("password")) == KErrNone);
    EUNIT_ASSERT(anotherMountConfig.iAuxData.Compare(_L("auxdata")) == KErrNone);
    EUNIT_ASSERT(anotherMountConfig.iFlags == KMountFlagNull);
    EUNIT_ASSERT(anotherMountConfig.iInactivityTimeout == 10);
    }

///////////////////////////////////////////////////////////////////////////////
////// Utility functions //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// T_MountMan::AddMountEntryL
// Helping function: based on given parameters creates CRsfwMountEntry object
// and attempts to add it using AddMountEntry()
// ----------------------------------------------------------------------------
//
TInt T_MountMan::AddMountEntryL(TInt aIndex,
                                const TDesC& aName,
                                TChar aDriveLetter,
                                const TDesC& aUri,
                                const TDesC& aUserName,
                                const TDesC& aPassword,
                                const TDesC& aIap)
    {
    TInt err;
	CRsfwMountEntry* mountEntry;
	mountEntry = CRsfwMountEntry::NewLC();   
    TRAP (err, mountEntry->SetEntryL(aIndex, 
                                      aName, 
                                      aDriveLetter, 
                                      aUri,
                                      aUserName, 
                                      aPassword, 
                                      aIap));

	if (err != KErrNone)
    	{
		CleanupStack::PopAndDestroy(mountEntry);
		return err;	
    	}

    // release ownership and add entry
    CleanupStack::Pop(mountEntry);
	TRAP (err, iMountMan->AddMountEntryL(mountEntry));
	return err;    
    }

// ----------------------------------------------------------------------------
// T_MountMan::EditMountEntryL
// Helping function: based on given parameters creates CRsfwMountEntry object
// and attempts to edit it using EditMountEntry()
// ----------------------------------------------------------------------------
//
TInt T_MountMan::EditMountEntryL(TInt aIndex,
                                const TDesC& aName,
                                TChar aDriveLetter,
                                const TDesC& aUri,
                                const TDesC& aUserName,
                                const TDesC& aPassword,
                                const TDesC& aIap)
    {
    TInt err;
	CRsfwMountEntry* mountEntry;
	mountEntry = CRsfwMountEntry::NewLC();   
    TRAP (err, mountEntry->SetEntryL(aIndex, 
                                      aName, 
                                      aDriveLetter, 
                                      aUri,
                                      aUserName, 
                                      aPassword, 
                                      aIap));

	if (err != KErrNone)
    	{
		CleanupStack::PopAndDestroy(mountEntry);
		return err;	
    	}

    // release ownership and edit entry
    CleanupStack::Pop(mountEntry);
	TRAP (err, iMountMan->EditMountEntryL(mountEntry));
	return err;    
    }

// ----------------------------------------------------------------------------
// T_MountMan::ClearAllMountsL
// Deletes all mount entries
// ----------------------------------------------------------------------------
//
void T_MountMan::ClearAllMountsL( )
    {
    TInt err;
    CDesC16Array* names = new (ELeave) CDesC16ArraySeg(4);
    CleanupStack::PushL(names);
    TRAP (err, iMountMan->GetMountNamesL(names));
    if (err == KErrNone)
        {
        TInt i;
        for ( i = 0; i < names->Count(); i++)
            {
            TRAP (err, iMountMan->DeleteMountEntryL((*names)[i]));
            if (err != KErrNone)
                {
                break;
                }
            }
        }
    names->Reset();    
    CleanupStack::PopAndDestroy(names);    
    EUNIT_ASSERT(err == KErrNone);
    }

// ----------------------------------------------------------------------------
// T_MountMan::DriveToChar
// ----------------------------------------------------------------------------
//
TChar T_MountMan::DriveToChar(TInt aDrive)
    {
    TChar c;
    TInt err = iFs.DriveToChar(aDrive, c);
    EUNIT_ASSERT(err == KErrNone);
    return c;
    }

// ----------------------------------------------------------------------------
// T_MountMan::CompareArrays
// Compares two arrays e.g. with mount names
// Note that order DOES matter!
// ----------------------------------------------------------------------------
//
TInt T_MountMan::CompareArrays(CDesC16Array* aArray1, CDesC16Array* aArray2)
    {	
	if (aArray1->Count() != aArray2->Count())
	    {
		return KErrNotFound;
	    }
    TInt i;
    for ( i = 0; i < aArray1->Count(); i++ )
        {
        if ((*aArray1)[i].Compare((*aArray2)[i]) != KErrNone)
            {
            return KErrNotFound;
            }
        }
    return KErrNone;
    }

// ----------------------------------------------------------------------------
// T_MountMan::CheckEntryL
// Checks whether all fields in given entry matches
// ----------------------------------------------------------------------------
//
TInt T_MountMan::CheckEntryL(const CRsfwMountEntry* aEntry,
                             TInt aIndex,
                             const TDesC& aName,
                             TChar aDriveLetter,
                             const TDesC& aUri,
                             const TDesC& aUserName,
                             const TDesC& aPassword,
                             const TDesC& aIap)
    {
    TInt err;
	CRsfwMountEntry* entry;
	entry = CRsfwMountEntry::NewLC();   
    TRAP (err, entry->SetEntryL(aIndex, 
                                aName, 
                                aDriveLetter, 
                                aUri,
                                aUserName, 
                                aPassword, 
                                aIap));  
    if (err != KErrNone)
        {
		CleanupStack::PopAndDestroy(entry);        
        return err;
        }
                                
    TInt i;
    for ( i = EMountEntryItemIndex; i < EMountEntryItemCount; i++ )
        {
        const HBufC* item1 = aEntry->Item(i);
        const HBufC* item2 = entry->Item(i);      
        if ( (item1 && !item2) ||
             (item2 && !item1) ||
             (item1 && item2 && (*item1).Compare(*item2)!=KErrNone) )
            {
		    CleanupStack::PopAndDestroy(entry);
            return KErrNotFound;
            }
        }

    CleanupStack::PopAndDestroy(entry);
    return KErrNone;
    }


//  TEST TABLE

EUNIT_BEGIN_TEST_TABLE(
    T_MountMan,
    "Add test suite description here.",
    "MODULE" )

EUNIT_TEST(
    "SetMountEntryL - set mount entry with too long attributes",
    "RsfwMountMan",
    "SetMountEntryL",
    "FUNCTIONALITY",
    SetupL, TestSetMountEntryL, Teardown)

EUNIT_TEST(
    "TestSetItemL - set mount items - SetItemL(TInt aIndex, const TDesC8& aValue)",
    "RsfwMountMan",
    "SetItemL",
    "FUNCTIONALITY",
    SetupL, TestSetItemL, Teardown)

EUNIT_TEST(
    "TestSetItemL - set mount items - SetItemL(TInt aIndex, const TDesC& aValue)",
    "RsfwMountMan",
    "SetItemL",
    "FUNCTIONALITY",
    SetupL, TestSetItem2L, Teardown)

EUNIT_TEST(
    "TestClearL - clear mount entry",
    "RsfwMountMan",
    "Clear",
    "FUNCTIONALITY",
    SetupL, TestClearL, Teardown)

EUNIT_TEST(
    "TestCloneL - clone mount entry",
    "RsfwMountMan",
    "CloneL",
    "FUNCTIONALITY",
    SetupL, TestCloneL, Teardown)

EUNIT_TEST(
    "AddMountEntryL - add max number of mounts",
    "RsfwMountMan",
    "AddMountEntryL",
    "FUNCTIONALITY",
    SetupL, TestAddMountEntryL, Teardown)
    
EUNIT_TEST(
    "AddMountEntryL - add two mounts with the same name",
    "RsfwMountMan",
    "AddMountEntryL",
    "FUNCTIONALITY",
    SetupL, TestAddMountEntry2L, Teardown)    

EUNIT_TEST(
    "AddMountEntryL - add two mounts with the same drive letter",
    "RsfwMountMan",
    "AddMountEntryL",
    "FUNCTIONALITY",
    SetupL, TestAddMountEntry3L, Teardown)

EUNIT_TEST(
    "EditMountEntryL - basic test",
    "RsfwMountMan",
    "EditMountEntryL",
    "FUNCTIONALITY",
    SetupL, TestEditMountEntryL, Teardown)

EUNIT_TEST(
    "EditMountEntryL - test nonexisting mount",
    "RsfwMountMan",
    "EditMountEntryL",
    "FUNCTIONALITY",
    SetupL, TestEditMountEntry2L, Teardown)

EUNIT_TEST(
    "EditMountEntryL - test the name that is in use",
    "RsfwMountMan",
    "EditMountEntryL",
    "FUNCTIONALITY",
    SetupL, TestEditMountEntry3L, Teardown)

EUNIT_TEST(
    "DeleteMountEntryL - basic test",
    "RsfwMountMan",
    "DeleteMountEntryL",
    "FUNCTIONALITY",
    SetupL, TestDeleteMountEntryL, Teardown)

EUNIT_TEST(
    "DeleteMountEntryL - test nonexisting mount",
    "RsfwMountMan",
    "DeleteMountEntryL",
    "FUNCTIONALITY",
    SetupL, TestDeleteMountEntry2L, Teardown)

EUNIT_TEST(
    "DeleteMountEntryL - by drive letter",
    "RsfwMountMan",
    "DeleteMountEntryL",
    "FUNCTIONALITY",
    SetupL, TestDeleteMountEntry3L, Teardown)

EUNIT_TEST(
    "TestMountEntryL - basic test",
    "RsfwMountMan",
    "MountEntryL",
    "FUNCTIONALITY",
    SetupL, TestMountEntryL, Teardown)

EUNIT_TEST(
    "TestMountEntryL - test nonexisting mount",
    "RsfwMountMan",
    "MountEntryL",
    "FUNCTIONALITY",
    SetupL, TestMountEntry2L, Teardown)
   
EUNIT_TEST(
    "GetAllDrivesL - basic test",
    "RsfwMountMan",
    "GetAllDrivesL",
    "FUNCTIONALITY",
    SetupL, TestGetAllDrivesL, Teardown)

EUNIT_TEST(
    "GetMountNamesL - basic test",
    "RsfwMountMan",
    "GetMountNamesL",
    "FUNCTIONALITY",
    SetupL, TestGetMountNamesL, Teardown)

EUNIT_TEST(
    "GetRemoteMountListL - basic test",
    "RsfwMountMan",
    "GetRemoteMountListL",
    "FUNCTIONALITY",
    SetupL, TestGetRemoteMountListL, Teardown)

EUNIT_TEST(
    "IsAppOnBlackList - basic test",
    "RsfwMountMan",
    "IsAppOnBlackList",
    "FUNCTIONALITY",
    SetupL, TestIsAppOnBlackListL, Teardown)

EUNIT_TEST(
    "GetMountInfoL - basic test",
    "RsfwMountMan",
    "GetMountInfoL",
    "FUNCTIONALITY",
    SetupL, TestGetMountInfoL, Teardown)
    
EUNIT_TEST(
    "SetMountConnectionStateL - basic test",
    "RsfwMountMan",
    "SetMountConnectionStateL",
    "FUNCTIONALITY",
    SetupL, TestSetMountConnectionStateL, Teardown)    

EUNIT_TEST(
    "RefreshDirectory - basic test",
    "RsfwMountMan",
    "RefreshDirectory",
    "FUNCTIONALITY",
    SetupL, TestRefreshDirectoryL, Teardown)

EUNIT_TEST(
    "CancelRemoteTransfer - basic test",
    "RsfwMountMan",
    "CancelRemoteTransfer",
    "FUNCTIONALITY",
    SetupL, TestCancelRemoteTransferL, Teardown)

EUNIT_TEST(
    "SetMountConnectionStateBlindL - basic test",
    "RsfwMountMan",
    "SetMountConnectionStateBlindL",
    "FUNCTIONALITY",
    SetupL, TestSetMountConnectionStateBlindL, Teardown)

EUNIT_TEST(
    "Externalize and internalize - basic test",
    "RsfwMountMan",
    "InternalizeL, ExternalizeL",
    "FUNCTIONALITY",
    SetupL, TestExternalizeAndInternalizeL, Teardown)

EUNIT_END_TEST_TABLE

//  END OF FILE
