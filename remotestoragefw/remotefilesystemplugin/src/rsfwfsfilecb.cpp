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
* Description:  File server interface class representing an open file.
*                Allows to access a specific remote file.
*
*/


// INCLUDE FILES
#include "rsfwfsfilecb.h"
#include "rsfwfsmountcb.h"

// -----------------------------------------------------------------------------
// CRsfwFsFileCB::CRsfwFsFileCB
// C++ default constructor can NOT contain any code, that
// might leave.
// -----------------------------------------------------------------------------
//
CRsfwFsFileCB::CRsfwFsFileCB()
    {
    iLastFlushFailed = ETrue;
    iPartialWriteSupported = ETrue;
    }
 
// Destructor
CRsfwFsFileCB::~CRsfwFsFileCB()
    {   
    // close the cache file first so that RFE can move/delete it if upload fails
    iContFile.Close(); 
    TUint flags = 0;
    if (iFileName)
        {
        if (!iLastFlushFailed) 
            {
            // Now the container file has been changed,
            // tell Remote File Engine to update it on the servers
            // RSessionL() should not leave here as the remote session surely is created by now...
            if (iAtt & KEntryAttModified) 
                {
                flags |= ECloseModified;
         
                // File was modified use, flush to write data to the server
                // We write the whole file always, if flush was never called we cannot
                // know whether partial write is supported.
                TRAP_IGNORE(static_cast<CRsfwFsMountCB&>
                    (Mount()).RSessionL()->Flush(iThisFid, 0, iCachedSize, iCachedSize));
                }
            else 
                {
                flags |= ECloseNotModified;
                }
                
            }
        else 
            {
            // flush was called and failed
            // do not try to flush again if the application closes the file
            // instead indicate this to the close state machine
            flags |= ECloseLastFlushFailed;
            }
        }
        
     // close will release the write lock if possible
     // and also allows user to save the file locally if flush failed
     TRAP_IGNORE(static_cast<CRsfwFsMountCB&>
              (Mount()).RSessionL()->CloseFile(iThisFid, flags));
    
    }
    
          
// -----------------------------------------------------------------------------
// CRsfwFsFileCB::RenameL
// Renames the file with the full file name provided. Because the full name of 
// the file includes the path, the function can also be used to move the file.
//
// It can be assumed that no other sub-session has access to the file:
// i.e. the file has not been opened in EFileShareAny share mode.
// It can also be assumed that the file has been opened for writing. 
// -----------------------------------------------------------------------------
//
void CRsfwFsFileCB::RenameL(
    const TDesC& aNewName)
    {
   	static_cast<CRsfwFsMountCB&>(Mount()).RenameFidL(iParentFid, *iFileName, aNewName);
   	delete iFileName;
   	iFileName = NULL;
   	iFileName = aNewName.AllocL();
    }

// -----------------------------------------------------------------------------
// CRsfwFsFileCB::ReadL
// Reads a specified number of bytes from the open file starting at
// the specified position, and writes the result into a descriptor.
//    
// It can be assumed that aPos is inside the file and aLength > 0.
// The file should only be read up to its end regardless of
// the value of aPos + aLength. The number of bytes read should be stored
// in aLength on return.
//
// Implemented by sending FETCH request to Remote File Engine for the
// specified data and subsequently reading the data from the local cache file.
// Reading the local cache file sets aLength.
// Note that we want to keept the cache file continuos. If the requested data
// starts behind the end of the current cache file, the function sends FETCHDATA
// and Remote File Engine puts this data into a temp cache file valid only 
// for the duration of this operation if the caching mode is something else than 
// Whole File Caching
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsFileCB::ReadL(
    TInt aPos,
    TInt& aLength,
    const TAny* /*aDes*/,
    const RMessagePtr2& aMessage)

    { 
          
    if (iCachedSize == 0) 
        {
        // iCachedSize possibly not up-to-date...
        iContFile.Size(iCachedSize);
        }
    
    HBufC8* data = HBufC8::NewMaxLC(aLength);
    TPtr8 buf(data->Des());

    if (aPos > iCachedSize) 
        {
        // Depending on the caching mode this type of request may bypass the 
        // normal cache file.
        TBool useTempCache = EFalse;
        HBufC* tmpCacheFile = HBufC::NewLC(KMaxPath);
        TPtr tmpCache(tmpCacheFile->Des());
        User::LeaveIfError(static_cast<CRsfwFsMountCB&>
            (Mount()).RSessionL()->FetchData(iThisFid, 
            								aPos, 
            								aPos + aLength - 1, 
            								tmpCache, 
            								useTempCache));
        if (useTempCache) 
        	{
        	// use "temp" in the same directory instead of the cache file
        	RFile tempFile;
        	TParse parser;
        	parser.Set(tmpCache, NULL, NULL);
        	HBufC* tempPath = HBufC::NewLC(KMaxPath);
            TPtr tempptr = tempPath->Des();
            tempptr.Append(parser.DriveAndPath());
            tempptr.Append(KTempFileName);
        	User::LeaveIfError(tempFile.Open(*(RFs* )Dll::Tls(),
                                         tempptr, EFileRead));
            CleanupStack::PopAndDestroy(tempPath);                       
        	CleanupClosePushL(tempFile);
       		User::LeaveIfError(tempFile.Read(buf, aLength));
       		CleanupStack::PopAndDestroy(&tempFile);
        	}
        else 
        	{
        	// read from the normal container file (Whole File Caching mode).
        	iContFile.Size(iCachedSize);
        	User::LeaveIfError(iContFile.Read(aPos, buf, aLength));
        	}
        CleanupStack::PopAndDestroy(tmpCacheFile); // tempcacheFile
        }
    else if ((aPos + aLength) > iCachedSize)
        {
        User::LeaveIfError(static_cast<CRsfwFsMountCB&>
       		(Mount()).RSessionL()->Fetch(iThisFid, 
       									 aPos, 
       									 aPos + aLength - 1, 
       									 iCachedSize)); 
       									  									 
        User::LeaveIfError(iContFile.Read(aPos, buf, aLength));
        }
    else 
        {
        User::LeaveIfError(iContFile.Read(aPos, buf, aLength));
        }
        
    aMessage.WriteL(0, buf, 0);     
    CleanupStack::PopAndDestroy(data);
    if (iCachedSize == iSize) 
    	{
    	// clear the remote attribute if the whole file has now been fetched
    	iAtt &= ~KEntryAttRemote;
    	}
    }
    
// -----------------------------------------------------------------------------
// CRsfwFsFileCB::WriteL
// Writes data to the open file. iModified and iSize are set by the file server 
// after this function has completed successfully.
//
// It can be assumed that aPos is within the file range and aLength > 0.
// When aPos + aLength is greater than the file size then the file should
// be enlarged using SetSizeL(). The number of bytes written should be
// returned through the argument aLength.
//
// Implemented by writing to the local cache file. First requests Remote File
// Engine whether there is enough space in the cache to write the file (this
// also calls SysUtil::DiskSpaceBelowCritical()). FE attempts to free space 
// from the cache if necessary Implementation notice: writes a large file in 
// chunks of 64K.
//  
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsFileCB::WriteL(
    TInt aPos,
    TInt& aLength,
    const TAny* /*aDes*/,
    const RMessagePtr2& aMessage)
    {
    
    if (iCachedSize == 0) 
        {
        // iCachedSize possibly not up-to-date...
        iContFile.Size(iCachedSize);
        }
    
   // if flush was cancelled, but we come again to write, again set iLastFlushFailed to EFalse
    iLastFlushFailed = EFalse;
    
 	// We must first fetch the file to the local cache
 	// unless aPos = 0 and aLength => iSize 
 	// Note that if files are written to they cannot be partially cached
 	// as the whole file will be sent to the server and overwrites the old file.
 	// That is why we must ensure that the whole file is cached as soon as we
 	// get the first write.
 	// in subsequent writes iCachedSize will equal iSize
 	// This may eventually change if we start to use some kind of "delta-PUT".
 	if (!((aPos == 0) && (aLength >= iSize)) && iCachedSize < iSize && !iFetchedBeforeWriting)
 		{
 		User::LeaveIfError(static_cast<CRsfwFsMountCB&>
 				(Mount()).RSessionL()->Fetch(iThisFid, iCachedSize, iSize-1, iCachedSize));
 		iFetchedBeforeWriting = ETrue;
        }

    // make sure that a potential cache addition still fits into the cache
    TInt sizeToBeWritten = 0;
    
 	if (iSize > iCachedSize)
 	    {
 	    // when current Write is executed as a part of Copy operation
 	    // then iSize has been set to final size, even before any writing started
 	    sizeToBeWritten = iSize - iCachedSize;
 	    }
    else if (aPos + aLength > iCachedSize)
        {
        sizeToBeWritten = aPos + aLength - iCachedSize + iWrittenSize;
        }
             	 
 	TBool okToWrite;
 	User::LeaveIfError(static_cast<CRsfwFsMountCB&> 
 			(Mount()).RSessionL()->OkToWrite(iThisFid, 
 											sizeToBeWritten,
 											okToWrite));
 											 											
    if (!okToWrite) 
        {
        User::Leave(KErrDiskFull);
        }
    
    TInt anOffset = 0;
    HBufC8* data = HBufC8::NewMaxLC(aLength);
    TPtr8 buf(data->Des());
   
    aMessage.ReadL(0, buf, anOffset);
  
    User::LeaveIfError(iContFile.Write(aPos, *data, aLength));
    User::LeaveIfError(iContFile.Flush());
    CleanupStack::PopAndDestroy(data);

    // update iCachedSize and iWrittenSize if the container file size has grown
    if (aPos + aLength > iCachedSize)
        {
        iCachedSize = aPos + aLength;
        iWrittenSize = sizeToBeWritten;
        }
    
    // for flush() calls after this call:
    // set iFlushedSize to aPos, so that changes will be flushed
    if (iFlushedSize > aPos) 
        {
        iFlushedSize = aPos;
        }
    
    }

// -----------------------------------------------------------------------------
// CRsfwFsFileCB::SetSizeL
// Emply implementation, upper class already set iSize
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsFileCB::SetSizeL(
    TInt aSize)
    {
    iReportedSize = aSize;
    // we cannot set the actual size of the remote file, but also we do not
    // return KErrNotSupported as that would cause problems with CFileMan
    // and File Manager, which use SetSize() as an optimization to set the 
    // target file size in copy.
    // Propably calling setsize() on remote files when for example just writing
    // to an existing file would cause weird results. 
    }

// -----------------------------------------------------------------------------
// CRsfwFsFileCB::SetEntryL
// Sets the attribute mask, iAtt, and the modified time of the file, iModified.
// If aMask|aVal does not equal zero, then aMask should be OR'ed with iAtt,
// whilst the inverse of aVal should be AND'ed with iAtt.
// If the modified flag is set in aMask then iModified should be set to aTime.
//
// Implemented by calling CRsfwFsMountCB::SetEntryL().
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsFileCB::SetEntryL(
    const TTime& aTime,
    TUint aMask,
    TUint aVal)
    {
    static_cast<CRsfwFsMountCB&>(Mount()).SetEntryL(*iFileName,
                                                      aTime,
                                                      aMask,
                                                      aVal);                                             
    }

// -----------------------------------------------------------------------------
// CRsfwFsFileCB::FlushAllL
// Flushes, to disk, all cached file data (e.g. attributes, modification time,
// file size). The modified bit in the file attributes mask should be cleared if
// the flush was successful.
//
// File Server calls this before reading directory entries, getting an entry
// details for a directory entry etc. The idea is to make sure that all the
// information to be retrieved is up to date. We don't need to implement this,
// as our framework does not have buffers that could cause this problem. 
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsFileCB::FlushAllL()
    {
    }

// -----------------------------------------------------------------------------
// CRsfwFsFileCB::FlushDataL
// Flushes, to disk, the cached information necessary for the integrity
// of recently written data, such as the file size.
//
// Called by File Server as a result of RFile::Flush() (if the file has been 
// modified). In our framework the file should be written to the server.
// We should also clear KEntryAttModified here.
//
// (other items were commented in a header).
//
// -----------------------------------------------------------------------------
//
void CRsfwFsFileCB::FlushDataL()
    {
    TInt err = KErrNone;
    
    // close the container file to make sure all the local changes are cached
    iContFile.Close();
        
    // if flush was cancelled, but we come again to write, 
    // again set lastflushfailed to EFale	
    iLastFlushFailed = EFalse;
    
    // may attempt to write part of the file or whole file
    // also we may or may not know for sure whether the 
    // server supports partial write
starttoflush:
    if (!iPartialWriteSupported) 
        {
        // If we know that partial write is not supported
        // AND the client has reported the full size of 
        // the file, do not really flush until all the data
        // has been written to the cache file.
        // *
        // If we do not know the total size flush whatever is cached
        if ((iCachedSize == iReportedSize)  || iReportedSize == 0)
            {
            err = static_cast<CRsfwFsMountCB&>
    			(Mount()).RSessionL()->Flush(iThisFid,
    			                            0,
    			                            iCachedSize, 
    			                            iCachedSize);  			                            
            }
         // else do not do anything yet, only when the whole file is in cache
        }
     else 
        {
        // we "still" assume that partial write is supported
        err = static_cast<CRsfwFsMountCB&>
    		(Mount()).RSessionL()->Flush(iThisFid,
    			                         iFlushedSize,
    			                         iCachedSize, 
    			                         iReportedSize);
        if (err == KErrNotSupported) 
            {
            err = KErrNone; // reset the error
            // flushing the file not supported
            // probably because the access protol plugin does not support partial write
            iPartialWriteSupported = EFalse;
            // apply the flush logic again
            // this time with the knowledge that the server does not support partial
            // write
            goto starttoflush;
            }
         else 
            {
            iAtt &= ~KEntryAttModified; // clear KEntryAttModified if flush worked
            }
    			                         
        }
        
    // re-open the container file    
     User::LeaveIfError(iContFile.Open(*(RFs* )Dll::Tls(),
                                      iCachePath,
                                      EFileWrite | EFileShareAny)); 
    
        
    // handling the results    
    if (err == KErrNone) 
        {
        // the operation was successful
        iFlushedSize = iCachedSize;			                            
        }
    else if (err != KErrNone)
        {
        iLastFlushFailed = ETrue;
        User::Leave(err);
        }
   
    }
  
// -----------------------------------------------------------------------------
// CRsfwFsFileCB::SetContainerFileL
// Opens the container file.
// Called in CRsfwFsMountCB when the file is opened.
// Implementation notice: If this file is only opened for reading, it would
// seem sensible to open  EFileRead|EFileShareAny here, but when Remote 
// File Engine then tries to open EFileWrite|EFileShareAny it fails unless
// EFileWrite is specified here also...
//
// (other items were commented in a header).
// -----------------------------------------------------------------------------
//
void CRsfwFsFileCB::SetContainerFileL(
    const TDesC& aPath)
    {
    iCachePath = aPath;
    User::LeaveIfError(iContFile.Open(*(RFs* )Dll::Tls(),
                                      aPath,
                                      EFileWrite | EFileShareAny)); 
    }

// End of File
