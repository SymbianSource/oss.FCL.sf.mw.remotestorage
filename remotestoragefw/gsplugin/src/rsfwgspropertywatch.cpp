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
* Description:  RsfwPlugin Implementation
*
*/


#include <rsfwmountman.h>
#include <rsfwmountentry.h>

#include "rsfwgspropertywatch.h"
//#include "rsfwgsplugin.hrh"
#include "rsfwgsplugindrivelistcontainer.h"
#include "rsfwcommon.h"


CRsfwGsPropertyWatch* CRsfwGsPropertyWatch::NewL(CRsfwGsPluginDriveListContainer* aContainer) 
	{
	CRsfwGsPropertyWatch* me=new(ELeave) CRsfwGsPropertyWatch;
	CleanupStack::PushL(me);
	me->ConstructL(aContainer);
	CleanupStack::Pop(me);
	return me;
	}

CRsfwGsPropertyWatch::CRsfwGsPropertyWatch()
	:CActive(EPriority)
	{}
	
void CRsfwGsPropertyWatch::ConstructL(CRsfwGsPluginDriveListContainer* aContainer)
	{
	iContainer = aContainer;
	User::LeaveIfError(iProperty.Attach(KRfeServerSecureUid, ERsfwPSKeyConnect));
	CActiveScheduler::Add(this);
	iProperty.Subscribe(iStatus);
	SetActive();
	}
	
CRsfwGsPropertyWatch::~CRsfwGsPropertyWatch()	
	{
	Cancel();
	iProperty.Close();
	}
	
void CRsfwGsPropertyWatch::DoCancel()
{
	iProperty.Cancel();
}

void CRsfwGsPropertyWatch::RunL()
{
	// resubscribe before processing new value to prevent missing updates
	iProperty.Subscribe(iStatus);
	SetActive();
	if ((iProperty.Get(KRfeServerSecureUid, ERsfwPSKeyConnect, iDriveList) == KErrNone) &&
	// if the key is defined but not written to, the length of the list is zero
		(iDriveList.Length() == KMaxDrives))  
	{
		TDriveList fsDriveList;
		RFs fs;
		User::LeaveIfError(fs.Connect());
		CleanupClosePushL(fs);
		fs.DriveList(fsDriveList, KDriveAttRemote);		
		TInt drive = EDriveY;
		while (drive >=0) {
        	if (fsDriveList[drive] && (fsDriveList[drive] & KDriveAttRemote)) {
        		// get the friendly name for this drive
        		TChar driveChar;
        		fs.DriveToChar(drive, driveChar);
        		const CRsfwMountEntry* mountEntry;
        		mountEntry = iContainer->iMountMan->MountEntryL(driveChar);
        		if (!mountEntry) 
        		    {
        		    User::Leave(KErrNotFound);
        		    }
        		const HBufC* mountName;
        		mountName= mountEntry->Item(EMountEntryItemName);
        		if (!mountName) 
        		    {
        		    User::Leave(KErrNotFound);
        		    }
        		if (iDriveList[drive] == 0) 
        		{
        			 iContainer->SetDriveConnectedStateL(*mountName, EFalse);
        		}
        		else if (iDriveList[drive] == 1) 
        		{
        			iContainer->SetDriveConnectedStateL(*mountName, ETrue);	
        		}
        	}
        	drive--;	
		}
       CleanupStack::PopAndDestroy(); // fs
	}
}

// End of File
