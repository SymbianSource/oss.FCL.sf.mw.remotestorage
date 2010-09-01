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
* Description:  BIO control for handling remote drive configurations as 
*                smart messages
*
*/

 
// INCLUDE FILES

#include <biocmtm.h>					// for CBIOClientMtm
#include <msgbiocontrolObserver.h>		// for MMsgBioControlObserver
#include <StringLoader.h>				// for StringLoader
#include <CRichBio.h>               	// CRichBio
#include <aknnotewrappers.h>			// for CAknInformationNote
#include <mmsvattachmentmanager.h>
#include <utf.h> // for CnvUtfConverter
#include <rsfwmountman.h>
#include <rsfwmountentry.h>

#include <rsfwmountconfbc.rsg>              // for resouce identifiers

#include "rsfwmountconfbc.h"				
#include "rsfwmountconfbc.hrh"
#include "rsfwmountutils.h"
#include "rsfwgsplugin.hrh"
#include "rsfwnotpluginnamedialog.h"
#include "mdebug.h"

#define KUidGeneralSettings   0x100058EC

enum TOptionListLocation
	{
	EFirstMenuItem	= 0,
	ESecondMenuItem,
	EThirdMenuItem,
	EFourthMenuItem
	};

const TInt KMConfBcHeightReductionBva = 9;
_LIT(KMountConfBcResourceFile, "rsfwmountconfbc.rsc");
_LIT(KAvkonResourceFile, "avkon.rsc");


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// constructor
// ---------------------------------------------------------------------------
//
EXPORT_C CMsgBioControl* CRsfwMountConfBioControl::NewL(
		MMsgBioControlObserver& aObserver,
		CMsvSession* aSession,
		TMsvId aId,
		TMsgBioMode aEditorOrViewerMode,
		const RFile* aFile)
	{
	CRsfwMountConfBioControl* self =
		new(ELeave) CRsfwMountConfBioControl(aObserver,
										 aSession,
										 aId,
										 aEditorOrViewerMode,
										 aFile);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);

	return self;
	}

// ---------------------------------------------------------------------------
// destructor
// ---------------------------------------------------------------------------
//
CRsfwMountConfBioControl::~CRsfwMountConfBioControl()
	{	
    delete iViewer;

	if (iMountEntry) 						
		{
		delete iMountEntry;
		}
	delete iMountMan;
	}

// ---------------------------------------------------------------------------
// constructor
// ---------------------------------------------------------------------------
//
CRsfwMountConfBioControl::CRsfwMountConfBioControl(
		 MMsgBioControlObserver& aObserver,
		 CMsvSession* aSession,
		 TMsvId aId,
		 TMsgBioMode aEditorOrViewerMode,
		 const RFile* aFile):
		 CMsgBioControl(aObserver,
						aSession,
						aId,
						aEditorOrViewerMode,
						aFile)
	{
	}
	
// ---------------------------------------------------------------------------
// second phase constructor
// ---------------------------------------------------------------------------
//
void CRsfwMountConfBioControl::ConstructL()
	{
	DEBUGSTRING16(("CRsfwMountConfBioControl::ConstructL"));
	iMountMan = CRsfwMountMan::NewL(0, NULL);
	
	iIsFileBased = IsFileBased();
	LoadResourceL(KMountConfBcResourceFile);
	LoadResourceL(KAvkonResourceFile);
	LoadResourceL(KNotPluginResourcePath);
    LoadStandardBioResourceL();
	// file handle to the remote drive configuration attachment in msv store
	RFile fileHandle;
    ResolveFileL( fileHandle );
    CleanupClosePushL( fileHandle );

	// read stream
    RFileReadStream stream( fileHandle,0 );
    CleanupClosePushL( stream );
	
	MStreamBuf* buf = stream.Source();
    buf->PushL();
    User::LeaveIfNull(buf);
    TInt bufferSize = buf->SizeL();
    if (bufferSize == 0 || (bufferSize < KMountMessagePrefixLength))
        {
        User::Leave(KErrMsgBioMessageNotValid);
        }
    TRequestStatus status;    
	HBufC8* utf8configuration = HBufC8::NewMaxLC(bufferSize);
	TPtr8 utfPtr = utf8configuration->Des();
	buf->ReadL(utfPtr, status);
	User::WaitForRequest(status);
	
	HBufC* unistring(NULL); 
	unistring = CnvUtfConverter::ConvertToUnicodeFromUtf7L(*utf8configuration);
	CleanupStack::PushL(unistring);
	
	RsfwMountUtils::ImportMountEntryL(*unistring,&iMountEntry);
								 		
	iViewer = new (ELeave) CRichBio(ERichBioModeEditorBase);
	
	FillViewerWithDataL();
           
	CleanupStack::PopAndDestroy(5,&fileHandle); 						 	
	}

// ---------------------------------------------------------------------------
// Formats displaying a remote drive (only the drive name is shown)
// ---------------------------------------------------------------------------
//
void CRsfwMountConfBioControl::FillViewerWithDataL()
    {
    DEBUGSTRING16(("CRsfwMountConfBioControl::FillViewerWithDataL"));
   	AddItemL(R_STR_SETTING_ITEM_DRIVE_NAME, *(*iMountEntry).Item(EMountEntryItemName));
    }
	
	
// ---------------------------------------------------------------------------
// Display a single item
// ---------------------------------------------------------------------------
//	
void CRsfwMountConfBioControl::AddItemL(TInt aLabelRes, const TDesC& aValue)
    {
    DEBUGSTRING16(("CRsfwMountConfBioControl::AddItemL"));	
    // Empty fields are not shown.
    if (&aValue) 
    	{
    	if (aValue.Length())
            {
            HBufC* labelTxt = StringLoader::LoadLC(aLabelRes, iCoeEnv);
            iViewer->AddItemL(*labelTxt, aValue);
            CleanupStack::PopAndDestroy(labelTxt);
            }
    	}
    }
    

void CRsfwMountConfBioControl::SetAndGetSizeL(TSize& aSize)
	{
	DEBUGSTRING16(("CRsfwMountConfBioControl::SetAndGetSizeL"));		
	if(iIsFileBased)
    	{
    	SetPosition(TPoint(0,KMConfBcHeightReductionBva));
    	aSize.iHeight -= KMConfBcHeightReductionBva;
    	iViewer->SetAndGetSizeL(aSize);
    	}
    else
    	{
    	iViewer->SetAndGetSizeL(aSize);
    	}
    SetSizeWithoutNotification(aSize);
	}

// ---------------------------------------------------------------------------
// Adds Save command to "Options" set
// ---------------------------------------------------------------------------
//
void CRsfwMountConfBioControl::SetMenuCommandSetL(CEikMenuPane& aMenuPane)
	{
	DEBUGSTRING16(("CRsfwMountConfBioControl::SetMenuCommandSetL"));			
	if (!IsEditor())
        {
   	    if( iIsFileBased )
            {
            FileBasedAddMenuItemL(aMenuPane, R_SM_SAVE,
                EMountConfBcCmdSave);
            }
        else
            {
            AddMenuItemL(aMenuPane, R_SM_SAVE,
                EMountConfBcCmdSave, EFirstMenuItem);
                             
            }
        }
	}


void CRsfwMountConfBioControl::FileBasedAddMenuItemL(CEikMenuPane& aMenuPane,
    TInt aStringRes, TInt aCommandOffset)
    {
		DEBUGSTRING16(("CRsfwMountConfBioControl::FileBasedAddMenuItemL"));		   	
    CEikMenuPaneItem::SData menuItem;
    menuItem.iCascadeId = NULL;
    menuItem.iFlags = NULL;
    HBufC* string = StringLoader::LoadL(aStringRes, iCoeEnv);
    menuItem.iText.Format(*string);
    delete string;
    menuItem.iCommandId = iBioControlObserver.FirstFreeCommand()
        + aCommandOffset;
    aMenuPane.InsertMenuItemL(menuItem, 0);
    }

TRect CRsfwMountConfBioControl::CurrentLineRect() const
	{
		DEBUGSTRING16(("CRsfwMountConfBioControl::CurrentLineRect"));			
  	return iViewer->CurrentLineRect();
	}

TBool CRsfwMountConfBioControl::IsFocusChangePossible(
		TMsgFocusDirection aDirection) const
	{
	DEBUGSTRING16(("CRsfwMountConfBioControl::IsFocusChangePossible"));		
   	if (aDirection == EMsgFocusUp)
        {
        return iViewer->IsCursorLocation(EMsgTop);
        }
    return EFalse;
	}

// ---------------------------------------------------------------------------
// Sets the header or the remote drive view
// ---------------------------------------------------------------------------
//
HBufC* CRsfwMountConfBioControl::HeaderTextL() const
	{
	DEBUGSTRING16(("CRsfwMountConfBioControl::HeaderTextL"));				
	return StringLoader::LoadL(R_SM_TITLE_MOUNT_CONF, iCoeEnv);
	}


// ---------------------------------------------------------------------------
// Handles the command selected by the user
// ---------------------------------------------------------------------------
//
TBool CRsfwMountConfBioControl::HandleBioCommandL(TInt aCommand)
	{
	DEBUGSTRING16(("CRsfwMountConfBioControl::HandleBioCommandL %d", aCommand));
	
	aCommand -= iBioControlObserver.FirstFreeCommand();
	switch (aCommand)
		{
	case EMountConfBcCmdSave:
    	TRAPD(err, DoMountL(iMountEntry));	
        if (!err) 
	        {
		    HBufC* buf = StringLoader::LoadLC(R_SM_SUCCESS_MOUNT);
            CAknConfirmationNote* note = new (ELeave) CAknConfirmationNote;
            note->ExecuteLD(*buf);
	        CleanupStack::PopAndDestroy(buf);	
	        }
	    else if (KErrInUse == err) 
	        {
	        HBufC* buf = StringLoader::LoadLC(R_SM_TOO_MANY_REMOTE_DRIVES);
            CAknErrorNote* note = new (ELeave) CAknErrorNote;
            note->ExecuteLD(*buf);
	        CleanupStack::PopAndDestroy(buf);	
	        }
	    else if (!(KErrCancel == err)) 
	        {
	        // cancel means that user does not want to rename
	        User::Leave(err);
	        }
	    
	    return ETrue;
	default:
		return EFalse;
		}
	}
	
TKeyResponse CRsfwMountConfBioControl::OfferKeyEventL(
		const TKeyEvent& aKeyEvent,
		TEventCode aType)
	{
DEBUGSTRING16(("CRsfwMountConfBioControl::OfferKeyEventL"));		
	return iViewer->OfferKeyEventL(aKeyEvent, aType);
	}

TInt CRsfwMountConfBioControl::CountComponentControls() const
	{
DEBUGSTRING16(("CRsfwMountConfBioControl::CountComponentControls"));			
 	return 1; // the viewer component
	}

CCoeControl* CRsfwMountConfBioControl::ComponentControl(TInt aIndex) const
	{
	DEBUGSTRING16(("CRsfwMountConfBioControl::ComponentControl"));		
    if (aIndex == 0)
        {
        return iViewer;
        }
    return NULL;

	}

void CRsfwMountConfBioControl::SizeChanged()
	{
	DEBUGSTRING16(("CRsfwMountConfBioControl::SizeChanged"));		
	iViewer->SetExtent(Position(), iViewer->Size());
	}

void CRsfwMountConfBioControl::FocusChanged(TDrawNow /* aDrawNow */)
	{
	DEBUGSTRING16(("CRsfwMountConfBioControl::FocusChanged"));				
 	iViewer->SetFocus(IsFocused());
	}

void CRsfwMountConfBioControl::SetContainerWindowL(const CCoeControl& aContainer)
	{
		DEBUGSTRING16(("CRsfwMountConfBioControl::SetContainerWindowL"));			
	CCoeControl::SetContainerWindowL(aContainer);

    // The reason for creating the viewer control here is that the
    // construction of the viewer requires a parent with a window. So it
    // cannot be done in ConstructL().
    //
    iViewer->ConstructL(this);

	}
	
TInt CRsfwMountConfBioControl::VirtualHeight()
	{
		DEBUGSTRING16(("CRsfwMountConfBioControl::VirtualHeight"));			
 	return iViewer->VirtualHeight();
	}

TInt CRsfwMountConfBioControl::VirtualVisibleTop()
	{
		DEBUGSTRING16(("CRsfwMountConfBioControl::VirtualVisibleTop"));				
	return iViewer->VirtualVisibleTop();
	}

TBool CRsfwMountConfBioControl::IsCursorLocation(TMsgCursorLocation aLocation) const
    {
 		DEBUGSTRING16(("CRsfwMountConfBioControl::IsCursorLocation"));			   	
    return iViewer->IsCursorLocation(aLocation);
    }


void CRsfwMountConfBioControl::ResolveFileL( RFile& aFile )
    {
  		DEBUGSTRING16(("CRsfwMountConfBioControl::ResolveFileL"));	   	
    if ( iIsFileBased )
        {
		aFile.Duplicate(FileHandle());
        }
    else
        {
        CMsvEntry* entry = MsvSession().GetEntryL( iId );

        CleanupStack::PushL( entry );
		CMsvStore* store = entry->ReadStoreL();
		CleanupStack::PushL(store);
		MMsvAttachmentManager& attachMan = store->AttachmentManagerL();
		aFile = attachMan.GetAttachmentFileL( 0 ); //entry is the first attachment
		CleanupStack::PopAndDestroy( 2, entry ); // store, entry
        }
    }

// ---------------------------------------------------------------------------
// Saves the selected remote drive to the Central Repository table
// ---------------------------------------------------------------------------
//
void CRsfwMountConfBioControl::DoMountL(CRsfwMountEntry* aMounterEntry)
	{
  		DEBUGSTRING16(("CRsfwMountConfBioControl::DoMountL"));	   		
	// CRsfwMountEntry API ensures that EMountEntryItemName
	// length does not exceed KMaxFriendlyNameLength characters	
	TBuf<KMaxFriendlyNameLength>  newFriendlyName;
    newFriendlyName.Copy(*aMounterEntry->Item(EMountEntryItemName));
	
    TBool nameUnique = EFalse;
    TBool operationCancelled = EFalse;
    
    nameUnique = isNameUniqueL(newFriendlyName);
    
    while ((!nameUnique) && (!operationCancelled)) 
        {
        operationCancelled = GetNameForNewMountL(newFriendlyName);
        nameUnique = isNameUniqueL(newFriendlyName);
        }
    
    if (!operationCancelled) 
        {
        // clone the entry and add it to the cenrep
        // we clone it so that the original smart message content does not change
        CRsfwMountEntry* entrytoBeSaved = aMounterEntry->CloneL();
        
        // copy newFriendlyName to name
        entrytoBeSaved->SetItemL(EMountEntryItemName, newFriendlyName);
        
        // set mountentryindex to -1 so that this will go the end of the list
        TBuf<5> index;
   	    index.Num(-1);
   	    entrytoBeSaved->SetItemL(EMountEntryItemIndex, index);	
   	    
   	    // ownership is transferred to MountMan
	    iMountMan->AddMountEntryL(entrytoBeSaved);
        }
    else 
        {
        User::Leave(KErrCancel);
        }

	}
	
// ---------------------------------------------------------------------------
// Tests whether the chosen remote drive name is unique
// ---------------------------------------------------------------------------
//
TBool CRsfwMountConfBioControl::isNameUniqueL(const TDesC& aName)
    {
  		DEBUGSTRING16(("CRsfwMountConfBioControl::isNameUniqueL"));	    	
    // figure out whether a drive with the same name already exists
	CDesCArray* driveArray = new (ELeave) CDesC16ArraySeg(4);
	CleanupStack::PushL(driveArray);
	iMountMan->GetMountNamesL(driveArray); 
	// report error if there are already 9 remote drives
	if (driveArray->Count() == KMaxRemoteDrives) 
	    {
	    CleanupStack::PopAndDestroy(driveArray);
	    User::Leave(KErrInUse);
	    }
	                       
    for (int i = 0; i < driveArray->Count(); i++) 
        {
        if (aName == driveArray->MdcaPoint(i)) 
            {
            // there was a match and the name is not unique
            CleanupStack::PopAndDestroy(driveArray);
            return EFalse;
            }
        }
    CleanupStack::PopAndDestroy(driveArray);    
    return ETrue;    
    }

// ---------------------------------------------------------------------------
// Queries new remote drive name from the user
// ---------------------------------------------------------------------------
//
TBool CRsfwMountConfBioControl::GetNameForNewMountL(TDes& aName)	
    {
  		DEBUGSTRING16(("CRsfwMountConfBioControl::GetNameForNewMountL"));	    	
    TBool operationCancelled = EFalse; 
    // ask user to change the name and try again
    HBufC* confmess = StringLoader::LoadLC(R_STR_NAME_RENAME_QUERY, aName);
    CAknQueryDialog* query = CAknQueryDialog::NewL
                                        (CAknQueryDialog::EConfirmationTone);
     
	if (query->ExecuteLD( R_CONFIRMATION_QUERY, *confmess)) 
	    {
	    TBuf<KMaxFriendlyNameLength> newName;
	    TBool retval;
	    
	    // for getting the new name, use name dialog from the "global save as dialog"
	    HBufC* defaultname = StringLoader::LoadLC(R_SM_DEFAULT_DRIVE_NAME);
         
        CRsfwNotPluginNameDialog* dlg = 
                CRsfwNotPluginNameDialog::NewL( 
               *defaultname, newName, iEikonEnv->FsSession());
        dlg->SetMaxLength(KMaxFriendlyNameLength);
        dlg->PrepareLC( R_FILE_NAME_QUERY );
        retval = dlg->RunLD();
        CleanupStack::PopAndDestroy(defaultname);
        if (!retval) 
            {
            // user cancelled renaming
            operationCancelled = ETrue;
            }
         else 
            {
            aName = newName;
            }
	    }
	else 
	    {
	    // user does not want to rename
	    operationCancelled = ETrue;
	    }
    CleanupStack::PopAndDestroy(confmess);      
	return operationCancelled;    
    }
