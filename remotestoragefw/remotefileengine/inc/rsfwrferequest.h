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
* Description:  Base class for request encapsulation.
*
*/

#ifndef C_RSFWRFEREQUEST_H
#define C_RSFWRFEREQUEST_H

class TParse;
class TRfeInArgs;
class TRfeOutArgs;
class CRsfwVolumeTable;
class CRsfwVolume;
class CRsfwRfeOperation;


#include <e32base.h>

// type of the request
// in the future we could have e.g. internal requests
enum TRequestType 
	{
	EMessageRequest
	};


/**
 *  Base class for request encapsulation.
 *
 *
 */
class CRsfwRfeRequest : public CBase
    {
public:
    ~CRsfwRfeRequest();
    //
    virtual void CompleteAndDestroy(TInt aError) = 0;  // pure virtual
    
    void Destroy();
    virtual void Dispatch();
    virtual TParse& Src();
    virtual TParse& Dest();
    //
    CRsfwRfeOperation* Operation();
    void SetOperation(CRsfwRfeOperation* aCaller);
    TRequestType RequestType();
    void SetRequestType(TRequestType aRequestType);
    
// Request parameters
public:
    TRfeInArgs*    iInArgs;
    TRfeOutArgs*   iOutArgs;

    CRsfwVolumeTable*  iVolumeTable;
    // volume can be null in mount operation, but not after that  
    CRsfwVolume*       iVolume;
public:
    TDblQueLink    iLink;
protected:
    CRsfwRfeOperation* iOperation;
private:
    TRequestType   iRequestType;
    };

#endif

