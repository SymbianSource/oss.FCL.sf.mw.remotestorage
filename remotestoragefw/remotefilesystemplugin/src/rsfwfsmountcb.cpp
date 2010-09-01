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
* Description:  File server interface class representing a mount.
*                An instance of this object is referred to as
*                a mount control block.
*
*/




#include "rsfwfsmountcb.h"
#include "rsfwfsfilecb.h"
#include "rsfwfsdircb.h"


// there is no good way to give remote storage size so just put some big numbers here
const TInt KMountReportedSize         = 999999999;
const TInt KMountReportedFreeSize     = 999999999;

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::NewL
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CRsfwFsMountCB* CRsfwFsMountCB::NewL()
    {
    CRsfwFsMountCB* remoteFsMntCB = new(ELeave) CRsfwFsMountCB();
    return remoteFsMntCB;
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::CRsfwFsMountCB
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CRsfwFsMountCB::CRsfwFsMountCB()
    {
   
    }

// Destructor
CRsfwFsMountCB::~CRsfwFsMountCB()
    {   
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::MountL
// The function should set the volume name (iVolumeName), the unique ID 
// (iUniqueID) and the volume size (iSize) by reading and processing the current 
// mount. The function should leave, on error detection, with an appropriate error 
// code. When aForceMount is set to ETrue, the properties of a corrupt volume 
// should be forcibly stored. The classic case of when this is desirable is when a 
// corrupt volume needs to be formatted.
//
// We set the values (iSize to a fixed size). We also create a
// File Server session (needed to access the local cache) and put the handle into 
// the TLS for future access. aForceMount is ignored.
// Note that this operation does not attempt to connect to remote server.
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::MountL(
    TBool /* aForceMount */)
    {
    
    iServerName = HBufC::NewL( KMaxVolumeNameLength);
    TPtr serverName (iServerName->Des());
    serverName.Append(KRemoteVolumeName);
    SetVolumeName(iServerName); 
       
    TTime timeID;
    timeID.HomeTime();
    iUniqueID = I64LOW(timeID.Int64());
    iSize = KMountReportedSize;
    
    User::LeaveIfError(iFsSession.Connect());
    User::LeaveIfError(Dll::SetTls(&iFsSession));
    
    iRootFid.iVolumeId = Drive().DriveNumber();
    iRootFid.iNodeId = 1;
    }


// -----------------------------------------------------------------------------
// CRsfwFsMountCB::ReMount
// The function should check whether the mount control block represents the 
// current mount on the associated drive. The function should read mount 
// information from the current volume, and check it against the mount 
// information from this mount - typically iVolumeName and iUniqueID. If the mount 
// information matches, the function should return KErrNone, otherwise it should 
// return KErrGeneral.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
int CRsfwFsMountCB::ReMount()
    {
    // we assume that this operation - if called on remote drive
    // - can be succesfull
    return KErrNone;
    
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::Dismounted
// Carries out clean-up necessary for a volume dismount:
// Closing the session to the File Server used for local cache.
// Closing the session to Remote File Engine.
// Dismounting a volume will always succeed, so the function does not need to 
// return an error value. 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::Dismounted()
    {
      
    iSession->Close();  
    iFsSession.Close();
    
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::VolumeL
// Gets volume information. The only information that the function has to supply 
// is the free space, TVolumeInfo::iFree, since the remaining members have already 
// been set by the calling function. The function should leave, on error detection, 
// with an appropriate error code.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::VolumeL(
    TVolumeInfo& aVolume) const
    {
      
    // : we should return info about free storage
    // at the remote server, if possible...
    aVolume.iFree = KMountReportedFreeSize;
    
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::SetVolumeL
// Sets the volume name for the mount, thus writing the new volume name to the 
// corresponding volume.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::SetVolumeL(
    TDes& aName)
    {   
      
    if (aName.Length() >  KMaxVolumeNameLength) 
        {
        User::Leave(KErrBadName);
        }
    if (iServerName) 
        {
        delete iServerName;
        iServerName = NULL;
        }
    iServerName = HBufC::NewL( KMaxVolumeNameLength);
    TPtr serverName (iServerName->Des());
    serverName.Append(aName);
    SetVolumeName(iServerName);
    
    }
    
// -----------------------------------------------------------------------------
// CRsfwFsMountCB::MkDirL
// Creates a new directory on the mount by figuring out the FID of the parent
// directory and the name of the child to be created.
//
// The full name in aName is in the form:
//    \\dirA\\dirB\\dirC\\dirD  
// where dirD is the new directory to be created in \\dirA\\dirB\\dirC\\.
// This means that dirC is the leaf directory in which dirD will be created.
//    
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::MkDirL(
    const TDesC& aName)
    {
      
    TInt delimiterPos;
    TPtrC directory, path;
    TFid theFid;
  
    delimiterPos = aName.LocateReverse(KPathDelimiter);
    directory.Set(aName.Right(aName.Length() - (delimiterPos + 1)));
    path.Set(aName.Left(delimiterPos + 1)); 
    theFid = FetchFidL(path, KNodeTypeDir);
    User::LeaveIfError(RSessionL()->MakeDirectory(theFid, directory));
       
    }


// -----------------------------------------------------------------------------
// CRsfwFsMountCB::RmDirL
// Removes the directory specified by aName by figuring out the FID of the parent
// directory and the name of the child to be created. The function can assume 
// that the directory exists and is not read-only. 
//
// The directory specified by aName is in the form:
//    \\dirA\\dirB\\dirC\\dirD  
// where dirD is the directory to be removed from \\dirA\\dirB\\dirC\\.
// This means that dirC is the leaf directory from which dirD should be removed.
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::RmDirL(
    const TDesC& aName)
    {
     
    TInt delimiterPos;
    TPtrC directory, path;
    TFid parentFid;
        
    delimiterPos = aName.LocateReverse(KPathDelimiter);
    directory.Set(aName.Right(aName.Length() - (delimiterPos + 1)));
    path.Set(aName.Left(delimiterPos + 1)); 
    parentFid = FetchFidL(path, KNodeTypeDir);
    User::LeaveIfError(RSessionL()->RemoveDirectory(parentFid, directory));
        
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::DeleteL
// Deletes the specified file from the mount by figuring out the FID of the parent
// directory and the name of the child to be created. The function can assume that 
// the file is closed.
//
//  The file name specified by aName is of the form:
//    \\dirA\\dirB\\dirC\\file.ext    
//  The extension is optional.
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//

void CRsfwFsMountCB::DeleteL(
    const TDesC& aName)
    {  
 
    TInt delimiterPos;
    TPtrC file, path;
    TFid parentFid;
    
    delimiterPos = aName.LocateReverse(KPathDelimiter);
    file.Set(aName.Right(aName.Length() - (delimiterPos + 1)));
    path.Set(aName.Left(delimiterPos + 1)); 
    parentFid = FetchFidL(path, KNodeTypeDir);
    User::LeaveIfError(RSessionL()->RemoveFile(parentFid, file));      
    
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::RenameL
// Renames or moves a single file or directory on the mount by figuring out
// the FIDs of the parent directories and child names for both anOldName and 
// anNewName.  If oldEntryName is a file, it can be assumed that it is closed.
// If oldEntryName is a directory, it can be assumed that there are no
// open files in this directory. Furthermore, if newEntryName specifies
// a directory, it can be assumed that it is not a subdirectory of oldEntryName.
//
// anOldName and anNewName specify the respective entries with full names; 
// for example,
// \\dirA\\dirB\\dirC\\oldEntryName
// and
// \\dirE\\dirF\\dirG\\newEntryName
//   
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::RenameL(
    const TDesC& anOldName, 
    const TDesC& aNewName)
    {
      
    TInt delimiterPos;
    TPtrC sourcepath, destpath;
    TPtrC srcname, destname;
    TFid sourceFid, destFid;

    delimiterPos = anOldName.LocateReverse(KPathDelimiter);
    srcname.Set(anOldName.Right(anOldName.Length() - (delimiterPos + 1)));  
    sourcepath.Set(anOldName.Left(delimiterPos + 1));   
    sourceFid = FetchFidL(sourcepath, KNodeTypeDir);

    delimiterPos = aNewName.LocateReverse(KPathDelimiter);
    destname.Set(aNewName.Right(aNewName.Length() - (delimiterPos + 1)));
    destpath.Set(aNewName.Left(delimiterPos + 1));  
    destFid = FetchFidL(destpath, KNodeTypeDir);

    User::LeaveIfError(RSessionL()->MoveFids(sourceFid, 
        										  srcname, 
        										  destFid, 
        										  destname, 
        										  EFalse));    
        
 
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::ReplaceL
// Replaces one file on the mount with another. The file anOldName should have 
// its contents, attributes, and the universal date and time of its last 
// modification, copied to the file aNewName, overwriting any existing contents 
// and attribute details. If the file aNewName does not exist it should be created.
// The function can assume that both anOldName and, if it exists, anNewName
// contain the full file names of files, and that these files are not open.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::ReplaceL(
    const TDesC&  anOldName, 
    const TDesC& aNewName )
    {
   
    TInt delimiterPos;
    TPtrC sourcepath, destpath;
    TPtrC srcname, destname;
    TFid sourceFid, destFid;

    delimiterPos = anOldName.LocateReverse(KPathDelimiter);
    srcname.Set(anOldName.Right(anOldName.Length() - (delimiterPos + 1)));
    sourcepath.Set(anOldName.Left(delimiterPos + 1));   
    sourceFid = FetchFidL(sourcepath, KNodeTypeDir);

    delimiterPos = aNewName.LocateReverse(KPathDelimiter);
    destname.Set(aNewName.Right(aNewName.Length() - (delimiterPos + 1)));
    destpath.Set(aNewName.Left(delimiterPos + 1));  
    destFid = FetchFidL(destpath, KNodeTypeDir);
        
    User::LeaveIfError(RSessionL()->MoveFids(sourceFid, 
        										  srcname, 
        										  destFid, 
        										  destname, 
        										  ETrue)); 
    
   
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::EntryL
// Gets the entry details for the specified file or directory.
// This function is defined as a const function in the base class CMountCB.
// However, we need to modify the shared memory chunks used in the parameter 
// passing. That's why we need  to cast away const.
//
// Always returns KErrPathNotFoud for certain Symbian system directories. The 
// reason for this is that scanning all drives for some system directory does
// not always skip remote drives, but it is not feasible to start to look
// for some library, recognizer etc. from a remote drive.
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::EntryL(
    const TDesC& aName, 
    TEntry& anEntry) const
    {
    
    if (aName.Length() > KMaxPath) 
    	{
    	User::Leave(KErrBadName);
    	}
        
    CONST_CAST(CRsfwFsMountCB*, this)->RemoteFsEntryL(aName, anEntry);
    
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::RemoteFsEntryL
// Gets the entry details for the specified file or directory by figuring out
// the fid of the entry.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::RemoteFsEntryL(const TDesC& aName, TEntry& anEntry)
    {
      
    anEntry.iName = aName;
    TFid fileFid = FetchFidL(aName, KNodeTypeUnknown); 
    User::LeaveIfError(RSessionL()->GetAttributes(fileFid, anEntry));
      
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::SetEntryL
// Sets entry details for a specified file or directory. 
// The entry identified by the full name descriptor aName should have
// its modification time and its attributes mask updated as required.
// We also use this function to control (using new attributes bits) the file 
// caching state (KEntryAttCachePriorityHigh)
// 
// The entry receives a new universal modified time from aTime.
// The entry attributes are set with aSetAttMask and cleared with aClearAttMask:
// the bits that are set in aSetAttMask should be set in the entry attribute mask;
// the bits that are set in aClearAttMask should be cleared from the entry 
// attribute mask.
//
// The function can assume that aSetAttMask and aClearAttMask do not change
// the type of attribute (i.e. volume or directory). Furthermore, if aName
// specifies a file, it can be assumed that this file is closed.
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::SetEntryL(
    const TDesC& aName,
    const TTime& aTime,
    TUint aSetAttMask,
    TUint aClearAttMask)
    {
        
    TFid thisFid;
    thisFid = FetchFidL(aName, KNodeTypeUnknown);
    
    TInt result = RSessionL()->SetEntry(thisFid, aTime, aSetAttMask, aClearAttMask);
    
    // KErrNotSupported is dismissed currently
    // The reason for this is CFileMan::Copy
    // CFileMan::Copy is a composite operation which as a last step
    // calls RFs::SetAtt() for the target file.
    // If we honestly return KErrNotSupported CFileMan::Copy will now
    // return KErrNotSupported also, although it already copied the file on the server.
    // We rather have a working copy and return false information about the success of 
    // SetAtt().
    if ((result != KErrNone) && (result != KErrNotSupported))
        {
        User::Leave(result);
        }
    
    }


// -----------------------------------------------------------------------------
// CRsfwFsMountCB::FileOpenL
// If needed creates a new file by figuring out the FID of the parent directory 
// and the name of the new file. After that opens the file. After successful 
// completion of the function, the file control block pointer will be added to the 
// file server's global files container.  Adds information to the file control 
// block (e.g. path of the local cache file and pointer to this mount control block)
//
// If anOpen specifies EFileReplace (rather than EFileCreate or EFileOpen) then
// the data contained in the file should be discarded, the archive attribute 
// should be set, and the size of the file should be set to zero. Note that it can 
// be assumed that if anOpen specifies EFileReplace then the file already exists.
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::FileOpenL(
    const TDesC& aName,
    TUint aMode,
    TFileOpen anOpen,
    CFileCB* aFile)
    {

    TFid parentFid;
    TFid thisFid;
    TInt delimiterPos;
    TPtrC filename, path;
    delimiterPos = aName.LocateReverse(KPathDelimiter);
    filename.Set(aName.Right(aName.Length() - (delimiterPos + 1)));
    path.Set(aName.Left(delimiterPos + 1)); 
    parentFid = FetchFidL(path, KNodeTypeDir);
       
    switch (anOpen)
        {
    case EFileCreate:
        // exclusive = 1 
        User::LeaveIfError(RSessionL()->
      		CreateFile(parentFid, filename, aMode, TRUE, thisFid));
        break;
    case EFileReplace: 
        // exclusive = 0 
        User::LeaveIfError(RSessionL()->
        	CreateFile(parentFid, filename, aMode, FALSE, thisFid));
        break;
    case EFileOpen: // the file must exist
    default:
        User::LeaveIfError(RSessionL()->
			Lookup(parentFid, filename, KNodeTypeFile, thisFid));
        break;
        } 
    
    TDirEntAttr attributes;
    HBufC* unicodepath = HBufC::NewMaxLC(KMaxPath);
    TPtr unicodepathptr(unicodepath->Des());
    attributes.iAtt = aMode;
    User::LeaveIfError(RSessionL()->
    				   OpenByPath(thisFid, unicodepathptr, &attributes, ETrue));
    unicodepathptr.SetLength(unicodepath->Length());
    
    ((CRsfwFsFileCB*)aFile)->iLastFlushFailed = EFalse;
    ((CRsfwFsFileCB*)aFile)->iThisFid = thisFid;
    ((CRsfwFsFileCB*)aFile)->iParentFid = parentFid;
    ((CRsfwFsFileCB*)aFile)->SetContainerFileL(unicodepathptr);
  	// set size, att, modified for the file
  	((CRsfwFsFileCB*)aFile)->SetSize(attributes.iSize);
  	((CRsfwFsFileCB*)aFile)->SetAtt(attributes.iAtt);
  	((CRsfwFsFileCB*)aFile)->SetModified(attributes.iModified);
    aFile->SetMount(this);
    CleanupStack::PopAndDestroy(unicodepath); // unicodepath
    
    }
 
// -----------------------------------------------------------------------------
// CRsfwFsMountCB::DirOpenL
// Opens a directory on the mount by figuring out the FID of the parent 
// directory and the name of the directory. 
// Note that aName is of the form
//  \\dirA\\dirB\\dirC\\file.ext
// where \\dirA\\dirB\\dirC\\ is the directory to be opened and file.ext is
// an optional entry name and extension. The name and extension (e.g. "*" or 
// "*.txt") limit entries that reading the directory should return.
// 
// Always returns KErrPathNotFoud for certain Symbian system directories. The 
// reason for this is that scanning all drives for some system directory does
// not always skip remote drives, but it is not feasible to start to look
// for some library, recognizer etc. from a remote drive.
//
// After successful completion of the function, the directory control block
// pointer will be added to the file server global directories container. Adds 
// information to the directory control block (e.g. path of the local cache file 
// and possible name and extension)
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::DirOpenL(
    const TDesC& aName, 
    CDirCB* aDir)
    {
            
    TFid fileFid;
    TInt namePos;
    namePos = aName.LocateReverse(KPathDelimiter);
    TPtrC directoryPathName(aName.Left(namePos + 1));
    TPtrC matchName(aName.Mid(namePos + 1));
    if (matchName.Length() == 0)
        {
        matchName.Set(KDirReadAllMask);
        }
    if (directoryPathName.Length() == 0)
        {
        fileFid = iRootFid;
        }
    else
        {
        fileFid = FetchFidL(directoryPathName, KNodeTypeDir);
        }   

    HBufC* unicodepath = HBufC::NewMaxLC(KMaxPath);
    TPtr unicodepathptr(unicodepath->Des());
    User::LeaveIfError(RSessionL()->
    				   OpenByPath(fileFid, unicodepathptr, NULL, EFalse));
    ((CRsfwFsDirCB*)aDir)->SetDirL(unicodepathptr, matchName);    
    ((CRsfwFsDirCB*)aDir)->iThisFid = fileFid;
    ((CRsfwFsDirCB*)aDir)->SetMount(this);
    CleanupStack::PopAndDestroy(unicodepath); // unicopath
   
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::RawReadL
// Should read the specified length of data from the specified position on
// the volume directly into the client thread. It can be assumed that if this 
// function is called, then there has been  a successful mount.
// Not supported, as "position in a volume" without files is meaningless to 
// us.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::RawReadL(
    TInt64 /* aPos */, 
    TInt /* aLength */ , 
    const TAny* /* aTrg */, 
    TInt /* anOffset */ ,
    const RMessagePtr2& /* aMessage */) const 
    {
      
    User::Leave(KErrNotSupported);
    
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::RawWriteL
// Should write a specified length of data from the client thread to the volume
// at the specified position. It can be assumed that if this 
// function is called, then there has been  a successful mount.
// Not supported, as "position in a volume" without files is meaningless to 
// us.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::RawWriteL(
    TInt64 /* aPos */, 
    TInt /* aLength */, 
    const TAny* /* aSrc */, 
    TInt /* anOffset */,
    const RMessagePtr2& /* aMessage */)
    {
      
    User::Leave(KErrNotSupported);
    
    }
    
// -----------------------------------------------------------------------------
// CRsfwFsMountCB::GetShortNameL
// Should get the short name of the file or directory with the given full name.
// This function is used in circumstances where a file system mangles
// Symbian OS natural names, in order to be able to store them on
// a file system that is not entirely compatible.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::GetShortNameL(
    const TDesC& /* aLongName */,
    TDes& /* aShortName */)
    {
         
    User::Leave(KErrNotSupported);
    
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::GetLongNameL
// Should get the long name of the file or directory associated with the given 
// short name. This function is used in circumstances where a file system mangles
// Symbian OS natural names in order to be able to store them on
// a file system that is not entirely compatible. 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::GetLongNameL(
    const TDesC& /* aShortName */,
    TDes& /* aLongName */)
    {
      
    User::Leave(KErrNotSupported);
    
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::ReadSectionL
// Reads a specified section of the file, regardless of the file's lock state.
// This function basically does what the chain of opening a file, creating 
// the file control block, and reading file data using CRsfwFsFile::Read()
// does, but without creating the file control block. For us file's lock state 
// does not have meaning.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::ReadSectionL(
    const TDesC& aName,
    TInt aPos,
    TAny* /*aTrg */,
    TInt aLength,
    const RMessagePtr2& aMessage)

    {

    HBufC8* data = HBufC8::NewMaxLC(aLength);
    TPtr8 buf(data->Des());   
  
    TFid parentFid;
    TFid newFid;
    TInt delimiterPos;
    TPtrC filename, path;
    HBufC* unicodepath = HBufC::NewMaxLC(KMaxPath);
    TPtr unicodepathptr(unicodepath->Des());
    
    delimiterPos = aName.LocateReverse(KPathDelimiter);
    filename.Set(aName.Right(aName.Length() - (delimiterPos + 1)));
    path.Set(aName.Left(delimiterPos + 1));
    parentFid = FetchFidL(path, KNodeTypeDir);
    User::LeaveIfError(RSessionL()->
    		Lookup(parentFid, filename, KNodeTypeFile, newFid));
    User::LeaveIfError(RSessionL()->
   			OpenByPath(newFid, unicodepathptr, NULL, EFalse));
   			
   	// get the size of the container file	
   	TEntry tentry;
   	User::LeaveIfError((*(RFs* )Dll::Tls()).Entry(unicodepathptr, tentry));
    if (aPos > tentry.iSize) 
        {
        // non-sequential read 
        // i.e. if 128 bytes have been cached 
        // starting read from pos 128 continues to fill the cache file
        // but starting read from pos 129- is "random access"
        // that by-passes the cache */
        RFile tempFile;
        TBool usetempCache = EFalse;
        HBufC* tmpcacheFile = HBufC::NewLC(KMaxPath);
        TPtr tmpCache(tmpcacheFile->Des());
        User::LeaveIfError(RSessionL()->FetchData(newFid, 
        										  aPos, 
        										  aPos + aLength - 1, 
        										  tmpCache, 
        										  usetempCache));
        // if caching mode is "Whole File Caching" this operation may fetch
        // the whole file into normal cache anyway. "tmpCache" contains
        // the right path in this case too, so in this function (where the file
        // is not open), we don't have to check "usetempCache" boolean.									  
        User::LeaveIfError(tempFile.Open(*(RFs* )Dll::Tls(), tmpCache, EFileRead));
        CleanupClosePushL(tempFile);
        User::LeaveIfError(tempFile.Read(buf, aLength));
        CleanupStack::PopAndDestroy(2, tmpcacheFile); // tempfile, tempcacheFile
        }
    else 
        {
        TInt lastByte;
        User::LeaveIfError(RSessionL()->Fetch(newFid, aPos, aPos + aLength-1, lastByte));
        User::LeaveIfError((*(RFs* )Dll::Tls()).ReadFileSection(*unicodepath,
                                                                aPos,
                                                                buf,
                                                                aLength));
        }
    CleanupStack::PopAndDestroy(unicodepath); // unicodepath
   
    // we have read the data into buf
    aMessage.WriteL(0, buf, 0);
    CleanupStack::PopAndDestroy(data); //  data 
    
    }


   
// -----------------------------------------------------------------------------
// CRsfwFsMountCB::FetchFidL
// Fetches fid for an entry.
// aPath is of the form \\dirA\\dirB\\dirC\\entry
// Goes through the path recursively. I.e. first gets the fid of dirA by
// fid_dirA = lookup(iRootFid, dirA), then does lookup(fid_dirA, dirB) etc. 
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TFid CRsfwFsMountCB::FetchFidL(
    const TDesC& aPath, 
    TUint aNodeType)
    {
    TFid newFid;  
    TInt pathlength = aPath.Length();

    if (pathlength <= 1)
        {
        // '\'
        // In some rare cases called with zero length aPath,
        // it seems to be ok to return rootFid
        return iRootFid;
        }
    else
        {
        TInt delimiterPos = aPath.LocateReverse(KPathDelimiter);
        if (delimiterPos == (pathlength - 1))
            {
            // The path ends with a slash,
            //i.e. this is a directory - continue parsing
            TPtrC nextdelimiter;
            nextdelimiter.Set(aPath.Left(delimiterPos));
            delimiterPos = nextdelimiter.LocateReverse(KPathDelimiter);
            }
        TPtrC entry(aPath.Right(aPath.Length() - (delimiterPos + 1)));
        TPtrC path(aPath.Left(delimiterPos + 1));
        // fetch recursively TFid of path 
        User::LeaveIfError(RSessionL()->
				Lookup(FetchFidL(path, KNodeTypeDir), entry, aNodeType, newFid));
        return newFid;
        }
        
    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::RenameFidL
// Renames a file
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsMountCB::RenameFidL( 
    TFid aDirFid, 
    const TDesC& aSourceName,
    const TDesC& aNewName )
    {

    TInt delimiterPos;
    TPtrC destpath, srcname, destname;
    TFid destFid;

    delimiterPos = aSourceName.LocateReverse(KPathDelimiter);
    srcname.Set(aSourceName.Right(aSourceName.Length() - (delimiterPos + 1)));

    delimiterPos = aNewName.LocateReverse(KPathDelimiter);
    destname.Set(aNewName.Right(aNewName.Length() - (delimiterPos + 1)));
    destpath.Set(aNewName.Left(delimiterPos + 1));  
    destFid = FetchFidL(destpath, KNodeTypeDir);

    User::LeaveIfError(RSessionL()->MoveFids(aDirFid, 
    										  srcname, 
    										  destFid, 
    										  destname, 
    										  EFalse));     

    }

// -----------------------------------------------------------------------------
// CRsfwFsMountCB::CheckDisk
// Checks the integrity of the disk on the specified drive
// temporarily return KErrNone as a workaround for CommonDialogs 
// should return KErrNotSupported for remote drives
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
TInt CRsfwFsMountCB::CheckDisk()
    {
    return KErrNone;
    }


// -----------------------------------------------------------------------------
// CRsfwFsMountCB::RSessionL
// Singleton-function that creates a session to Remote File Engine once.
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
RRsfwSession* CRsfwFsMountCB::RSessionL()
    { 
      
    if (!iSession)
        {
        iSession = new (ELeave) RRsfwSession();
        User::LeaveIfError(iSession->Connect());
        }     
    return iSession;
    
    }


// End of File
