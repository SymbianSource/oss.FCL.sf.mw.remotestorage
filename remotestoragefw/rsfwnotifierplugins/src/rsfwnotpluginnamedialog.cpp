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
* Description:  File name query dialog for "save as" dialog
*
*/

// ============================ MEMBER FUNCTIONS ===============================

#include <rsfwnotplugindlg.rsg>
#include <StringLoader.h>
#include <aknnotewrappers.h>
#include <bautils.h>
#include "rsfwnotpluginnamedialog.h"

// Left to right and right to left markers
_LIT( KDirectionalChars, "\x202A\x202B\x202C\x202D\x200E\x200F" );




// -----------------------------------------------------------------------------
// CRsfwNotPluginNameDialog::NewL
// 
// -----------------------------------------------------------------------------
// 
EXPORT_C CRsfwNotPluginNameDialog* CRsfwNotPluginNameDialog::NewL(
        const TDesC& aOldName,
        TDes& aNewName,
        RFs& aFs)
	{
	CRsfwNotPluginNameDialog* self =
	    new( ELeave ) CRsfwNotPluginNameDialog(
	        aNewName, aFs);
	
	CleanupStack::PushL( self );
	self->ConstructL( aOldName );
	CleanupStack::Pop( self );

	return self;
	}

// -----------------------------------------------------------------------------
// CRsfwNotPluginNameDialog::CRsfwNotPluginNameDialog
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CRsfwNotPluginNameDialog::CRsfwNotPluginNameDialog(
        TDes& aNewName, RFs& aFs ) : 
        CAknTextQueryDialog( aNewName ), iFs(aFs)
    {
    }

// -----------------------------------------------------------------------------
// CRsfwNotPluginNameDialog::ConstructL
// 
// -----------------------------------------------------------------------------
// 
void CRsfwNotPluginNameDialog::ConstructL( const TDesC& aOldName )
	{
    TParsePtrC name( aOldName );
	Text().Copy( name.NameAndExt() );
    iOldName = aOldName.AllocL();

    // Strip any directionality markers to get pure name
    TPtr ptr( iOldName->Des() );
    AknTextUtils::StripCharacters( ptr, KDirectionalChars );
	}

// -----------------------------------------------------------------------------
// CRsfwNotPluginNameDialog::~CRsfwNotPluginNameDialog
// Destructor
// -----------------------------------------------------------------------------
// 
CRsfwNotPluginNameDialog::~CRsfwNotPluginNameDialog()
    {
    delete iOldName;
    }

// -----------------------------------------------------------------------------
// CRsfwNotPluginNameDialog::OkToExitL
// 
// -----------------------------------------------------------------------------
// 
TBool CRsfwNotPluginNameDialog::OkToExitL( TInt aButtonId )
    {
    TBool result( CAknTextQueryDialog::OkToExitL( aButtonId ) );

    HBufC* userText = Text().AllocLC();
    TPtr ptrUserText( userText->Des() );

    // Strip any directionality markers to get pure name
    AknTextUtils::StripCharacters( ptrUserText, KDirectionalChars );

    // Check file name
    TBool isValidName( EFalse );
    TText badChar(NULL);
    isValidName = iFs.IsValidName(*userText, badChar);
   
        
	if( !isValidName )
        {
        TBuf<5> dotbuf;
        dotbuf.Append(KDot);
        TChar dot(dotbuf[0]);
        if (badChar == dot)
            {
            // dot is a special case, as "." or ".." are illegal file names
            // but for example "file.name" is not, and thus dot is not mentioned
            // in R_RD_FLDR_ILLEGAL_CHARACTERS
            ShowSimpleInfoNoteL(R_RD_FLDR_BAD_FILE_NAME );
            }
        else 
            {
            ShowSimpleInfoNoteL(R_RD_FLDR_ILLEGAL_CHARACTERS );
            }
       
        CAknQueryControl* queryControl = QueryControl();
        if (queryControl)
            {
            CEikEdwin* edwin = static_cast< CEikEdwin* >(
                queryControl->ControlByLayoutOrNull( EDataLayout ) );
            if (edwin)
                {
                edwin->SetSelectionL( edwin->TextLength(), 0 );
                }
            }
        CleanupStack::PopAndDestroy( userText );
        return EFalse;
        }
    CleanupStack::PopAndDestroy( userText );
    return result;
    
    }
    

// ------------------------------------------------------------------------------
// CRsfwNotPluginNameDialog::ShowSimpleInfoNoteL
//
// ------------------------------------------------------------------------------
//
void CRsfwNotPluginNameDialog::ShowSimpleInfoNoteL(
        const TInt aTextId)
    {
    HBufC* text = NULL;
    text = StringLoader::LoadLC( aTextId );
    CAknInformationNote* dlg = new(ELeave) CAknInformationNote( ETrue );
    dlg->ExecuteLD( *text );
    CleanupStack::PopAndDestroy( text );
    }



