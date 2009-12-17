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
* Description:  Operation independent remote file handling functions
*
*/

#ifndef C_RSFWFILEENGINE_H
#define C_RSFWFILEENGINE_H

#include "rsfwinterface.h"
#include "rsfwremoteaccess.h"

class CRsfwLockManager;
class TDirEnt;
class CRsfwFileTable;
class CRsfwRfeStateMachine;
class CRsfwVolume;
class CRsfwFileEntry;

// Default values for various operation parameters
const TInt KDefaultMaxCacheSize             = (16 * 1024 * 1024); // 16 MB
const TInt KDefaultMaxEntryCount            = 1024;
const TInt KDefaultCacheValidity            = 30;            // in seconds
const TInt KDefaultDirCacheValidity         = 30;
const TInt KDefaultRecognizerLimit          = 256;
const TInt KDefaultJpegLimit                = 0;            
const TInt KDefaultMpegLimit                = 10000;


class CRsfwFileEngine: public CBase, public MRsfwRemoteAccessObserver
    {
public:
    static CRsfwFileEngine* NewL(CRsfwVolume* aVolume);
    static CRsfwFileEngine* NewLC(CRsfwVolume* aVolume);
    ~CRsfwFileEngine();
    
    void DispatchL(TRfeInArgs& aIn, TRfeOutArgs& aOut);
    HBufC* FullNameLC(CRsfwFileEntry& aFe);
    HBufC* FullNameL(CRsfwFileEntry& aFe);
    void SetupAttributes(CRsfwFileEntry& aFe);
    void MakeDirectoryEntry(CRsfwFileEntry& aFe, TDirEnt& aDirEnt);
    void UpdateDirectoryContainerL(CRsfwFileEntry& aFe);
    TBool DataChanged(const CRsfwDirEntAttr& oldAttr, const CRsfwDirEntAttr& newAttr);
    TBool UseCachedData(CRsfwFileEntry& aFe);    
    TBool UseCachedAttributes(CRsfwFileEntry& aFe);
    void GetAttributesL(CRsfwFileEntry& aFe,
                        CRsfwDirEntAttr*& aAttr,
                        TUint aNodeType, 
                        CRsfwRfeStateMachine* aCaller = NULL);
    void UpdateAttributesL(TDesC& aPath,
                           TDesC& aName,
                           CRsfwDirEntAttr*& aAttr,
                           TUint aNodeType,
                           MRsfwRemoteAccessResponseHandler* aCaller = NULL);
    void UpdateAttributesL(CRsfwFileEntry& aFe, 
                           CRsfwDirEntAttr*& aAttr, 
                           TUint aNodeType, 
                           MRsfwRemoteAccessResponseHandler* aCaller = NULL);
    void UpdateAttributesL(TDesC& aFullPath, 
                           CRsfwDirEntAttr*& aAttr, 
                           TUint aNodeType, 
                           MRsfwRemoteAccessResponseHandler* aCaller = NULL);
    void CreateContainerFileL(CRsfwFileEntry& aFe);
    TUint FetchAndCacheL(CRsfwFileEntry& aFe,
                        TInt aFirstByte,
                        TInt* aLength, 
                        RPointerArray<CRsfwDirEnt>* aDirEntsp,
                        CRsfwRfeStateMachine* aCaller = NULL);                                                         
    TUint RequestConnectionStateL(TUint aConnectionState,
                                 CRsfwRfeStateMachine* aCaller = NULL);                                                                  
    void EnteredConnectionStateL(TUint aConnectionState, TBool aRequested);
    TUint ConnectionState();
    CRsfwLockManager* LockManager();       
    void SetPermanenceL(TBool aPermanence);
    TBool Disconnected();
    TBool WriteDisconnected();
    TInt AddToCacheL(CRsfwFileEntry& aFe, 
                     RPointerArray<CRsfwDirEnt>* aDirEnts, 
                     CRsfwFileEngine *aFileEngine, 
                     TUint cachedSize);
    CRsfwRemoteAccess* RemoteAccessL();
    void OperationCompleted();
    void CancelTransactionL(TDesC& aPathName);
    void CancelTransaction(TUint aTransactionId);
    void SetFailedLookup(TDesC& aPath, TDesC& aKidName); 
    void ResetFailedLookup();
    CRsfwVolume* Volume();
    TInt PurgeFromCache(const TDesC& aPath);
    HBufC8* GetContentType(TDesC& aName); 
    CRsfwFileEntry* FetchFep(const TDesC& aPath);

private:
    void ConstructL(CRsfwVolume* aVolume);
    void PrepareCacheL();
    TInt UpdateDirectoryL(CRsfwFileEntry& aFe, TDesC* aFullName);
    TInt UpdateDirectoryL(CRsfwFileEntry& aFe);
    TUint GetDirectoryL(CRsfwFileEntry& aFe,
                       TDesC& aFullName,
                       RFile& aF,
                       RPointerArray<CRsfwDirEnt>* aDirEntsp, 
                       MRsfwRemoteAccessResponseHandler* aCaller = NULL);
    void BuildContainerPathL(CRsfwFileEntry& aFe, TDes& aPath);
    void ApplyMultiDirCacheL(TDes& aPath); 
    void CreateContainerFileL(CRsfwFileEntry& aFe, TDes& aPath, RFile& aF);
    void DoCreateL(TRfeCreateInArgs& aIn, TRfeCreateOutArgs& aOut);
    void DoIoctlL(TRfeIoctlInArgs& aIn,TRfeOutArgs& aOut);
    void DoRootL(TRfeRootInArgs& aIn, TRfeRootOutArgs& aOut);
    void DoSetAttrL(TRfeSetAttrInArgs& aIn, TRfeOutArgs& aOut);
    void SetupRootL(TBool aPermanence);
    void CleanupCorruptedCacheL();
    TUint ConnectL(TBool aRestart, CRsfwRfeStateMachine* aCaller = NULL);
    void DisconnectL();
    void StartInactivityTimer();
    void StopInactivityTimer();
    static TInt InactivityTimerExpired(TAny* aArg);
   
    
    // the purpose of these functions is to give capability info
    // for the access protocol plugin used
    
    // whether getting the directory listing also gives reliable file metadata
    TBool DirectoryListingContainsFileMetadata();
    
    // from MRsfwRemoteAccessObserver
    void HandleRemoteAccessEventL(TInt aEventType, TInt aEvent, TAny* aArg);

public:
    CRsfwFileTable*    iFileTable;         // table of known vnodes, by fid
    TUint          iConnectionState;   // connection state
    CRsfwRemoteAccess* iRemoteAccess;      // remote file transport module
    CRsfwLockManager*  iLockManager;       // implement locking for files
 
private:
    
	// Data
    CRsfwFileEntry*    iRootFep;           // root file entry
    CRsfwVolume*       iVolume;            // volume info
    const TFid*    iRootFid;           // root file id
    TFileName      iCacheRoot;         // location of local cache files
    RFs            iFs;                // fileserver handle
    TInt           iInactivityTimeout; // inactivity timeout
    CPeriodic*     iInactivityTimer;   // remote access inactivity watch dog
    TBool          iConnectionStateChanged;
    TFileName      iLastFailedLookup;   // path of the last failed lookup, cached 
    TTime          iLookupTime; 		// then this failed lookup happened      
    };


#endif
