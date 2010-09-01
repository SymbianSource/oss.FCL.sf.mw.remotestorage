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
* Description:  functions for synchronous operations
*
*/


#ifndef RSFW_SYNCOPERATIONS_H
#define RSFW_SYNCOPERATIONS_H

class CRsfwRfeRequest;


/**
 *  wrapper for all sync requests
 *
 */
class TRFeSynCRsfwRfeRequest
    {
public:
    static void DoRequestL(CRsfwRfeRequest* aRequest);
    };
    
/**
 *  dismount a previously mounted volume
 *
 *	by volume ID	
 *
 */
class TRFeDismountVolumeId
    {
public:
    static void DoRequestL(CRsfwRfeRequest* aRequest);
    };
    
/**
 *  dismount a previously mounted volume
 *
 *	by drive letter
 *
 */ 
class TRFeDismountByDriveLetter
    {
public:
    static void DoRequestL(CRsfwRfeRequest* aRequest);
    };
    
 /**
 *  get a list of currently active mounts
 *
 */    
class TRFeGetMountList
    {
public:
    static void DoRequestL(CRsfwRfeRequest* aRequest);
    };
    
/**
 *  get information about a specific mount
 *
 */    
class TRFeGetMountInfo
    {
public:
    static void DoRequestL(CRsfwRfeRequest* aRequest);
    };
    
/**
 *  get permission to write certain amount of data
 *
 */      
class TRFeWriteData
    {
public:
    static void DoRequestL(CRsfwRfeRequest* aRequest);
    };
    
/**
 *  refresh a directory
 *
 */     
class TRFeDirectoryRefresh
    {
 public:
    static void DoRequestL(CRsfwRfeRequest* aRequest);   
    }; 
       
/**
 *  cancel all active upload/dowload operations
 *
 */         
class TRFeCancelAll
    {
 public:
    static void DoRequestL(CRsfwRfeRequest* aRequest);   
    };            
#endif