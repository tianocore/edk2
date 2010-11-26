/*++

Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:  

  MultiThread.h
  
Abstract:

  Defines and function prototypes for the ProcessDsc utility.
  
--*/

#ifndef _MULTI_THREAD_H_
#define _MULTI_THREAD_H_

typedef struct _COMPONENTS_ITEM  COMPONENTS_ITEM;
typedef struct _BUILD_ITEM       BUILD_ITEM;
typedef struct _SOURCE_FILE_ITEM SOURCE_FILE_ITEM;
typedef struct _DEPENDENCY_ITEM  DEPENDENCY_ITEM;

//
// Use this structure to keep track of module build items
//
typedef struct _BUILD_ITEM {
  BUILD_ITEM        *Next;
  INT8              *BaseName;
  INT8              *Processor;
  INT8              *Makefile;
  UINT32            Index;
  UINT32            CompleteFlag;
  SOURCE_FILE_ITEM  *SourceFileList;
  DEPENDENCY_ITEM   *DependencyList;
} BUILD_ITEM;

//
// Use this structure to keep track of module source files
//
typedef struct _SOURCE_FILE_ITEM {
  SOURCE_FILE_ITEM  *Next;
  INT8              *FileName;
} SOURCE_FILE_ITEM;

//
// Use this structure to keep track of module build dependencies
//
typedef struct _DEPENDENCY_ITEM {
  DEPENDENCY_ITEM   *Next;
  BUILD_ITEM        *Dependency;
} DEPENDENCY_ITEM;

//
// Use this structure to keep track of [components] and [components.n] sections
//
typedef struct _COMPONENTS_ITEM {
  COMPONENTS_ITEM   *Next;
  BUILD_ITEM        *BuildList;
} COMPONENTS_ITEM;

//
// Function prototypes
//
BUILD_ITEM *
AddBuildItem (
  BUILD_ITEM  **BuildList,
  INT8        *BaseName,
  INT8        *Processor,
  INT8        *Makefile
  );


SOURCE_FILE_ITEM *
AddSourceFile (
  BUILD_ITEM  *BuildItem, 
  INT8        *FileName
  );

DEPENDENCY_ITEM *
AddDependency (
  BUILD_ITEM  *BuildList, 
  BUILD_ITEM  *BuildItem, 
  INT8        *BaseName,
  INT8        AdjustIndex
  );

void
FreeBuildList (
  BUILD_ITEM  *BuildList
  );

COMPONENTS_ITEM *
AddComponentsItem (
  COMPONENTS_ITEM  **ComponentsList
  );

void
FreeComponentsList (
  COMPONENTS_ITEM  *ComponentsList
  );
  
INT8
StartMultiThreadBuild (
  BUILD_ITEM  **BuildList,
  UINT32      ThreadNumber,
  INT8        *BuildDir
  );

#endif // ifndef _MULTI_THREAD_H_
