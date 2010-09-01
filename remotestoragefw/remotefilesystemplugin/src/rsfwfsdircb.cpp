/*
* Copyright (c) 2003-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description: 	File server interface class representing an open directory.
*                The code in this class allows to access a specific 
*                remote directory.
*
*/


// INCLUDE FILES
#include "rsfwfsdircb.h"
#include "rsfwfsmountcb.h"

// ============================ MEMBER FUNCTIONS ===============================

// static constructor
CRsfwFsDirCB* CRsfwFsDirCB::NewL()
    {
    CRsfwFsDirCB* remoteFsDirCB = new (ELeave) CRsfwFsDirCB;
    return remoteFsDirCB;
    }

// -----------------------------------------------------------------------------
// CRsfwFsDirCB::CRsfwFsDirCB
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CRsfwFsDirCB::CRsfwFsDirCB()
    {
    iHasBeenFetched = EFalse;
    iEntryPos = 0;
    iStreamPos = 0;
    }

// destructor
CRsfwFsDirCB::~CRsfwFsDirCB()
    { 
    delete iMatch;
    iDirContReadStream.Close();   
    }


// -----------------------------------------------------------------------------
// CRsfwFsDirCB::ReadL
// File Server calls this function to retrieve one entry from open directory 
// (next unread). When the last entry has been read, the function leaves with 
// User::Leave(KErrEof). All of the properties of a TEntry, other than the  UID 
// types, are always read.  The time stored in the iModified member of  anEntry 
// should be converted from UTC time to local time. When storing the iName 
// member of anEntry, the current (.), or parent marker (..) in the directory 
// should not be returned.
//
// If the KEntryAttAllowUid flag is set in the iAtt member of anEntry, then
// the entry UID type of an entry will be read. If, on reading the UID from
// a file, KErrCorrupt is generated, because the file is corrupt,
// ReadL() should not leave with this error message, but should return
// as normal.
//
// FILTERING:
// The function should read successive entries until a suitable entry is found.
// An entry is suitable if the entry attributes match the criteria set by this
// object's attributes, which are set on initialisation.  The File Server has 
// set CDirCB::iAtt (which we inherit) with the bitmask.
//
// The File Server opened the directory with a name mask (e.g. "*"). 
// This mask has been to stored to iMask of this class. We must only return entries, 
// whose name match this mask.
//
// If, on return, the entry's full file name, TEntry::iName, is longer than
// the maximum buffer size, then the entry cannot be returned to the client.
// In this case the file server will set iPending to true and will call
// StoreLongEntryName() before calling this function again.
// In this case (when iPending is true), the function should re-read
// the last entry to be read; it should also set iPending to false and
//  should not advance the current read position.
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
// 
void CRsfwFsDirCB::ReadL
(TEntry& anEntry)
    {       
      
    if (!iHasBeenFetched)
        {
        //   First read, the directory hasn't been fetched from the server
        //   yet (can't be done from open as that is always synchronous
        //   operation from UI's point of view)
        TInt totalBytes;
        User::LeaveIfError(static_cast<CRsfwFsMountCB&>
                (Mount()).RSessionL()->Fetch(iThisFid, 0, 0, totalBytes));
        iHasBeenFetched = ETrue;
        }

    TEntry entry;
    
    if (iPending) 
        {
        iStreamPos = iPendingPos;
        iPending = EFalse; 
        }
        
    MStreamBuf* contFileStreamBuf = iDirContReadStream.Source();
    // Skip over entries already read
    contFileStreamBuf->SeekL(MStreamBuf::ERead, iStreamPos);

    TDirEnt d;
       
    do
        {
        TInt strippedBits = 0;
        TBool matchName = EFalse;
   		TBool matchAttr = EFalse;
        
        iPendingPos = iStreamPos;
        // Each round must fill one TEntry (or leave with KErrEof)
        d.InternalizeL(iDirContReadStream); // leaves with KErrEof
        iEntryPos++;
          
        entry.iName.Des().Copy(d.iName);
        entry.iAtt = d.iAttr.iAtt;
        entry.iSize = d.iAttr.iSize;
        entry.iModified = d.iAttr.iModified;
        entry.iType = (KNullUid, KNullUid, d.iAttr.iUid3);
        // do not set KEntryAttRemote for directories
        // it is defined by Symbian to be a file attribute only
        // remote file engine uses it internally for dirs also
        if (entry.iAtt & KEntryAttDir) 
            {
            entry.iAtt &= ~KEntryAttRemote;
            }
             
        // Filtering:
            
        // compare name against the match pattern
        matchName = (entry.iName.Match(*iMatch) != KErrNotFound);
        
        // compare against iAtt attribute mask
        // The mask works as follows: 
        // To match files only, specify KEntryAttNormal. 
        // To match both files and directories, specify KEntryAttDir. 
        // To match directories only, specify KEntryAttDir|KEntryAttMatchExclusive. 
        // To match files with a specific attribute, then OR the attribute involved with KEntryAttMatchExclusive. 
        // For example, to match read-only files, specify KEntryAttReadOnly|KEntryAttMatchExclusive. 
        if (iAtt & KEntryAttMatchExclusive)
        	{ // files with a specific attribute only
        	matchAttr = iAtt & entry.iAtt; 
        	}
        else 
        	{
        	if (iAtt & KEntryAttDir)
       			{
       			// both files and directories
       			matchAttr = ETrue;
       			}
       		else 
       			{
       			// files only
       			if (!(entry.iAtt & KEntryAttDir)) 
       				{
       				matchAttr = ETrue;
       				}
       			}
        	}     	
        if (matchName && matchAttr)
            {
            // "reverse" bits modified to make comparison easier
            entry.iAtt |= strippedBits;
            entry.iAtt &= ~KEntryAttMatchExclusive; 
            anEntry = entry;
            break;
            }
        }
    while (1);

    iStreamPos = contFileStreamBuf->TellL(MStreamBuf::ERead);
        
    }
    
    
// -----------------------------------------------------------------------------
// CRsfwFsDirCB::SetDirL
// Opens read stream in the container file. Called in CRsfwFsMountCB when the 
// directory is opened.
// Implementation notice: As this class only reads the container file, it would
// seem sensible to open  EFileRead|EFileShareAny here, but when Remote 
// File Engine then tries to open EFileWrite|EFileShareAny it fails unless
// EFileWrite is specified here also...
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsDirCB::SetDirL(
    const TDesC& aPath, 
    const TDesC& aName)
    {
      
    User::LeaveIfError(iDirContReadStream.Open(*(RFs* )Dll::Tls(),
                                               aPath, EFileWrite | EFileShareAny));
    iMatch = aName.AllocL();
    
    }

// End of File
