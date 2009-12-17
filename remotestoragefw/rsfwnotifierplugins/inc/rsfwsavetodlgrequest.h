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
* Description:  File selection dialog request to the notifier plugin
*
*/


#ifndef T_RSFWSAVETODLGREQUEST_H
#define T_RSFWSAVETODLGREQUEST_H

#include "e32cmn.h"

#include "rsfwnotpluginrequest.h"


/**
* Class TRsfwSaveToDlgRequest
*/
class TRsfwSaveToDlgRequest : public TRsfwNotPluginRequest
    {
	public: // New functions

     // File name
    TBuf<KMaxFileName>  iFileName;  
    
    // Drive letter
    TBuf<KRsfwMaxDriveletterLength>  iCacheDrive;  
    
    // File size as a string (in bytes)
    TBuf<KRsfwMaxFileSizeString>  iFileSize;  
    
    };

// Package buffer to hold update parameter information
typedef TPckgBuf<TRsfwSaveToDlgRequest> TRsfwSaveToParamsPckg;

#endif