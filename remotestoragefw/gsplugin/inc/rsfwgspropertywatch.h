/*
* Copyright (c) 2002-2005 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  P&S observer
*
*/


#ifndef CRSFWGSPROPERTYWATCH_H
#define CRSFWGSPROPERTYWATCH_H

#include <e32base.h>
#include <e32property.h>
#include <f32file.h>

// FORWARD DECLARATIONS
class CRsfwGsPluginDriveListContainer;

class CRsfwGsPropertyWatch : public CActive
{
	enum { EPriority=0 };
	
public:
	static CRsfwGsPropertyWatch* NewL(CRsfwGsPluginDriveListContainer* aContainer);
	~CRsfwGsPropertyWatch();
private:
	CRsfwGsPropertyWatch();
	void ConstructL(CRsfwGsPluginDriveListContainer* aContainer);
	void RunL();
	void DoCancel();
private:
	RProperty iProperty;
	CRsfwGsPluginDriveListContainer* iContainer;
	TDriveList iDriveList;
};

#endif // CRSFWGSPROPERTYWATCH_H

// End of File