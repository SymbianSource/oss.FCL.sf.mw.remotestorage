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


#ifndef C_RSFWPERMANENTSTORE_H
#define C_RSFWPERMANENTSTORE_H

// INCLUDES
#include <e32std.h>
#include <f32file.h>
#include <s32file.h>

// CLASS DECLARATION
class TFileHeader
    {
public:
    TUint   iHeaderStart;           // magic number
    TInt    iHeaderSize;            // (max) header size
    TInt    iBlockSize;             // block size
    HBufC8* iHeader;                // header 

public:
    void ExternalizeL(RWriteStream& aStream) const;
    void InternalizeL(RReadStream& aStream);
    };

class TSlot
    {
 public:
    TInt iIndex;        // data index (used as key for RArray)
    TInt iBlockNumber;  // starting block number
    TInt iBlockCount;   // number of blocks
    };

class CRsfwPermanentStore: public CBase
    {

class TFreeBlockList
    {
 public:
    RArray<TInt> iFreeBlockList; // list of continuous free block sequences
    };

    enum TFileStateInPermanentStore
        {
        EFileStateClosed = 0,
        EFileStateReading,
        EFileStateWriting
        };
public:
    static CRsfwPermanentStore* NewL(const TDesC& aPath,
                                 TInt aHeaderSize = 0,
                                 TInt aBlockSize = 0);
    ~CRsfwPermanentStore();
    void ResetL(TBool aWriting);
    TInt Commit();
    TInt Purge();
    void CompactL();
    void SetHeaderL(TDesC8& aHeader);
    const HBufC8* Header();
    void GetNextDataL(TUint8* aData, TInt& aDataLength, TInt& aIndex);
    void PutDataL(const TUint8* aData, TInt aDataLength, TInt& aIndex);

protected:
    void ConstructL(const TDesC& aPath,
                    TInt aHeaderSize,
                    TInt aBlockSize);

private:
    TInt BlockCount(TInt aDataLength);
    TInt StreamPosition(TInt aBlockNumber);
    TSlot* Slot(TInt aIndex);
    void FixSlot(TInt aOldBlockNumber, TInt aNewBlockNumber);
    void SetFileStateL(TInt aFileState);
    void LoadHeaderL();
    void SaveHeaderL();
    void ClearFreeBlockLists();
    void ClearSlotL(TInt aIndex);
    void WriteBlocksL(const TUint8* aData,
                      TInt aDataLength,
                      TInt aBlockNumber);
    void ReserveSlot(TInt aIndex, TInt aBlockNumber, TInt aBlockCount);
    void PutToFreeBlockList(TInt aBlockPos, TInt aBlockCount);
    TInt GetFromFreeBlockList(TInt aBlockCount);

private:
    RFs                    iFs;               // file server session
    RFile                  iFile;             // the file
    RFileReadStream        iFileReadStream;   // iFile mapped to read stream
    RFileWriteStream       iFileWriteStream;  // iFile mapped to write stream
    TBuf<KMaxPath>         iPath;             // path name of the store file
    TInt                   iHeaderSize;       // size of header payload
    TInt                   iFileHeaderSize;   // total size of header
    TInt                   iBlockSize;        // size of elementary block
    TInt                   iFileState;        // file opening state
    RArray<TFreeBlockList> iFreeBlockLists;   // lists of free blocks
    TFileHeader            iFileHeader;       // file information in the header
    RArray<TSlot>          iSlots;            // maps index/block number
    TInt                   iIndex;            // next slot position
    TInt                   iReadBlockNumber;  // next block to read
    TInt                   iWriteBlockNumber; // next block to write
    HBufC8*                iZeroBlock;        // a filler block
    };

#endif // PERMANENTSTORE_H

// End of File
