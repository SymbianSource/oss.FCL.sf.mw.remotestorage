/*
* Copyright (c) 2003-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Remote File Engine session manager
*
*/


#ifndef C_RSFWRFESESSION_H
#define C_RSFWRFESESSION_H

#include <e32base.h>
#include "rsfwrfemessagerequest.h"
#include "rsfwcontrol.h" // KDefaultMessageSlots = 4

class CRsfwRfeServer;
class CRsfwRfeOperation;
class CRsfwVolumeTable;
class CRsfwRfeRequest;

// CLASS DECLARATION
// class CRsfwRfeSession: public CSharableSession

class CRsfwRfeSession: public CSession2
{
public:
    static CRsfwRfeSession* NewL(CRsfwRfeServer& aServer);
    static CRsfwRfeSession* NewLC(CRsfwRfeServer& aServer);
    ~CRsfwRfeSession();
    void ServiceL(const RMessage2& aMessage);
    CRsfwVolumeTable* Volume();
    CRsfwRfeServer* Server();
    
    void RemoveFromMessageRequestArray(CRsfwRfeMessageRequest* aMessageRequest);
 private:
    CRsfwRfeSession(CRsfwRfeServer& aServer);
    CRsfwRfeOperation* GetOperation(CRsfwRfeRequest* pR, TInt aOperation);
    void ConstructL();
#ifndef SHARABLE_SESSION
    void PanicClient(TInt aPanic) const;
#endif
    void SetToMessageRequestArray(CRsfwRfeMessageRequest* aMessageRequest);

protected:
    void Disconnect(const RMessage2& aMessage);
    

private:
    CRsfwRfeServer& iRfeServer; 
    CRsfwVolumeTable* iVolumes;
    CRsfwRfeMessageRequest* iMessageRequests[KDefaultMessageSlots];
    };

#endif RFESESSION_H

// End of File
