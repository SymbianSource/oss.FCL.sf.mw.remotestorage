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
* Description:  Remote drive configuration BIO control
*
*/


#ifndef CRSFWMOUNTCONFBIOCONTROL_H
#define CRSFWMOUNTCONFBIOCONTROL_H

// INCLUDES
#include <msgbiocontrol.h>			 // CMsgBioControl


// FORWARD DECLARATIONS
class CRichBio;
class CRsfwMountMan;
class CRsfwMountEntry;

// CLASS DECLARATION

/**
 * Bio control for Remote drive settings
 */
NONSHARABLE_CLASS(CRsfwMountConfBioControl) : 
	public CMsgBioControl
	{
	public:
	 /**
      * Two-phased constructor
      * @param aObserver Reference to the Bio control observer.
      * @param aSession Reference to Message Server session.
      * @param aId Id of the message.
      * @param aEditorOrViewerMode Flags the new Bio control as editor or viewer.
      * @param aFile filehandle.         
      * @return The newly created object.
      */
	IMPORT_C static CMsgBioControl* NewL(MMsgBioControlObserver& aObserver,
										 CMsvSession* aSession,
										 TMsvId aId,
										 TMsgBioMode aEditorOrViewerMode,	
										 const RFile* aFile);
	 // Destructor									 
	~CRsfwMountConfBioControl();

public: //from MMsgBioControl

        /**
         * Calculates and sets size for a Bio control according to aSize.
         * @param aSize A reference to the suggested size and new size.
         */
        void SetAndGetSizeL(TSize& aSize);
        
         /**
         * Adds a menu command.
         * @param aMenuPane Reference to the CEikMenuPane of the application.
         */
        void SetMenuCommandSetL(CEikMenuPane& aMenuPane);       

        /*
         * The command handler of the bio control.
         * The commands usually originate from a bio specific menu item being
         * selected by the user.
         * @param aCommand Id of command to be handled.
         */
        TBool HandleBioCommandL(TInt aCommand);

        /**
        * Returns a rectangle slice of the bio controls viewing area.
        * It is used by the CMsgEditorView class for scrolling the screen.
        * @return TRect
        */
        TRect CurrentLineRect() const;

        /**
         * This is used by the body container for managing focus and
         * scrolling.
         * @param aDirection The direction to be checked.
         */
        TBool IsFocusChangePossible(TMsgFocusDirection aDirection) const;
        

        /**
         * Returns the header text.
         * @return The header text.
         */
        HBufC* HeaderTextL(void) const;

        /**
         * Gives the height of the text in pixels.
         * It is used by the scrolling framework of Editor Base.
         * @return Height of the text in pixels.
         */
        TInt VirtualHeight();

        /**
         * Gives the cursor position in pixels.
         * It is used by the scrolling framework of Editor Base.
         * @return Cursor position in pixels.
         */
        TInt VirtualVisibleTop();

       /* Tells whether the cursor is in the topmost or bottom position.
         * It is used by the scrolling framework.
         * @param aLocation Specifies either top or bottom.
         * @return ETrue if the cursor is in the part specified by aLocation.
         */
        TBool IsCursorLocation(TMsgCursorLocation aLocation) const;

	
	// Functions from CCoeControl
	TKeyResponse OfferKeyEventL(
		const TKeyEvent& aKeyEvent,
		TEventCode aType);
	
protected:
	// Functions from CCoeControl
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;
	void SizeChanged();
	void FocusChanged(TDrawNow aDrawNow);
	void SetContainerWindowL(const CCoeControl& aContainer);
	
private:
	CRsfwMountConfBioControl(
		MMsgBioControlObserver& aObserver,
		CMsvSession* aSession,
		TMsvId aId,
		TMsgBioMode aEditorOrViewerMode,
		const RFile* aFile);
	void ConstructL();

	void DoMountL(CRsfwMountEntry*);
	void EncodeMounterEntry(CRsfwMountEntry&, TDes&);
	TBool isNameUniqueL(const TDesC& aFriendlyName);
	
	/* returns true when user has selected an unique name for the new drive
	 *
	 */
    TBool GetNameForNewMountL(TDes& aName);
	
	/**
      * Fills the viewer with data from the mount entry
      */
    void FillViewerWithDataL();

	/**
      * Helper for adding to viewer.
      * @param aLabelRes Resource id for label
      * @param aValue Value text
      */
	void AddItemL(TInt aLabelRes, const TDesC& aValue);

	/**
      * Resolve the file handle of the data file that is used as input.
      * @param aFile A reference that gets the file handle.
      */
    void ResolveFileL( RFile& aFile );
    
    /**
      * This is needed because the menuPane adding is done in a different
      * way in BVA than in SMS Editor/Viewer.
      * @param aMenuPane Reference to the menu pane.
      * @param aStringRes String resource ID of the command text.
      * @param aCommandOffset Offset in the Options list.
      */
    void FileBasedAddMenuItemL(CEikMenuPane& aMenuPane,
            TInt aStringRes, TInt aCommandOffset);

private:
    CRichBio* iViewer; 	// The viewer control
    
	TBool iIsFileBased;	// ETrue if BIO Control is lauched through BVA
    
    CRsfwMountEntry* iMountEntry;  // Contains the imported mount entry
   					 		   // only one because the list in GS is not markable
	CRsfwMountMan* iMountMan; // mount manager
	};

#endif // CRSFWMOUNTCONFBIOCONTROL_H
