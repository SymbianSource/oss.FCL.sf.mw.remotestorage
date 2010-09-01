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
* Description:  Sending remote drive configuration entries
*
*/


#include <eikenv.h>
#include <sendui.h>
#include <CMessageData.h>
#include <SendUiConsts.h>
#include <txtrich.h>
#include <utf.h> // for CnvUtfConverter
#include <StringLoader.h>
#include <AknQueryDialog.h>
#include <sysutil.h>
#include <rsfwgspluginrsc.rsg>
#include <rsfwmountentry.h>

#include "rsfwgsremotedrivesend.h"
#include "rsfwmountutils.h"
#include "rsfwgsplugin.hrh"


// ---------------------------------------------------------
// CRsfwGsRemoteDriveSend::NewL
// Static constructor
// (other items were commented in a header).
// ---------------------------------------------------------
//
CRsfwGsRemoteDriveSend* CRsfwGsRemoteDriveSend::NewL(TInt aMenuCommandId)
    {
    CRsfwGsRemoteDriveSend* self = new (ELeave) CRsfwGsRemoteDriveSend();
    CleanupStack::PushL(self);
    self->ConstructL(aMenuCommandId);
    CleanupStack::Pop(self);
    return self;
    }

// ---------------------------------------------------------
// CRsfwGsRemoteDriveSend::CRsfwGsRemoteDriveSend
// C++ constructor
// (other items were commented in a header).
// ---------------------------------------------------------
//
CRsfwGsRemoteDriveSend::CRsfwGsRemoteDriveSend()
    {
    }

// ---------------------------------------------------------
// CRsfwGsRemoteDriveSend::ConstructL
// 2nd phase constructor
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CRsfwGsRemoteDriveSend::ConstructL(TInt aMenuCommandId)
    {
    iSendUi = CSendUi::NewL();
    iSendAsCmdId=aMenuCommandId;
    iSendMtmsToDim = new (ELeave) CArrayFixFlat<TUid>(4);

    // for the time being, dim everything else but SMS
    // for some reason technology-group UIDs do not seem to work
    // e-mail Mtmss
    iSendMtmsToDim->AppendL(KSenduiMtmSmtpUid);
    iSendMtmsToDim->AppendL(KSenduiMtmImap4Uid);
    iSendMtmsToDim->AppendL(KSenduiMtmPop3Uid);
    
    // MMS, BT, Irda, Postcard
    iSendMtmsToDim->AppendL(KSenduiMtmMmsUid);
    iSendMtmsToDim->AppendL(KSenduiMtmPostcardUid);
    iSendMtmsToDim->AppendL(KSenduiMtmBtUid);
    iSendMtmsToDim->AppendL(KSenduiMtmIrUid);
    
    // Audio Message
    iSendMtmsToDim->AppendL(KSenduiMtmAudioMessageUid);
    
    iSendMtmsToDim->AppendL(KSenduiMtmSyncMLEmailUid  );
    
    // filters out EMail Via Exchange, as we only support SMS
    // for some reason this cannot be found from headers currently
    const TUid KEmailViaExchange        = {  0x102826F8    };
    iSendMtmsToDim->AppendL(KEmailViaExchange );
      
    }

// ---------------------------------------------------------
// CRsfwGsRemoteDriveSend::~CRsfwGsRemoteDriveSend
// Destructor
// (other items were commented in a header).
// ---------------------------------------------------------
//
CRsfwGsRemoteDriveSend::~CRsfwGsRemoteDriveSend()
    {
    delete iSendUi;
    delete iSendMtmsToDim;
    }


// ---------------------------------------------------------
// CRsfwGsRemoteDriveSend::CanSendL
// Check wheter sending is possible
// (other items were commented in a header).
// ---------------------------------------------------------
//
TBool CRsfwGsRemoteDriveSend::CanSend()
    {
    if( iSelectedMtmUid != KNullUid )
        {
        return ETrue;
        }
    else
        {
        return EFalse;
        }
    }

// ---------------------------------------------------------
// CRsfwGsRemoteDriveSend::DisplaySendMenuItemL
// Show sendui menu
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CRsfwGsRemoteDriveSend::DisplaySendMenuItemL(CEikMenuPane& aMenuPane, 
                                               TInt aIndex)
    {
    iSendUi->AddSendMenuItemL( aMenuPane, aIndex, iSendAsCmdId, TSendingCapabilities() );
    }

// ---------------------------------------------------------
// CRsfwGsRemoteDriveSend::DisplaySendCascadeMenuL
// Show send quesry / cascaded menu
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CRsfwGsRemoteDriveSend::DisplaySendCascadeMenuL()
    {
    iSelectedMtmUid = iSendUi->ShowSendQueryL( NULL, KCapabilitiesForAllServices, iSendMtmsToDim, KNullDesC );
    }

    

// ---------------------------------------------------------
// CRsfwGsRemoteDriveSend::SendL
// Send a remote drive entry via SendUi
// (other items were commented in a header).
// ---------------------------------------------------------
//
void CRsfwGsRemoteDriveSend::SendL(const CRsfwMountEntry& anEntry)
    {

    // the old entry can be deleted in the middle of the operation
    // e.g. from the File Manager
    CRsfwMountEntry* entryToBeSent = anEntry.CloneL();
    CleanupStack::PushL(entryToBeSent);
    
    CEikonEnv* eikonEnv = CEikonEnv::Static();
    CRichText* text = CRichText::NewL(
        eikonEnv->SystemParaFormatLayerL(),
        eikonEnv->SystemCharFormatLayerL());
    CleanupStack::PushL(text);
    
    
     // if user name has been set
    // ask user whether he would like to send the login credentials
    TBool sendCredentials = EFalse;
    if ((entryToBeSent->Item(EMountEntryItemUserName))->Length() > 0) 
        {
        HBufC* myDisplayMessage = NULL;
        myDisplayMessage = StringLoader::LoadLC(R_STR_RSFW_SEND_CREDENTIALS_QUERY);
        CAknQueryDialog* query = CAknQueryDialog::NewL
                                        (CAknQueryDialog::EConfirmationTone);    
        sendCredentials = 
                query->ExecuteLD( R_CONFIRMATION_QUERY, *myDisplayMessage);
        CleanupStack::PopAndDestroy(myDisplayMessage);
        }
    
    
     // Encode configuration entry
    HBufC* vcal = HBufC::NewLC(KMaxMountConfLength);
    TPtr p = vcal->Des();
    RsfwMountUtils::ExportMountEntryL(*entryToBeSent, sendCredentials, p);   
    
    // convert to 8-bit
    // note that safe conversion is needed both when sending
    // as an attachement and when sending as message text
    HBufC8* temp = ConvertToUtf7LC(p); 

    TUid mtmUid = iSelectedMtmUid;
    TSendingCapabilities capabilities;
    iSendUi->ServiceCapabilitiesL( mtmUid, capabilities );
    if (capabilities.iFlags & TSendingCapabilities::ESupportsAttachments)
        {
         // send as Attachment
        RFs fs;
        User::LeaveIfError( fs.Connect() );
        CleanupClosePushL( fs );
        // must share the handle between processes
        User::LeaveIfError( fs.ShareProtected() );
        TInt err = fs.MkDirAll(KRemoteDriveAttachmentFilename);
        RFile file;
        err = file.Replace(fs,KRemoteDriveAttachmentFilename,EFileWrite | EFileShareAny );
        CleanupClosePushL(file);
        TInt spaceNeeded = vcal->Size();
        if ( SysUtil::FFSSpaceBelowCriticalLevelL( &fs, spaceNeeded ) )
            {
            // don't show any own notes here
            User::Leave( KErrDiskFull );
            }

        User::LeaveIfError(file.Write(*temp));

        TParse parse;
        User::LeaveIfError(parse.SetNoWild(KRemoteDriveAttachmentFilename,
                                           NULL, NULL));

        CMessageData* messageData = CMessageData::NewL();
        CleanupStack::PushL( messageData );
        messageData->AppendAttachmentHandleL(file);
        iSendUi->CreateAndSendMessageL( iSelectedMtmUid, messageData, KUidBIOMountConfMsg, ETrue );
      
        CleanupStack::PopAndDestroy( 2 ); // messageData, file
        fs.Delete(parse.FullName());
        CleanupStack::PopAndDestroy( &fs );;
        }
   else
        {
        // send as message body
        // message data interface is 16-bit
        // however, along the way 8-bit interface is used 
        // to pass the data without safe conversion
        // so the unicode conversion above is still needed
        HBufC* bufCnv = HBufC::NewLC(temp->Length());
        TPtr16 des(bufCnv->Des());
        des.Copy(p);
        text->InsertL(0, des);

        CMessageData* messageData = CMessageData::NewL();
        CleanupStack::PushL( messageData );
        messageData->SetBodyTextL( text );
        iSendUi->CreateAndSendMessageL( iSelectedMtmUid, messageData, KUidBIOMountConfMsg, ETrue );
        CleanupStack::PopAndDestroy(2); // messageData, bufCnv
        }         
    
    CleanupStack::PopAndDestroy(4); // entryToBeSent, text, vcal, temp
 
    }

// ------------------------------------------------------------------------------------------------
// HBufC* CRsfwGsRemoteDriveSend::ConvertToUtf7LC
// Encodes from Unicode UCS-2 to UTF-8
// ------------------------------------------------------------------------------------------------
HBufC8* CRsfwGsRemoteDriveSend::ConvertToUtf7LC(const TDesC16& aText)
    {
    TPtrC16    remainder( aText );
    TBuf8<20>  utfBuffer;
    HBufC8     *ret    = 0;
    CBufFlat   *buffer = CBufFlat::NewL( 128 );
    CleanupStack::PushL( buffer );

    TBool finish = EFalse;
    while( !finish )
        {
        utfBuffer.Zero();
        TInt unconverted = CnvUtfConverter::ConvertFromUnicodeToUtf7(utfBuffer, remainder, EFalse );
        if( unconverted >= 0 )
            {
            remainder.Set( remainder.Right( unconverted ) );
            buffer->InsertL( buffer->Size(), utfBuffer );
            finish = (unconverted == 0);
            }
        else
            {
            User::Leave( unconverted );
            }
        }

    buffer->Compress();
    ret = buffer->Ptr( 0 ).Alloc();
    CleanupStack::PopAndDestroy( buffer );
    CleanupStack::PushL( ret );
    return ret;
    }



// end of file
