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
* Description:  State machine for closing a file
*
*/

#include <apgcli.h>

#include "rsfwclosestatemachine.h"
#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwvolumetable.h"
#include "rsfwvolume.h"
#include "rsfwfileengine.h"
#include "rsfwrfeserver.h"
#include "rsfwwaitnotemanager.h"
#include "rsfwlockmanager.h"
#include "mdebug.h"


// CRsfwCloseStateMachine

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::CRsfwCloseStateMachine
// ----------------------------------------------------------------------------
//
CRsfwCloseStateMachine::CRsfwCloseStateMachine()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::~CRsfwCloseStateMachine
// ----------------------------------------------------------------------------
//
CRsfwCloseStateMachine::~CRsfwCloseStateMachine()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
//
CRsfwCloseStateMachine::TState* CRsfwCloseStateMachine::CompleteRequestL(TInt aError)
    {
    DEBUGSTRING(("CRsfwCloseStateMachine::CompleteRequestL::ErrorL %d", aError));

    // decrease count of files opened for writing
    Node()->iFileTable->UpdateOpenFileCount(-1);
 
    if (iFlags != ECloseLastFlushFailed) 
        {
        // file was closed successfully (not saved locally and deleted from filetable)
        DEBUGSTRING(("file was closed successfully (not saved locally and deleted from filetable)"));
        
        // If we just wrote the file to the server set attributes from the cache
        // file's attributes.
        if (Node()->CacheFileName() && Node()->IsOpenedForWriting())
                {
                Node()->SetCached(ETrue);
                FileEngine()->SetupAttributes(*Node());
                }
           
        // Add new cached and closed file to LRU cache managemet list.
        if ((Node()->Type() == KNodeTypeFile) &&
            (Node()->iCachedSize >0))
            {
            Volumes()->AddToLRUPriorityListL(Node(), Node()->CachePriority());
            }
    
        // If no content is in the cache then add closed file to the metadata LRU list.
        if ((Node()->Type() == KNodeTypeFile) &&
            (Node()->iCachedSize == 0))
            {
            Volumes()->AddToMetadataLRUPriorityListL(Node(), Node()->CachePriority());
            }  

        // uncommitted modifications have been resolved
        Node()->SetOpenedForWriting(EFalse);
        }
     else 
        {
        // remove the file from filetable as it was saved locally as a result of error
        // (either ECloseLastFlushFailed was set or we attempted to write the data and
        // there was error)
        Node()->iFileTable->RemoveL(Node());
        delete Node();
        }

    CompleteAndDestroyState()->SetErrorCode(aError);
    // remove the wait note
    DeleteWaitNoteL(ETrue);
    return CompleteAndDestroyState();
    }

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::ErrorOnStateEntry
// ----------------------------------------------------------------------------
// 
CRsfwCloseStateMachine::TState* CRsfwCloseStateMachine::ErrorOnStateEntry(TInt aError) 
    {
    DEBUGSTRING16(("CRsfwCloseStateMachine::ErrorOnStateEntry %d", aError));

    if (aError == KErrNotFound) 
        {
        // the node was not found, do not try to clos it
        return CRsfwRfeStateMachine::ErrorOnStateEntry(aError);
        }
    else 
        {    
        // don't show 'save as' note if transfer was cancelled explicitily by the user
        if (iFlags == ECloseLastFlushFailed && !Node()->IsCancelled()) 
            {
            // modified file, last flush failed so let user to save the file locally
            return new CRsfwCloseStateMachine::TSaveLocallyState(this);
            }
        else 
            {
            // in any case, we mark this file as closed
            CRsfwCloseStateMachine::TState* nextstate = NULL;
            TRAP_IGNORE(nextstate = CompleteRequestL(KErrNone));
            return nextstate;
            }
        }

    }

// Release lock

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::TReleaseLockState::TReleaseLockState
// ----------------------------------------------------------------------------
//
CRsfwCloseStateMachine::
TReleaseLockState::TReleaseLockState(CRsfwCloseStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::TReleaseLockState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwCloseStateMachine::TReleaseLockState::EnterL()
    {
    DEBUGSTRING(("CRsfwCloseStateMachine::TReleaseLockState::EnterL"));
    
    if (!iOperation->Node())
        {
        User::Leave(KErrNotFound);
        }

    DEBUGSTRING16(("closing fid %d (%S)", 
                        iOperation->Node()->Fid().iNodeId,
                        iOperation->Node()->Name()));
                    

    if (iOperation->Node()->Type() != KNodeTypeFile)
        {
        // Sanity
        DEBUGSTRING(("closing something else than a file!!!"));
        User::Leave(KErrArgument);
        }

    TRfeCloseInArgs* inArgs =
        static_cast<TRfeCloseInArgs*>(iOperation->iInArgs);
        
   DEBUGSTRING(("flags %d", inArgs->iFlags));  
   
   iOperation->iFlags = inArgs->iFlags;  
    
   if (iOperation->Node()->IsLocked())
        {
        // always attempt to unlock the file if it was locked
        iOperation->FileEngine()->LockManager()->ReleaseLockL(iOperation->Node(),
                                                          iOperation);
        }
    else
        {
        // no need to release the lock
        iOperation->HandleRemoteAccessResponse(0, KErrNone);
        } 
    
    }

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::TReleaseLockState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwCloseStateMachine::TState* CRsfwCloseStateMachine::TReleaseLockState::CompleteL()
    {
    DEBUGSTRING(("CRsfwCloseStateMachine::TReleaseLockState::CompleteL"));
    iOperation->Node()->RemoveLocked();
    
    // don't show 'save as' note if transfer was cancelled explicitily by the user
    if (iOperation->iFlags == ECloseLastFlushFailed && !iOperation->Node()->IsCancelled()) 
        {
        // modified file, last flush failed so let user to save the file locally
        return new CRsfwCloseStateMachine::TSaveLocallyState(iOperation);
        }
    else 
        {
        return iOperation->CompleteRequestL(KErrNone);
        }
   
    }

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::TReleaseLockState::ErrorL
// ----------------------------------------------------------------------------
//
CRsfwCloseStateMachine::TState*
CRsfwCloseStateMachine::TReleaseLockState::ErrorL(TInt /* aCode */)
    {
    DEBUGSTRING(("CRsfwCloseStateMachine::TReleaseLockState::ErrorL"));
    // Probably not really an error as  according to the RFC locks 
    // can disappear anytime anyway, lets just run the logic in CompleteL
    return CompleteL();
    }


// save as

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::TSaveLocallyState::TSaveLocallyState
// ----------------------------------------------------------------------------
//
CRsfwCloseStateMachine::
TSaveLocallyState::TSaveLocallyState(CRsfwCloseStateMachine* aParent)
   : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::TSaveLocallyState::EnterL
// ----------------------------------------------------------------------------
//
void CRsfwCloseStateMachine::TSaveLocallyState::EnterL()
    {
    DEBUGSTRING(("CRsfwCloseStateMachine::TSaveLocallyState::EnterL"));
    TEntry fEntry;
    CRsfwRfeServer::Env()->iFs.Entry((*(iOperation->Node()->CacheFileName())), fEntry);
    iFileSizeString.Num(fEntry.iSize);
    TPtrC cacheDriveLetter = iOperation->Node()->CacheFileName()->Left(1);
    
    iSaveToRequest.iMethod = TRsfwNotPluginRequest::ESaveToDlg;
    iSaveToRequest.iDriveName = iOperation->Node()->iFileTable->Volume()->MountInfo()
    									->iMountConfig.iName;
    									
    iSaveToRequest.iFileName = *(iOperation->Node()->Name());  
    iSaveToRequest.iCacheDrive = cacheDriveLetter;
    iSaveToRequest.iFileSize = iFileSizeString;
    
    				
    iOperation->Volumes()->WaitNoteManager()->SetSaveToDialogRequestL(iSaveToRequest);
    
    iOperation->Volumes()->WaitNoteManager()
            ->StartWaitNoteL(ERemoteSaveToLocal, iOperation);  
    }
   

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::TSaveLocallyState::CompleteL
// ----------------------------------------------------------------------------
//
CRsfwCloseStateMachine::TState* CRsfwCloseStateMachine::TSaveLocallyState::CompleteL()
    {  
    DEBUGSTRING(("CRsfwCloseStateMachine::TSaveLocallyState::CompleteL"));
    TInt err;
     
    // move the file from cache to the new location
    HBufC* newName = HBufC::NewMaxLC(KMaxPath);
    TPtr pathPtr = newName->Des();
    pathPtr = iSaveToRequest.iFileName;
    CFileMan* fman = CFileMan::NewL(CRsfwRfeServer::Env()->iFs);
    // we assume that this is local-to-local move, and can be synch. call
    err = fman->Move((*(iOperation->Node()->CacheFileName())), 
                   pathPtr, CFileMan::EOverWrite);
    delete fman;
    if (err == KErrNone) 
        {
        iOperation->Volumes()->WaitNoteManager()->ShowFileSavedToDialogL(pathPtr);
        }
    else 
        {
        iOperation->Volumes()->WaitNoteManager()->ShowFailedSaveNoteL();
        }
      
    CleanupStack::PopAndDestroy(newName);     
          
    return iOperation->CompleteRequestL(KErrNone);
    }

// ----------------------------------------------------------------------------
// CRsfwCloseStateMachine::TSaveLocallyState::ErrorL
// ----------------------------------------------------------------------------
//
CRsfwCloseStateMachine::TState*
CRsfwCloseStateMachine::TSaveLocallyState::ErrorL(TInt aCode)
    {
    DEBUGSTRING(("CRsfwCloseStateMachine::TSaveLocallyState::ErrorL %d", aCode));
    return iOperation->CompleteRequestL(KErrNone);
    }




