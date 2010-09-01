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
* Description:  State machine for creating directories
*
*/


#include "rsfwmkdirstatemachine.h"
#include "rsfwfileentry.h"
#include "rsfwfiletable.h"
#include "rsfwinterface.h"
#include "rsfwfileengine.h"
#include "mdebug.h"
#include "rsfwdirentattr.h"
#include "rsfwvolumetable.h"

// Make dir

// ----------------------------------------------------------------------------
// CRsfwMkDirStateMachine::CRsfwMkDirStateMachine
// ----------------------------------------------------------------------------
//    
CRsfwMkDirStateMachine::CRsfwMkDirStateMachine()
    {
    }

// ----------------------------------------------------------------------------
// CRsfwMkDirStateMachine::~CRsfwMkDirStateMachine
// ----------------------------------------------------------------------------
//    
CRsfwMkDirStateMachine::~CRsfwMkDirStateMachine() 
    {
    delete iDirEntAttr;
    }

// ----------------------------------------------------------------------------
// CRsfwMkDirStateMachine::CompleteRequestL
// ----------------------------------------------------------------------------
//    
CRsfwRfeStateMachine::TState* CRsfwMkDirStateMachine::CompleteRequestL(TInt aError) 
    {
    if (iKidCreated && aError)
        {
        if ( aError == KErrNotFound )
            {
            User::Leave( KErrNotFound );
            }
        delete iKidFep;
        iKidFep = NULL;
        }

    // it may happen by chance that the new name is equal to iLastFailedLookup value
    FileEngine()->ResetFailedLookup();
     
    CompleteAndDestroyState()->SetErrorCode(aError);
    return CompleteAndDestroyState();
    }



// Check if exists


// ----------------------------------------------------------------------------
// CRsfwMkDirStateMachine::TCheckIfExistsState::TCheckIfExistsState
// ----------------------------------------------------------------------------
//    
CRsfwMkDirStateMachine::
TCheckIfExistsState::TCheckIfExistsState(CRsfwMkDirStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwMkDirStateMachine::TCheckIfExistsState::EnterL
// ----------------------------------------------------------------------------
//    
void CRsfwMkDirStateMachine::TCheckIfExistsState::EnterL() 
    {
    TRfeMkdirInArgs* inArgs =
        static_cast<TRfeMkdirInArgs*>(iOperation->iInArgs);
    TPtrC kidName(inArgs->iEntry.iName);

    iOperation->iKidFep = NULL;
    iOperation->iKidCreated = EFalse;

    // the parent to which we are making             
    if (!iOperation->Node())
        {
        User::Leave(KErrNotFound);
        }
        
    DEBUGSTRING16(("making directory '%S' in fid %d",
                   &kidName,
                   iOperation->Node()->Fid().iNodeId));    

    // Do we know about the kid yet?
    iOperation->iKidFep = iOperation->Node()->FindKidByName(kidName);
    if (!iOperation->iKidFep)
        {
        // This is either a completely new directory,
        // or a directory that we have not yet created a file entry for.
        // (should always happen)
        if (!iOperation->Volumes()->EnsureMetadataCanBeAddedL(iOperation->Node()))
            {
            User::Leave(KErrNoMemory);  
            }        
        iOperation->iKidFep = CRsfwFileEntry::NewL(kidName, iOperation->Node());
        iOperation->iKidCreated = ETrue;
        }
       
    iOperation->FileEngine()->GetAttributesL(*iOperation->iKidFep,
                                             iOperation->iDirEntAttr,
                                             KNodeTypeDir,
                                             iOperation);
    }

// ----------------------------------------------------------------------------
// CRsfwMkDirStateMachine::TCheckIfExistsState::CompleteL
// ----------------------------------------------------------------------------
//    
CRsfwMkDirStateMachine::TState*
CRsfwMkDirStateMachine::TCheckIfExistsState::CompleteL()
    {
    // GetAttributes returned KErrNone
    // directory exist and we return KErrAlreadyExitsts
    return iOperation->CompleteRequestL(KErrAlreadyExists); 
    }

// ----------------------------------------------------------------------------
// CRsfwMkDirStateMachine::TCheckIfExistsState::ErrorL
// ----------------------------------------------------------------------------
//    
CRsfwMkDirStateMachine::TState*
CRsfwMkDirStateMachine::TCheckIfExistsState::ErrorL(TInt /*aCode*/)
    {
    // GetAttributes returned error - 
    // directory does not exists, let's create it
  
    return new CRsfwMkDirStateMachine::TMakeDirectoryState(iOperation);
    }



// Make directory

// ----------------------------------------------------------------------------
// CRsfwMkDirStateMachine::TMakeDirectoryState::TMakeDirectoryState
// ----------------------------------------------------------------------------
//    
CRsfwMkDirStateMachine::
TMakeDirectoryState::TMakeDirectoryState(CRsfwMkDirStateMachine* aParent)
    : iOperation(aParent)
    {
    }

// ----------------------------------------------------------------------------
// CRsfwMkDirStateMachine::TMakeDirectoryState::EnterL
// ----------------------------------------------------------------------------
//    
void CRsfwMkDirStateMachine::TMakeDirectoryState::EnterL() 
    {
    // Make the directory
    if (!iOperation->FileEngine()->Disconnected())
        {
        HBufC* kidPath =
            iOperation->FileEngine()->FullNameLC(*iOperation->iKidFep);
        iOperation->FileEngine()->RemoteAccessL()->MakeDirectoryL(*kidPath,
                                                                  iOperation);
        CleanupStack::PopAndDestroy(kidPath);
        }
    }

// ----------------------------------------------------------------------------
// CRsfwMkDirStateMachine::TMakeDirectoryState::CompleteL
// ----------------------------------------------------------------------------
//    
CRsfwMkDirStateMachine::TState*
CRsfwMkDirStateMachine::TMakeDirectoryState::CompleteL()
    {
    iOperation->iKidFep->SetType(KNodeTypeDir);
    iOperation->FileEngine()->SetupAttributes(*iOperation->iKidFep);    
    
    // Attach the kid
    if (iOperation->iKidCreated)
        {
        iOperation->FileEngine()->iFileTable->AddL(iOperation->iKidFep);
        iOperation->Node()->AddKid(*iOperation->iKidFep);
        }

    // Create an empty container file locally
    iOperation->FileEngine()->CreateContainerFileL(*iOperation->iKidFep);
    iOperation->iKidFep->SetCached(ETrue);
    iOperation->iKidFep->SetAttribValidationTime();
   
    // Refresh parent directory next time it is accessed
    iOperation->Node()->SetLocallyDirty();

    return iOperation->CompleteRequestL(KErrNone);
    }

// ----------------------------------------------------------------------------
// CRsfwMkDirStateMachine::TMakeDirectoryState::ErrorL
// ----------------------------------------------------------------------------
//    
CRsfwMkDirStateMachine::TState*
CRsfwMkDirStateMachine::TMakeDirectoryState::ErrorL(TInt aCode)
    {
    DEBUGSTRING(("remote mkdir failed!"));
    return iOperation->CompleteRequestL(aCode);
    }

