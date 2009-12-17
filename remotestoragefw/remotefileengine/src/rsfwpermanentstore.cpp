/*
* Copyright (c) 2004-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Provides persistent storage for memory blocks
*
*/


// This is a simple block based permanent storage, in which transient
// indices are computed on the fly when all data is sequentially read
// from the storage.
//
// The data is put into "slots" that consist of one or more "blocks".
// Subsequent updates to the data slots only use the current slot
// if the required slot size (block count) has not changed.
//
// The storage contains a header that may contain any data.
// In addition, Each slot contains a header (of type TInt) that
// describes the size of the data in the slot.
//
// The file can be compressed for space efficiency.
//
// The assumption is that the store is first opened for reading for loading
// the data, and then it is only opened for writing until the file is closed.
//
// (We don't use CRsfwPermanentStore because besides data, we should
//  also maintain the indices in a stream of its own)

// INCLUDE FILES
#include <s32strm.h>
#include <s32buf.h>

#include "rsfwpermanentstore.h"
#include "mdebug.h"


// CONSTANTS
const TUint KHeaderStart      = 0xbabefade;
const TInt  KSlotHeaderSize   = 4;          // sizeof(TInt)
const TInt  KDefaultBlockSize = 128;        // default block size in bytes
// max value for integer read from the stream, subsequently used to create descriptor
const TInt  KMaxDescriptorSize = 16384;         


// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// TFileHeader::ExternalizeL
// ----------------------------------------------------------------------------
//
void TFileHeader::ExternalizeL(RWriteStream& aStream) const
    {
    aStream.WriteInt32L(iHeaderStart);
    aStream.WriteInt32L(iBlockSize);
    aStream.WriteInt32L(iHeaderSize);
    aStream << *iHeader;
    }

// ----------------------------------------------------------------------------
// TFileHeader::InternalizeL
// ----------------------------------------------------------------------------
//
void TFileHeader::InternalizeL(RReadStream& aStream)
    {
    iHeaderStart = aStream.ReadInt32L();
    iBlockSize = aStream.ReadInt32L();
    if (iBlockSize < 0 || iBlockSize > KMaxDescriptorSize)
        {
        User::Leave(KErrCorrupt);
        }    
    iHeaderSize = aStream.ReadInt32L();
    if (iHeaderSize < 0 || iHeaderSize > KMaxDescriptorSize)
        {
        User::Leave(KErrCorrupt);
        }     
    iHeader = HBufC8::NewL(aStream, iHeaderSize);
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::NewL
// ----------------------------------------------------------------------------
//
CRsfwPermanentStore* CRsfwPermanentStore::NewL(const TDesC& aPath,
                                       TInt aHeaderSize,
                                       TInt aBlockSize)
    {
    CRsfwPermanentStore* self = new (ELeave) CRsfwPermanentStore();
    CleanupStack::PushL(self);
    self->ConstructL(aPath, aHeaderSize, aBlockSize);
    CleanupStack::Pop(self);
    return self;
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::~CRsfwPermanentStore
// ----------------------------------------------------------------------------
//
CRsfwPermanentStore::~CRsfwPermanentStore()
    {
    ClearFreeBlockLists();
    iSlots.Close();
    iFileReadStream.Close();
    iFileWriteStream.Close();
    iFile.Close();
    iFs.Close();
    delete iFileHeader.iHeader;
    delete iZeroBlock;
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::ConstructL
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::ConstructL(const TDesC& aPath,
                                 TInt aHeaderSize,
                                 TInt aBlockSize)
    {
    DEBUGSTRING(("CRsfwPermanentStore::ConstructL()"));
    User::LeaveIfError(iFs.Connect());
    iPath.Copy(aPath);

    iHeaderSize = aHeaderSize;
    // Offset to the block data
    iFileHeaderSize = iHeaderSize + sizeof(TFileHeader);
    TRAPD(err, LoadHeaderL());
    if (err == KErrNone)
        {
        // Read parameters from the existing file, if any
        iHeaderSize = iFileHeader.iHeaderSize;
        iBlockSize = iFileHeader.iBlockSize;
        }
    else
        {
        // There was no existing file
        DEBUGSTRING(("LoadHeaderL returns %d", err));
        if (aBlockSize)
            {
            iBlockSize = aBlockSize;
            }
        else
            {
            iBlockSize = KDefaultBlockSize;
            }
        iFileHeader.iHeaderStart = KHeaderStart;
        iFileHeader.iHeaderSize = iHeaderSize;
        iFileHeader.iBlockSize = iBlockSize;
        }
    // Compute the offset to data blocks
    // (this is 4 bytes too many but that is OK ...)
    iFileHeaderSize = iHeaderSize + sizeof(TFileHeader);
    // Make a block of zeroes
    iZeroBlock = HBufC8::NewL(iBlockSize);
    iZeroBlock->Des().FillZ();
    DEBUGSTRING(("ConstructL() done, blocksize=%d", iBlockSize));
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::ResetL
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::ResetL(TBool aWriting)
    {
    // This function is called by the client
    // before starting to read or write data slots
    if (aWriting)
        {
        SetFileStateL(EFileStateWriting);
        // Set write pointer to the end of the file
        MStreamBuf* streamBuf = iFileWriteStream.Sink();
        TInt size = streamBuf->SizeL();
        iWriteBlockNumber = (size - iFileHeaderSize) / iBlockSize;
        if ((iWriteBlockNumber * iBlockSize) != (size - iFileHeaderSize))
            {
            DEBUGSTRING(("ResetL(): file size incorrect (%d)",
                         size));
            }
        }
    else
        {
        // reading
        SetFileStateL(EFileStateReading);
        // Skip file header
        MStreamBuf* streamBuf = iFileReadStream.Source();
        streamBuf->SeekL(MStreamBuf::ERead,
                         EStreamBeginning,
                         iFileHeaderSize);
        iReadBlockNumber = 0;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::Commit
// ----------------------------------------------------------------------------
//
TInt CRsfwPermanentStore::Commit()
    {
    TInt err = KErrNone;
    // Commit writing
    if (iFileState == EFileStateWriting)
        {
        MStreamBuf* streamBuf = iFileWriteStream.Sink();
        err = streamBuf->Synch();
        }
    return err;
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::Purge
// ----------------------------------------------------------------------------
//
TInt CRsfwPermanentStore::Purge()
    {
    iFileReadStream.Close();
    iFileWriteStream.Close();
    iFile.Close();
    return iFs.Delete(iPath);
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::CompactL
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::CompactL()
    {
    // Compact the file by dropping empty slots.
    // The slot table is also fixed and the free block lists are cleared
    // to reflect the new file layout.

    // This function must not be called while the file is opened for reading.
    // It is most efficient to call this function before any data is read
    // (then there is no slot table to fix)

    // This function must be called before doing GetNextDataL()
    // (must be followed by ResetL()).
    // However, this function can be called while doing PutDataL(). 

    DEBUGSTRING(("CompactL()"));
    SetFileStateL(EFileStateClosed);
    SetFileStateL(EFileStateReading);
    
    TBuf<KMaxPath> dstPath;
    dstPath.Copy(iPath);
    dstPath.Append('a');
    RFile dstFile;
    User::LeaveIfError(dstFile.Replace(iFs,
                                       dstPath,
                                       EFileShareAny | EFileWrite));
    CleanupClosePushL(dstFile);
    RFileWriteStream dstFileWriteStream(dstFile);
    CleanupClosePushL(dstFileWriteStream);
    MStreamBuf* streamBuf = iFileReadStream.Source();

    // Copy header
    dstFileWriteStream.WriteL(iFileReadStream, iFileHeaderSize);

    RArray<TSlot> gaps;
    CleanupClosePushL(gaps);

    TInt blockNumber = 0;
    TInt lastReadBlockNumber = 0;
    TInt nextWriteBlockNumber = 0;
    TInt err = KErrNone;
    while (err == KErrNone)
        {
        TSlot gap;
        TInt dataLength = 0;
        TRAP(err, dataLength = iFileReadStream.ReadInt32L());
        if (err == KErrNone)
            {
            TInt blockCount;
            if (dataLength >= 0)
                {
                // real data
                blockCount = BlockCount(dataLength);
#if 0
                DEBUGSTRING(("Copying at block %d, count %d",
                             blockNumber,
                             blockCount));
#endif
                // Back off four bytes
                streamBuf->SeekL(MStreamBuf::ERead,
                                 EStreamMark,
                                 -KSlotHeaderSize);
                dstFileWriteStream.WriteL(iFileReadStream,
                                          blockCount * iBlockSize);
                lastReadBlockNumber = blockNumber;
                nextWriteBlockNumber += blockCount;
                }
            else
                {
                // empty slot
                DEBUGSTRING(("Compacting at block %d", blockNumber));
                blockCount = -dataLength;
                streamBuf->SeekL(MStreamBuf::ERead,
                                 EStreamMark,
                                 blockCount * iBlockSize - KSlotHeaderSize);
                // Mark block position
                gap.iIndex = 0;  // not needed here
                gap.iBlockNumber = blockNumber;
                gap.iBlockCount = blockCount;
                gaps.Append(gap);
                }
            blockNumber += blockCount;
            }
        }
    if (err == KErrEof)
        {
        err = KErrNone;
        }
    // Replace old file with the compressed file
    SetFileStateL(EFileStateClosed);
    dstFileWriteStream.Close();
    dstFile.Close();
    if ((err == KErrNone) && gaps.Count())
        {
        // No errors and some compaction was achieved
        err = iFs.Delete(iPath);
        if (err == KErrNone)
            {
            err = iFs.Rename(dstPath, iPath);
            if (err == KErrNone)
                {
                DEBUGSTRING(("CompactL(): gaps %d slots %d",
                             gaps.Count(),
                             iSlots.Count()));
                if (gaps.Count() && iSlots.Count())
                    {
                    // Fix slot table (0(n**2))
                    TInt oldBlockNumber = 0;
                    TInt newBlockNumber = 0;
                    while (oldBlockNumber <= lastReadBlockNumber)
                        {
                        if (gaps.Count())
                            {
                            // still more gaps ...
                            if (oldBlockNumber == gaps[0].iBlockNumber)
                                {
                                oldBlockNumber += gaps[0].iBlockCount;
                                gaps.Remove(0);
                                }
                            }
                        if (oldBlockNumber != newBlockNumber)
                            {
                            FixSlot(oldBlockNumber, newBlockNumber);
                            }
                        oldBlockNumber++;
                        newBlockNumber++;
                        }
                    }
                }
            }
        // Clear free block lists
        ClearFreeBlockLists();
        // Setup next block to write
        iWriteBlockNumber = nextWriteBlockNumber;
        }
    else
        {
        // Some error occured or no compaction was achieved
        TInt r = iFs.Delete(dstPath);
#ifdef _DEBUG
        DEBUGSTRING(("CompactL(): destination file deletion returns %d ", r));
#endif
        }

    CleanupStack::PopAndDestroy(3, &dstFile); // gaps, dstFileWriteStream, dstFile
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::SetHeaderL
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::SetHeaderL(TDesC8& aHeader)
    {
    delete iFileHeader.iHeader;
    iFileHeader.iHeader = NULL;
    iFileHeader.iHeader = aHeader.AllocL();
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::Header
// ----------------------------------------------------------------------------
//
const HBufC8* CRsfwPermanentStore::Header()
    {
    if (iFileHeader.iHeader)
        {
        return iFileHeader.iHeader;
        }
    else
        {
        return NULL;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::GetNextDataL
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::GetNextDataL(TUint8* aData,
                                   TInt& aDataLength,
                                   TInt& aIndex)
    {
    // Return the next slotful of data.
    // This function must only be used for sequential access
    SetFileStateL(EFileStateReading);

    MStreamBuf* streamBuf = iFileReadStream.Source();
    
    TBool done = EFalse;
    while (!done)
        {
        TInt dataLength = 0;
        // Eventually leaves with KErrEof
        dataLength = iFileReadStream.ReadInt32L();
        TInt blockCount;
        if (dataLength >= 0)
            {
            // Fill data
            iFileReadStream.ReadL(aData, dataLength);
            aDataLength = dataLength;
            aIndex = iIndex;

            // Update block map
            blockCount = BlockCount(dataLength);
#if 0
            DEBUGSTRING((
                            "GetNextDataL(): index=%d, block=%d, count=%d, len=%d",
                            iIndex,
                            iReadBlockNumber,
                            blockCount,
                            dataLength));
#endif

            ReserveSlot(iIndex, iReadBlockNumber, blockCount); 
            iIndex++;
            iReadBlockNumber += blockCount;

            // Update read position to the start of next block
            TInt offset = iBlockSize -
                (KSlotHeaderSize + dataLength) % iBlockSize;
            if (offset != iBlockSize)
                {
                streamBuf->SeekL(MStreamBuf::ERead, EStreamMark, offset);
                }
            
            done = ETrue;
            }
        else
            {
            // Negative length indicates a number of free blocks.
            // Put such empty blocks into the free block list
            blockCount = -dataLength;
            PutToFreeBlockList(iReadBlockNumber, blockCount);
            
            // Seek to the next slot
            streamBuf->SeekL(MStreamBuf::ERead, 
                             EStreamMark,
                             blockCount * iBlockSize - KSlotHeaderSize);
            iReadBlockNumber += blockCount;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::PutDataL
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::PutDataL(const TUint8* aData,
                               TInt aDataLength,
                               TInt& aIndex)
    {
    // Write data with the given index
    // If the index < 0, this a new data
    SetFileStateL(EFileStateWriting);

    if (aDataLength == 0)
        {
        // We just want to dispose of the slot
        ClearSlotL(aIndex);
        return;
        }

    TBool done = EFalse;
    TInt blockCount = BlockCount(aDataLength);
    if (aIndex >= 0)
        {
        // Check if we have space in the existing slot
        TSlot* slot = Slot(aIndex);
        // We should always find the slot
        if (slot)
            {
            if (slot->iBlockCount == blockCount)
                {
                // We can use the current slot
                WriteBlocksL(aData, aDataLength, slot->iBlockNumber);
                done = ETrue;
                }
            else
                {
                // Clear the slot
                ClearSlotL(aIndex);
                }
            }
        else
            {
            DEBUGSTRING(("Slot %d not found!", aIndex));
            }
        }
    else
        {
        // Allocate new index
        aIndex = iIndex++;
        }
    
    if (!done)
        {
        // Try to get a free slot
        TInt blockNumber = GetFromFreeBlockList(blockCount);
        if (blockNumber == KErrNotFound)
            {
            // We have to append to the file
            blockNumber = iWriteBlockNumber;
            iWriteBlockNumber += blockCount;
            }
        
        ReserveSlot(aIndex, blockNumber, blockCount);
        WriteBlocksL(aData, aDataLength, blockNumber);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::BlockCount
// ----------------------------------------------------------------------------
//
TInt CRsfwPermanentStore::BlockCount(TInt aDataLength)
    {
    // Get the block count for a given data size
    return (KSlotHeaderSize + aDataLength - 1) / iBlockSize + 1;
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::StreamPosition
// ----------------------------------------------------------------------------
//
TInt CRsfwPermanentStore::StreamPosition(TInt aBlockNumber)
    {
    // Get the stream position from block position
    return (iFileHeaderSize + aBlockNumber * iBlockSize);
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::Slot
// ----------------------------------------------------------------------------
//
TSlot* CRsfwPermanentStore::Slot(TInt aIndex)
    {
    // Find the slot with the given index
    TSlot slot;             // dummy slot for Find()
    slot.iIndex = aIndex;
    TInt i = iSlots.Find(slot);
    if (i != KErrNotFound)
        {
        return &iSlots[i];
        }
    else
        {
        return NULL;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::FixSlot
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::FixSlot(TInt aOldBlockNumber, TInt aNewBlockNumber)
    {
    // Assign a new starting block to the slot that used to
    // start at aOldBlockNumber.
    // The block numbers are changed due to compaction -
    // thus the block numbers can only become smaller.
    TInt i;
    for (i = 0; i < iSlots.Count(); i++)
        {
        TSlot& slot = iSlots[i];
        // Note that this function can also be called with block numbers
        // that do not start a slot - then there will be no match
        if (slot.iBlockNumber == aOldBlockNumber)
            {
            DEBUGSTRING(("Fixing slot %d = %d",
                         aOldBlockNumber,
                         aNewBlockNumber));
            slot.iBlockNumber = aNewBlockNumber;
            return;
            }
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::SetFileStateL
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::SetFileStateL(TInt aFileState)
    {
    DEBUGSTRING(("CRsfwPermanentStore::SetFileStateL"));
    DEBUGSTRING(("iFileState = %d, aFileState = %d", iFileState, aFileState));
    if (iFileState != aFileState)
        {
        switch (iFileState)
            {
        case EFileStateClosed:
            {
            if (aFileState == EFileStateReading)
                {
                DEBUGSTRING(("opening a closed file for reading"));
                DEBUGSTRING16(("path is %S", &iPath));
                User::LeaveIfError(iFile.Open(iFs,
                                              iPath,
                                              EFileShareAny | EFileRead));
                TInt size;
                iFile.Size(size);
                DEBUGSTRING(("opening successfull, file size is %d", size));
                DEBUGSTRING(("header size %d", iFileHeaderSize));
                // sanity
                if (size < iFileHeaderSize)
                    {
                    // Close the file and leave the file state as "closed"
                    iFile.Close();
                    User::Leave(KErrNotFound);
                    }
                else
                    {
                    iFileReadStream.Attach(iFile);
                    }
                }
            else
                {
                // EFileStateWriting
                TInt err = iFile.Open(iFs, iPath, EFileShareAny | EFileWrite);
                if (err != KErrNone)
                    {
                    // The file did not exist
                    User::LeaveIfError(
                        iFile.Create(iFs,
                                     iPath,
                                     EFileShareAny | EFileWrite));
                    }
                TInt size;
                User::LeaveIfError(iFile.Size(size));
                iFileWriteStream.Attach(iFile);
                if (size < iFileHeaderSize)
                    {
                    // Store header if this was a new file
                    SaveHeaderL();
                    }
                }
            }
            break;
        
        case EFileStateReading:
        case EFileStateWriting:
            {
            // close and redo
            if (iFileState == EFileStateReading)
                {
                iFileReadStream.Close();
                }
            else
                {
                iFileWriteStream.Close();
                }
            iFile.Close();
            iFileState = EFileStateClosed;
            SetFileStateL(aFileState);
            }
            break;
    
        default:
            break;
            }
        
        iFileState = aFileState;
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::LoadHeaderL
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::LoadHeaderL()
    {
    DEBUGSTRING(("CRsfwPermanentStore::LoadHeaderL"));
    SetFileStateL(EFileStateReading);
    iFileHeader.InternalizeL(iFileReadStream);
    ResetL(EFalse);
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::SaveHeaderL
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::SaveHeaderL()
    {
    // Write header with filler
    iFileHeader.ExternalizeL(iFileWriteStream);
    MStreamBuf* streamBuf = iFileWriteStream.Sink();
    TInt fileHeaderSize = streamBuf->TellL(MStreamBuf::EWrite).Offset();
    TInt residue = iFileHeaderSize - fileHeaderSize;
    HBufC8* fill = HBufC8::NewLC(residue);
    TPtr8 fillZ = fill->Des();
    fillZ.SetLength(residue);
    fillZ.FillZ();
    DEBUGSTRING(("SaveHeader(): header=%d, filler=%d",
                 iFileHeaderSize,
                 fillZ.Length()));
    iFileWriteStream.WriteL(fillZ);
    Commit();
    CleanupStack::PopAndDestroy(fill); // fill
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::ClearFreeBlockLists
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::ClearFreeBlockLists()
    {
    TInt i;
    for (i = 0; i < iFreeBlockLists.Count(); i++)
        {
        iFreeBlockLists[i].iFreeBlockList.Close();
        }
    iFreeBlockLists.Close();
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::ClearSlotL
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::ClearSlotL(TInt aIndex)
    {
    // Mark a slot as unused in the file and
    // add the slot in the free block list

    TSlot s;                // dummy slot for iSlots.Find()
    s.iIndex = aIndex;
    TInt i = iSlots.Find(s);
    if (i != KErrNotFound)
        {
        TSlot& slot = iSlots[i];
        // Mark the slot in the file as empty
        TInt pos = iFileHeaderSize +  slot.iBlockNumber * iBlockSize;

        DEBUGSTRING(("ClearSlotL(): index=%d, block=%d, count=%d, pos=%d",
                     aIndex,
                     slot.iBlockNumber,
                     slot.iBlockCount,
                     pos));

        MStreamBuf* streamBuf = iFileWriteStream.Sink();
        streamBuf->SeekL(MStreamBuf::EWrite, EStreamBeginning, pos);
        iFileWriteStream.WriteInt32L(-slot.iBlockCount);
        
        // Add the slot in the free block list
        PutToFreeBlockList(slot.iBlockNumber, slot.iBlockCount);

        // Delete the slot from the slot table
        iSlots.Remove(i);
        }
    else
        {
        DEBUGSTRING(("ClearSlotL(): index=%d not found!", aIndex));
        User::Leave(KErrNotFound);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::WriteBlocksL
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::WriteBlocksL(const TUint8* aData,
                                   TInt aDataLength,
                                   TInt aBlockNumber)
    {
    // Put the given data in the slot specified by the index
    TInt pos = iFileHeaderSize +  aBlockNumber * iBlockSize;

#if 0
    DEBUGSTRING(("WriteBlocksL(): block=%d, len=%d, pos=%d",
                 aBlockNumber,
                 aDataLength,
                 pos));
#endif
#if 0
    TInt size;
    iFile.Size(size);

    DEBUGSTRING(("size=%d", size));
#endif
    
    MStreamBuf* streamBuf = iFileWriteStream.Sink();
    streamBuf->SeekL(MStreamBuf::EWrite, EStreamBeginning, pos);
    
    // Write the data in the file (preceded by the length)
    iFileWriteStream.WriteInt32L(aDataLength);
    iFileWriteStream.WriteL(aData, aDataLength);

    // We have to fill the whole slot if this is the last slot
    // in the file. Well, we will will the last bock of all slots
    TInt residue = iBlockSize - (KSlotHeaderSize + aDataLength) % iBlockSize;
    if (residue != iBlockSize)
        {
        iFileWriteStream.WriteL(iZeroBlock->Des().Ptr(), residue);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::ReserveSlot
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::ReserveSlot(TInt aIndex,
                                  TInt aBlockNumber,
                                  TInt aBlockCount)
    {
    TSlot slot;

    slot.iIndex = aIndex;
    slot.iBlockNumber = aBlockNumber;
    slot.iBlockCount = aBlockCount;
    iSlots.Append(slot);
    }


// ----------------------------------------------------------------------------
// CRsfwPermanentStore::PutToFreeBlockList
// ----------------------------------------------------------------------------
//
void CRsfwPermanentStore::PutToFreeBlockList(TInt aBlockPos, TInt aBlockCount)
    {
    while (iFreeBlockLists.Count() < aBlockCount)
        {
        // Construct list until blockCount size
        TFreeBlockList freeBlockList;
        iFreeBlockLists.Append(freeBlockList);
        }
    iFreeBlockLists[aBlockCount - 1].iFreeBlockList.Append(aBlockPos);
    }

// ----------------------------------------------------------------------------
// CRsfwPermanentStore::GetFromFreeBlockList
// ----------------------------------------------------------------------------
//
TInt CRsfwPermanentStore::GetFromFreeBlockList(TInt aBlockCount)
    {
    // Only support exact matches.
    // That is, bigger slots are not broken into smaller slots
    if (iFreeBlockLists.Count() < aBlockCount)
        {
        // no list
        return KErrNotFound;
        }

    TFreeBlockList& freeBlockList = iFreeBlockLists[aBlockCount - 1]; 
    if (freeBlockList.iFreeBlockList.Count() == 0)
        {
        // no entries in the list
        return KErrNotFound;
        }

    // Get the first entry in the list
    TInt blockPos = freeBlockList.iFreeBlockList[0];
    freeBlockList.iFreeBlockList.Remove(0);
    return blockPos;
    }

// End of File
