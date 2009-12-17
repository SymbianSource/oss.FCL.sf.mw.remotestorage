/*
* Copyright (c) 2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Authentication dialog request to the notifier plugin
*
*/


#ifndef T_RSFWAUTHENTICATIONDLGREQUEST_H
#define T_RSFWAUTHENTICATIONDLGREQUEST_H

#include "e32cmn.h"

#include "rsfwnotpluginrequest.h"

/**
*  Parameters for authentication dialog request
*
*/
class TRsfwAuthenticationDlgRequest : public TRsfwNotPluginRequest
    {
public:
    // current username
    TBuf<KRsfwMaxUsernameLength>  iUserName;
    
    // current password
    TBuf<KRsfwMaxPasswordLength>  iPassword;
    	
    };

// Package buffer to hold auht request parameter information
typedef TPckgBuf<TRsfwAuthenticationDlgRequest> TRsfwAuthParamsPckg;

#endif