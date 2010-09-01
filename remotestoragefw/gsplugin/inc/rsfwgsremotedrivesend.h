/*
* Copyright (c) 2002 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Rsfw GS plugin, for sending drive
 *
*/


#ifndef CRSFWGSREMOTEDRIVESEND_H
#define CRSFWGSREMOTEDRIVESEND_H

// INCLUDES
#include <e32base.h>

// FORWARD DECLARATIONS;
class CRsfwMountEntry;
class CEikMenuPane;
class CSendUi;
class TParse;
class RFile;

// path and filename for vCalendar attachment
_LIT( KRemoteDriveAttachmentFilename, "c:\\system\\data\\rsfw_cache\\rdrive.cfg" );

NONSHARABLE_CLASS( CRsfwGsRemoteDriveSend ) : public CBase
    {
public: // Factory method and destructor 
    static CRsfwGsRemoteDriveSend* NewL(TInt aMenuCommandId);
    virtual ~CRsfwGsRemoteDriveSend();

public: // API 
    TBool CanSend();
    void DisplaySendMenuItemL(CEikMenuPane& aMenuPane, TInt aIndex);
    void DisplaySendCascadeMenuL();
    void SendL(const CRsfwMountEntry& aEntry);
    
private: // utility functions
    void DoSendAsAttachmentFileL(TInt aCommand, TParse& aFilename);

    void DoSendAsAttachmentHandleL(const RFile& aHandle);
    
    HBufC8* ConvertToUtf7LC(const TDesC16& aText);

private: // constrution 
    CRsfwGsRemoteDriveSend();
    void ConstructL(TInt aMenuCommandId);
private: // data
    CSendUi* iSendUi;
    TInt iSendAsCmdId;
    TUid iSelectedMtmUid;
    CArrayFixFlat<TUid>* iSendMtmsToDim;
 

    }; 

#endif // CRSFWGSREMOTEDRIVESEND_H


// End of File
