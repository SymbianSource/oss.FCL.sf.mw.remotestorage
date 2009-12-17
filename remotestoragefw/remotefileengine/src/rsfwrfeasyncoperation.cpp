/*
* Copyright (c) 2005-2006 Nokia Corporation and/or its subsidiary(-ies).
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
* Description:  Encapsulates an asynchronous operation
*
*/


#include "rsfwrfeasyncoperation.h"
#include "rsfwrfemessagerequest.h"
#include "rsfwrfestatemachine.h"
#include "rsfwmountstatemachine.h"
#include "rsfwmountconnectionstatemachine.h"
#include "rsfwopenbypathstatemachine.h"
#include "rsfwgetattributesstatemachine.h"
#include "rsfwattributerefreshingstatemachine.h"
#include "rsfwfetchandcachestatemachine.h"
#include "rsfwfetchdatastatemachine.h"
#include "rsfwlookupstatemachine.h"
#include "rsfwclosestatemachine.h"
#include "rsfwflushstatemachine.h"
#include "rsfwmkdirstatemachine.h"
#include "rsfwdeletestatemachine.h"
#include "rsfwcreatefilestatemachine.h"
#include "rsfwrenamefilestatemachine.h"
#include "rsfwvolumetable.h"
#include "mdebug.h"
#include "rsfwcommon.h"
#include "rsfwrfeserver.h"
#include "rsfwinterface.h"


// ----------------------------------------------------------------------------
// CRsfwRfeAsyncOperation::~CRsfwRfeAsyncOperation
// ----------------------------------------------------------------------------
//   
CRsfwRfeAsyncOperation::~CRsfwRfeAsyncOperation() 
    {
    delete iImplementation;
    iImplementation = NULL;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeAsyncOperation::Implementation
// ----------------------------------------------------------------------------
//  
CRsfwRfeStateMachine* CRsfwRfeAsyncOperation::Implementation()
    {
    return iImplementation;
    } 

// ----------------------------------------------------------------------------
// CRsfwRfeAsyncOperation::SetImplementation
// ----------------------------------------------------------------------------
//  
void CRsfwRfeAsyncOperation::SetImplementation(CRsfwRfeStateMachine* aImpl)
    {
    iImplementation = aImpl;
    }

// ----------------------------------------------------------------------------
// CRsfwRfeAsyncOperation::SetL
// Set asynchronous operation and its parameters
// Leave if OOM etc.
// ----------------------------------------------------------------------------
//  
void CRsfwRfeAsyncOperation::SetL(CRsfwRfeRequest* aRequest, TInt aOpCode) 
    {                 
    CRsfwRfeMessageRequest* request = (CRsfwRfeMessageRequest*)aRequest;
    CRsfwRfeStateMachine::TState* initialState = NULL;
    CRsfwRfeStateMachine* operation = NULL;

    if (aRequest->iVolume) 
        {
        DEBUGSTRING(("<<< Dispatch enter (operation=%d, volume=%d)",
                     aOpCode,
                     aRequest->iVolume->iMountInfo.iMountStatus.iVolumeId));
        }
    else 
        {
        DEBUGSTRING(("<<< Dispatch enter (operation=%d)", aOpCode));  
        }
                   
    switch (aOpCode) 
        {
    case EMount:
    case EMountByDriveLetter:
        {   
        DEBUGSTRING(("EMount / EMountByDriveLetter"));
        TRsfwMountConfig* mountConfig = new (ELeave) TRsfwMountConfig;
        CleanupStack::PushL(mountConfig);
        TPckg<TRsfwMountConfig> mountConfigPackage(*mountConfig);
        if (aOpCode == EMount)
            {
            request->Message().ReadL(0, mountConfigPackage);
            // this is to satisfy IPC fuzz testing
            VerifyMountConfigL(*mountConfig);
            }
        else
            {
            // EMountByDriveLetter - create a stub TRsfwMountConfig
            TInt driveNumber = request->Message().Int0();
            RFs fs = CRsfwRfeServer::Env()->iFs;
            fs.DriveToChar(driveNumber, mountConfig->iDriveLetter);
            mountConfig->iUri.SetLength(0);
            }

        // fetch mountconfig from mountStore
        if (!mountConfig->iUri.Length() || aOpCode == EMountByDriveLetter)
            {
            // the drive letter must be set
            User::LeaveIfError(aRequest->iVolumeTable->GetMountConfigL(
                                   *mountConfig));
            }
        
        // If there is an existing mount, the configurations must match.
        // Otherwise we unmount the already existing mount.
        CRsfwVolume* volume =
            aRequest->
            iVolumeTable->
            VolumeByDriveLetter(mountConfig->iDriveLetter);         
        if (volume)
            {
            if (volume->iMountInfo.iMountConfig.iUri.CompareF(
                    mountConfig->iUri) != 0)
                {
                // We have a mismatch
                DEBUGSTRING16(("Dismounting an obsolete mount %S",
                               &volume->iMountInfo.iMountConfig.iUri));
                aRequest->
                    iVolumeTable->
                    DismountByDriveLetterL(mountConfig->iDriveLetter,
                                           ETrue);
                }
            }
        TInt mountState =
            aRequest->iVolumeTable->MountState(mountConfig->iDriveLetter);
        operation = CRsfwMountStateMachine::NewL(*mountConfig, 
                                                 mountState, 
                                                 aRequest->iVolumeTable);
        CleanupStack::PushL(operation);
        ((CRsfwWaitNoteStateMachine *) operation)->BaseConstructL();
        initialState =
                new (ELeave) CRsfwMountStateMachine::TRequestConnectionState(
                    (CRsfwMountStateMachine *)operation);
        
        CleanupStack::Pop(operation);
        DEBUGSTRING16(("EMount: name '%S' with URI='%S' as '%c' (state=%d)",
                       &mountConfig->iName,
                       &mountConfig->iUri,
                       TUint(mountConfig->iDriveLetter),
                       mountState));
        CleanupStack::PopAndDestroy(mountConfig);
        }
        break;

    case ESetMountConnectionState:
        {
         DEBUGSTRING(("ESetMountConnectionState"));
        // Set the connection state of the mount
        TChar driveLetter = reinterpret_cast<TInt>(request->Message().Ptr0());
        TUint state = reinterpret_cast<TUint>(request->Message().Ptr1());
        DEBUGSTRING(("EMountConnectionState: drive '%c', state=%d",
                     TUint(driveLetter),
                     state));
                     
        operation = new(ELeave) CRsfwMountConnectionStateMachine(driveLetter,
                                                             state);
        CleanupStack::PushL(operation);
        ((CRsfwWaitNoteStateMachine *) operation)->BaseConstructL();
        initialState =
            new (ELeave) CRsfwMountConnectionStateMachine::TChangeConnectionState(
                (CRsfwMountConnectionStateMachine *)operation);
        CleanupStack::Pop(operation);
        }
    
        break;

    case EOpenByPath:
        {
        DEBUGSTRING(("OPENBYPATH"));
        operation = new (ELeave) CRsfwOpenByPathStateMachine();
        CleanupStack::PushL(operation);
        operation->BaseConstructL();
        // we need to refresh attributes first
        initialState =
            new (ELeave) CRsfwGetAttributesStateMachine::TRefreshAttributesState(
                (CRsfwAttributeRefreshingStateMachine *)operation);
        CleanupStack::Pop(operation);
        break;
        }

    case EFetch:
        {
        DEBUGSTRING(("FETCH"));
        operation = new (ELeave) CRsfwFetchAndCacheStateMachine();
        CleanupStack::PushL(operation);
        ((CRsfwWaitNoteStateMachine *) operation)->BaseConstructL();
        initialState =
            new (ELeave) CRsfwFetchAndCacheStateMachine::TFetchDataState(
                (CRsfwFetchAndCacheStateMachine *)operation);
        CleanupStack::Pop(operation);
        break;
        }

    case EFetchData:
        {
        DEBUGSTRING(("FETCHDATA"));
        operation = new (ELeave) CRsfwFetchDataStateMachine();
        CleanupStack::PushL(operation);
        ((CRsfwWaitNoteStateMachine *) operation)->BaseConstructL();
        initialState = new (ELeave) CRsfwFetchDataStateMachine::TFetchDataState(
            (CRsfwFetchDataStateMachine *)operation);
        CleanupStack::Pop(operation);
        break;
        }

    case ELookUp:
        {   
        DEBUGSTRING(("LOOKUP"));
        operation = new (ELeave) CRsfwLookupStateMachine();
        CleanupStack::PushL(operation);
        operation->BaseConstructL();
        initialState = new (ELeave)
            CRsfwLookupStateMachine::TUpdateKidAttributesTryFirstTypeState(
                (CRsfwLookupStateMachine *)operation);
        CleanupStack::Pop(operation);
        break;
        }

    case EGetAttr:
        {
        DEBUGSTRING(("GETATTR"));   
        operation = new (ELeave) CRsfwGetAttributesStateMachine();
        CleanupStack::PushL(operation);
        operation->BaseConstructL();
        initialState =
            new (ELeave) CRsfwGetAttributesStateMachine::TRefreshAttributesState(
                (CRsfwGetAttributesStateMachine *)operation);
        CleanupStack::Pop(operation);
        break;
        }

    case EClose:
        {
        DEBUGSTRING(("CLOSE"));   
        operation = new (ELeave) CRsfwCloseStateMachine();
        CleanupStack::PushL(operation);
        ((CRsfwWaitNoteStateMachine *) operation)->BaseConstructL();
        initialState =
            new (ELeave) CRsfwCloseStateMachine::TReleaseLockState(
                (CRsfwCloseStateMachine *) operation);
        CleanupStack::Pop(operation);
        break;  
        }
   case EFlush:
        {
        DEBUGSTRING(("FLUSH"));  
        operation = new (ELeave) CRsfwFlushStateMachine();
        CleanupStack::PushL(operation);
        ((CRsfwFlushStateMachine *) operation)->BaseConstructL();
        initialState =
            new (ELeave) CRsfwFlushStateMachine::TFlushDataToServerState(
                (CRsfwFlushStateMachine *) operation);
        CleanupStack::Pop(operation);        
        break;  
        }

    case EMkDir:
        {
        DEBUGSTRING(("MKDIR")); 
        operation = new (ELeave) CRsfwMkDirStateMachine();
        CleanupStack::PushL(operation);
        operation->BaseConstructL();
        initialState = new (ELeave) CRsfwMkDirStateMachine::TCheckIfExistsState(
            (CRsfwMkDirStateMachine *) operation);
        CleanupStack::Pop(operation);
        break;  
        }

    case ERemoveDir:
        {
        DEBUGSTRING(("RMDIR")); 
        operation = new (ELeave) CRsfwDeleteStateMachine(KNodeTypeDir);
        CleanupStack::PushL(operation);
        operation->BaseConstructL();
        initialState = new (ELeave) CRsfwDeleteStateMachine::TCheckIfCanBeDeleted(
            (CRsfwDeleteStateMachine *) operation);
        CleanupStack::Pop(operation);
        break;  
        }

    case ERemove:
        DEBUGSTRING(("REMOVE")); 
        operation = new (ELeave) CRsfwDeleteStateMachine(KNodeTypeFile);
        CleanupStack::PushL(operation);
        operation->BaseConstructL();
        initialState = new (ELeave) CRsfwDeleteStateMachine::TCheckIfCanBeDeleted(
            (CRsfwDeleteStateMachine *) operation);
        CleanupStack::Pop(operation);
        break;

    case ECreateFile:
        DEBUGSTRING(("CREATE"));
        operation = new (ELeave) CRsfwCreateFileStateMachine();
        CleanupStack::PushL(operation);
        operation->BaseConstructL();
        initialState =
            new (ELeave) CRsfwCreateFileStateMachine::TCheckIfExistsState(
                (CRsfwCreateFileStateMachine *) operation);
        CleanupStack::Pop(operation);
        break;

    case ERenameReplace:
        DEBUGSTRING(("RENAME"));
        operation = new (ELeave) CRsfwRenameFileStateMachine();
        CleanupStack::PushL(operation);
        operation->BaseConstructL();
        initialState = new (ELeave) CRsfwRenameFileStateMachine::TRenameFileState(
            (CRsfwRenameFileStateMachine *) operation);
        CleanupStack::Pop(operation);
        break;

    default:
        // request handler function not set,
        // would lead to accessing a NULL pointer
        User::Panic(KRfeServer, ENullRequestHandler);
        }

    SetImplementation(operation);
    Implementation()->SetNextState(initialState);
    
    // Set parameters
    iImplementation->SetVolumes(aRequest->iVolumeTable);
    
    if (aRequest->iVolume) 
        {
        iImplementation->SetFileEngine(aRequest->iVolume->iFileEngine);
        }
    
    iImplementation->SetArguments(aRequest->iInArgs, aRequest->iOutArgs);
       
    // Set backpointer to the request we are running,
    // so that the state machine can easily complete it
    iImplementation->SetRequest(aRequest);
    
    iIsSync = EFalse;
    CRsfwRfeOperation::Set(aOpCode);
    }
    
// ----------------------------------------------------------------------------
// CRsfwRfeAsyncOperation::VerifyMountConfigL
// Checks whether mount config data is correct
// ----------------------------------------------------------------------------
//     
void CRsfwRfeAsyncOperation::VerifyMountConfigL(TRsfwMountConfig& aMountConfig) 
    {    
    if (aMountConfig.iName.Length() > KMaxMountNameLength ||
        aMountConfig.iUri.Length() > KMaxMountUriLength ||
        aMountConfig.iUserName.Length() > KMaxMountUserNameLength ||
        aMountConfig.iPassword.Length() > KMaxMountPasswordLength ||
        aMountConfig.iAuxData.Length() > KMaxMountAuxDataLength)
        {
        DEBUGSTRING16(("CRsfwRfeAsyncOperation::VerifyMountConfigL bad aMountConfig argument"));        
        User::Leave(KErrArgument);
        }
    }

