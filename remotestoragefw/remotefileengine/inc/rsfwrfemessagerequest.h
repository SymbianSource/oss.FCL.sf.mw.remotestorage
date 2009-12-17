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
* Description:  A request sent by a client (i.e. non-internal request)
*
*/

#ifndef C_RSFWRFEMESSAGEREQUEST_H
#define C_RSFWRFEMESSAGEREQUEST_H

#include "rsfwrferequest.h"

class CRsfwRfeSession;


/**
 *  A request sent by a client (i.e. non-internal request)
 *
 *  In practise from Remote file-system plugin
 *
 */
class CRsfwRfeMessageRequest : public CRsfwRfeRequest
    {
public:
    CRsfwRfeMessageRequest();
    void SetL(const RMessage2& aMessage,CRsfwRfeSession* aSession);
    const RMessage2& Message();
    CRsfwRfeSession* Session();
    void SetSession(CRsfwRfeSession* aSession);
    //
    void Complete(TInt aError);
    void CompleteAndDestroy(TInt aError);
private:
    void Destroy();    
public:
    TBool iMessageCompleted;

protected:
    RMessage2 iMessage;
    CRsfwRfeSession* iSession;
    };

#endif