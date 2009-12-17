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
* Description:  notifier plugin request encapsulation
*
*/

#ifndef T_RSFWNOTPLUGINGREQUEST_H
#define T_RSFWNOTPLUGINGREQUEST_H

const TUid KRsfwNotifierPluginUID     = { 0x101F9770 }; 

// CONSTANTS
const TInt KRsfwMaxDrivenameLength = 20;
const TInt KRsfwMaxDriveletterLength = 2;
const TInt KRsfwMaxFileSizeString = 20;
const TInt KRsfwMaxUsernameLength = 50;
const TInt KRsfwMaxPasswordLength = 50;
const TInt KRsfwNotifierMsgDataMaxSize = 1024;

/**
* Class TRsfwNotPluginRequest
* Base class for fixed sized requests transferred between client and plug-in
* notifier implementation.
* This is used internally. This should not be instantiated by the clients.
*/
class TRsfwNotPluginRequest
   	{
	public:

   		/**
   		*  List of supported functionalities in plug-in implementation
   		*/
   		enum TRsfwNotPluginMethod
       	{
       	ENoMethod,
       	EAuthenticationDlg,
       	ESaveToDlg,
       	EUnavailableRetryDlg,
       	// wait dialogs, note that these could come after EUnavailableRetryDlg
       	EConnectingDlg,
       	EFetchingDlg
       	};
       	
       	
    // method	
    TRsfwNotPluginMethod iMethod;
    
    // Drive Friendly name
    TBuf<KRsfwMaxDrivenameLength>  iDriveName;
    
    
    };

// Package buffer to hold parameter information
typedef TPckgBuf<TRsfwNotPluginRequest> TRsfwRetryParamsPckg;

#endif