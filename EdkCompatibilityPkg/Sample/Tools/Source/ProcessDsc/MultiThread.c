/*++

Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MultiThread.c

Abstract:

  This module is used to add multi-thread build support to ProcessDsc utility 
  to improve the build performance. 

--*/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include "Common.h"
#include "MultiThread.h"

BUILD_ITEM *
AddBuildItem (
  BUILD_ITEM  **BuildList,
  INT8        *BaseName,
  INT8        *Processor,
  INT8        *Makefile
  )
/*++

Routine Description:
  
  Add a build item to a specified build list

Arguments:
  
  BuildList  - build list where the new build item will be added
  BaseName   - base name of the new module
  Processor  - processor type of the new module
  Makefile   - makefile name of the new module

Returns:

  Pointer to the newly added build item

--*/
{
  BUILD_ITEM  *NewBuildItem;
  
  //
  // Create a new build item
  //
  NewBuildItem = malloc (sizeof (BUILD_ITEM));
  if (NewBuildItem == NULL) {
    return NULL;
  }
  memset (NewBuildItem, 0, sizeof (BUILD_ITEM));
  NewBuildItem->BaseName  = _strdup (BaseName);
  NewBuildItem->Processor = _strdup (Processor);
  NewBuildItem->Makefile = _strdup (Makefile);
  
  //
  // Add the build item to the head of the build list
  //
  NewBuildItem->Next = *BuildList;
  *BuildList = NewBuildItem;
  
  return NewBuildItem;
}

SOURCE_FILE_ITEM *
AddSourceFile (
  BUILD_ITEM  *BuildItem, 
  INT8        *FileName
  )
/*++

Routine Description:
  
  Add a source file for a build item

Arguments:
  
  BuildItem - build item to add the source file
  FileName  - source file name to be added

Returns:

  Pointer to the newly added source file item

--*/
{
  SOURCE_FILE_ITEM *NewSourceFile;
  
  //
  // Create a new source file item
  //
  NewSourceFile = malloc (sizeof (SOURCE_FILE_ITEM));
  if (NewSourceFile == NULL) {
    return NULL;
  }
  memset (NewSourceFile, 0, sizeof (SOURCE_FILE_ITEM));
  NewSourceFile->FileName  = _strdup (FileName);
  
  //
  // Add the source file item to the head of the source file list
  //
  NewSourceFile->Next = BuildItem->SourceFileList;
  BuildItem->SourceFileList = NewSourceFile;
  
  return NewSourceFile; 
}

DEPENDENCY_ITEM *
AddDependency (
  BUILD_ITEM  *BuildList, 
  BUILD_ITEM  *BuildItem, 
  INT8        *BaseName,
  INT8        AdjustIndex
  )
/*++

Routine Description:
  
  Add a build dependency for a build item in the specified build list

Arguments:
  
  BuildList   - build list where to search the dependency
  BuildItem   - build item to add the dependency
  BaseName    - dependency module base name
  AdjustIndex - Adjust BuildItem->Index when non-zero

Returns:

  Pointer to the newly added build dependency

--*/
{
  BUILD_ITEM       *TempBuildItem;
  DEPENDENCY_ITEM  *NewDependency;
  
  //
  // Search the dependency in the build list
  //
  TempBuildItem = BuildList;
  while (TempBuildItem != NULL) {
    if ((_stricmp (TempBuildItem->BaseName, BaseName) == 0) &&
        (_stricmp (TempBuildItem->Processor, BuildItem->Processor) == 0) &&
        (TempBuildItem != BuildItem)) {
      break;
    }
    TempBuildItem = TempBuildItem->Next;
  }
  if (TempBuildItem == NULL) {
    return NULL;
  }
  
  //
  // This index is used to isolate two modules with same base name and processor.
  // (ProcessDsc allows duplicate base name libraries.)
  //
  if (AdjustIndex) {
    BuildItem->Index = TempBuildItem->Index + 1;
  }
  
  //
  // Create a new build dependency item
  //
  NewDependency = malloc (sizeof (DEPENDENCY_ITEM));
  if (NewDependency == NULL) {
    return NULL;
  }
  memset (NewDependency, 0, sizeof (DEPENDENCY_ITEM));
  NewDependency->Dependency  = TempBuildItem;
  
  //
  // Add the build dependency item to the head of the dependency list
  //
  NewDependency->Next = BuildItem->DependencyList;
  BuildItem->DependencyList = NewDependency;
  
  return NewDependency; 
}

void
FreeBuildList (
  BUILD_ITEM  *BuildList
  )
/*++

Routine Description:
  
  Free a build list

Arguments:
  
  BuildList  - build list to be freed

Returns:

--*/
{
  BUILD_ITEM       *TempBuildItem;
  BUILD_ITEM       *FreeBuildItem;
  SOURCE_FILE_ITEM *TempSourceFile;
  SOURCE_FILE_ITEM *FreeSourceFile;
  DEPENDENCY_ITEM  *TempDependency;
  DEPENDENCY_ITEM  *FreeDependency;
  
  TempBuildItem = BuildList;
  while (TempBuildItem != NULL) {
    free (TempBuildItem->BaseName);
    free (TempBuildItem->Processor);
    free (TempBuildItem->Makefile);

    //
    // Free source file list
    //
    TempSourceFile = TempBuildItem->SourceFileList;
    while (TempSourceFile != NULL) {
      FreeSourceFile = TempSourceFile;
      TempSourceFile = TempSourceFile->Next;
      free (FreeSourceFile);
    }

    //
    // Free dependency list
    //
    TempDependency = TempBuildItem->DependencyList;
    while (TempDependency != NULL) {
      FreeDependency = TempDependency;
      TempDependency = TempDependency->Next;
      free (FreeDependency);
    }

    FreeBuildItem = TempBuildItem;
    TempBuildItem = TempBuildItem->Next;
    free (FreeBuildItem);
  }
}

COMPONENTS_ITEM *
AddComponentsItem (
  COMPONENTS_ITEM  **ComponentsList
  )
/*++

Routine Description:
  
  Add a new components item to a specified components list

Arguments:
  
  ComponentsList  - components list where the new components item will be added

Returns:

  Pointer to the newly added components item

--*/
{
  COMPONENTS_ITEM  *NewComponents;
  COMPONENTS_ITEM  *TempComponents;
  
  //
  // Create a new components item
  //
  NewComponents = malloc (sizeof (COMPONENTS_ITEM));
  if (NewComponents == NULL) {
    return NULL;
  }
  memset (NewComponents, 0, sizeof (COMPONENTS_ITEM));
  
  //
  // Add the components item to the tail of the components list
  //
  TempComponents = *ComponentsList;
  if (TempComponents == NULL) {
    *ComponentsList = NewComponents;
  } else {
    while (TempComponents->Next != NULL) {
      TempComponents = TempComponents->Next;
    }
    TempComponents->Next = NewComponents;
  }
  
  return NewComponents;
}

void
FreeComponentsList (
  COMPONENTS_ITEM  *ComponentsList
  )
/*++

Routine Description:
  
  Free a components list

Arguments:
  
  ComponentsList  - components list to be freed

Returns:

--*/
{
  COMPONENTS_ITEM  *TempComponents;
  COMPONENTS_ITEM  *FreeComponents;
  
  TempComponents = ComponentsList;
  while (TempComponents != NULL) {
    FreeBuildList (TempComponents->BuildList);
    FreeComponents = TempComponents;
    TempComponents = TempComponents->Next;
    free (FreeComponents);
  }
}

//
// Module globals for multi-thread build
//
static INT8             mError;            // non-zero means error occurred
static INT8             mDone;             // non-zero means no more build items available for build
static UINT32           mThreadNumber;     // thread number
static INT8             *mBuildDir;        // build directory
static INT8             mLogDir[MAX_PATH]; // build item log dir
static CRITICAL_SECTION mCriticalSection;  // critical section object
static HANDLE           mSemaphoreHandle;  // semaphore for "ready for build" items in mWaitingList
static HANDLE           mEventHandle;      // event signaled when one build item is finished
static BUILD_ITEM       *mPendingList;     // build list for build items which are not ready for build
static BUILD_ITEM       *mWaitingList;     // build list for build items which are ready for build
static BUILD_ITEM       *mBuildingList;    // build list for build items which are buiding
static BUILD_ITEM       *mDoneList;        // build list for build items which already finish the build

//
// Restore the BuildList (not care about the sequence of the build items)
//
static void
RestoreBuildList (
  BUILD_ITEM  **BuildList
  )
{
  BUILD_ITEM  *TempBuildItem;
  
  if (mPendingList != NULL) {
    //
    // Add the mPendingList to the header of *BuildList
    //
    TempBuildItem = mPendingList;
    while (TempBuildItem->Next != NULL) {
      TempBuildItem = TempBuildItem->Next;
    }
    TempBuildItem->Next = *BuildList;
    *BuildList = mPendingList;
  }

  if (mWaitingList != NULL) {
    //
    // Add the mWaitingList to the header of *BuildList
    //
    TempBuildItem = mWaitingList;
    while (TempBuildItem->Next != NULL) {
      TempBuildItem = TempBuildItem->Next;
    }
    TempBuildItem->Next = *BuildList;
    *BuildList = mWaitingList;
  }

  if (mBuildingList != NULL) {
    //
    // Add the mBuildingList to the header of *BuildList
    //
    TempBuildItem = mBuildingList;
    while (TempBuildItem->Next != NULL) {
      TempBuildItem = TempBuildItem->Next;
    }
    TempBuildItem->Next = *BuildList;
    *BuildList = mBuildingList;
  }

  if (mDoneList != NULL) {
    //
    // Add the mDoneList to the header of *BuildList
    //
    TempBuildItem = mDoneList;
    while (TempBuildItem->Next != NULL) {
      TempBuildItem = TempBuildItem->Next;
    }
    TempBuildItem->Next = *BuildList;
    *BuildList = mDoneList;
  }
}

//
// Return non-zero when no source file build conflict
//
static INT8
CheckSourceFile (
  SOURCE_FILE_ITEM  *SourceFileList
  )
{
  BUILD_ITEM        *TempBuildItem;
  SOURCE_FILE_ITEM  *TempSourceFile;
  
  while (SourceFileList != NULL) {
    TempBuildItem = mBuildingList;
    while (TempBuildItem != NULL) {
      TempSourceFile = TempBuildItem->SourceFileList;
      while (TempSourceFile != NULL) {
        if (_stricmp (SourceFileList->FileName, TempSourceFile->FileName) == 0) {
          return 0;
        }
        TempSourceFile = TempSourceFile->Next;
      }
      TempBuildItem = TempBuildItem->Next;
    }
    SourceFileList = SourceFileList->Next;
  }
  
  return 1;
}

//
// Return non-zero when all the dependency build items has been built
//
static INT8
CheckDependency (
  DEPENDENCY_ITEM  *DependencyList
  )
{
  while (DependencyList != NULL) {
    if (!(DependencyList->Dependency->CompleteFlag)) {
      return 0;
    }
    DependencyList = DependencyList->Next;
  }
  
  return 1;
}

//
// Run the build task. The system() function call  will cause stdout conflict 
// in multi-thread envroment, so implement this through CreateProcess().
//
static INT8
RunBuildTask (
  INT8  *WorkingDir, 
  INT8  *LogFile, 
  INT8  *BuildCmd
  )
{
  HANDLE                FileHandle;
  SECURITY_ATTRIBUTES   SecAttr;
  PROCESS_INFORMATION   ProcInfo; 
  STARTUPINFO           StartInfo;
  BOOL                  FuncRetn;
  DWORD                 ExitCode;
  
  //
  // Init SecAttr
  //
  SecAttr.nLength              = sizeof (SECURITY_ATTRIBUTES); 
  SecAttr.bInheritHandle       = TRUE; 
  SecAttr.lpSecurityDescriptor = NULL;
  
  //
  // Create the log file
  //
  FileHandle = CreateFile (
                 LogFile,                // file to create
                 GENERIC_WRITE,          // open for writing
                 0,                      // do not share
                 &SecAttr,               // can be inherited by child processes
                 CREATE_ALWAYS,          // overwrite existing
                 FILE_ATTRIBUTE_NORMAL,  // normal file
                 NULL                    // no attr. template
                 );

  if (FileHandle == INVALID_HANDLE_VALUE) { 
      EnterCriticalSection (&mCriticalSection);
      Error (NULL, 0, 0, NULL, "could not open file %s", LogFile);
      LeaveCriticalSection (&mCriticalSection);
      return 1;
  }
  
  //
  // Init ProcInfo and StartInfo
  //
  ZeroMemory (&ProcInfo, sizeof (PROCESS_INFORMATION));
  ZeroMemory (&StartInfo, sizeof (STARTUPINFO));
  StartInfo.cb         = sizeof (STARTUPINFO); 
  StartInfo.hStdError  = FileHandle;
  StartInfo.hStdOutput = FileHandle;
  StartInfo.hStdInput  = GetStdHandle (STD_INPUT_HANDLE);
  StartInfo.dwFlags    = STARTF_USESTDHANDLES;

  //
  // Create the child process
  //
  FuncRetn = CreateProcess (
               NULL,          // no application name
               BuildCmd,      // command line 
               NULL,          // process security attributes 
               NULL,          // primary thread security attributes 
               TRUE,          // handles are inherited 
               0,             // creation flags 
               NULL,          // use parent's environment 
               WorkingDir,    // set current directory 
               &StartInfo,    // STARTUPINFO pointer 
               &ProcInfo      // receives PROCESS_INFORMATION 
               );
  
  if (FuncRetn == FALSE) {
    EnterCriticalSection (&mCriticalSection);
    Error (NULL, 0, 0, NULL, "could not create child process");
    LeaveCriticalSection (&mCriticalSection);
    CloseHandle (FileHandle);
    return 1;
  } 
  
  //
  // Wait until child process exits
  //
  WaitForSingleObject (ProcInfo.hProcess, INFINITE);
  GetExitCodeProcess (ProcInfo.hProcess, &ExitCode);
  CloseHandle (ProcInfo.hProcess);
  CloseHandle (ProcInfo.hThread);
  CloseHandle (FileHandle);
  
  if (ExitCode != 0) {
    return 1;
  } else {
    return 0;
  }
}

//
// Thread function
//
static DWORD WINAPI
ThreadProc (
  LPVOID lpParam
  )
{
  UINT32      ThreadId;
  BUILD_ITEM  *PreviousBuildItem;
  BUILD_ITEM  *CurrentBuildItem;
  BUILD_ITEM  *NextBuildItem;
  INT8        WorkingDir[MAX_PATH];  
  INT8        LogFile[MAX_PATH];
  INT8        BuildCmd[MAX_PATH];
  
  ThreadId = (UINT32)lpParam;
  //
  // Loop until error occurred or no more build items available for build
  //
  for (;;) {
    WaitForSingleObject (mSemaphoreHandle, INFINITE);
    if (mError || mDone) {
      return 0;
    }
    
    //
    // When code runs here, there must have one build item available for this 
    // thread. Loop until error occurred or get one build item for build.
    //
    for (;;) {
      EnterCriticalSection (&mCriticalSection);
      PreviousBuildItem = NULL;
      CurrentBuildItem  = mWaitingList;
      while (CurrentBuildItem != NULL) {
        NextBuildItem = CurrentBuildItem->Next;
        //
        // CheckSourceFile() is to avoid concurrently build the same source file
        // which may cause the muti-thread build failure
        //
        if (CheckSourceFile (CurrentBuildItem->SourceFileList)) {
          //
          // Move the current build item from mWaitingList
          //
          if (PreviousBuildItem != NULL) {
            PreviousBuildItem->Next = NextBuildItem;
          } else {
            mWaitingList = NextBuildItem;
          }
          //
          // Add the current build item to the head of mBuildingList
          //
          CurrentBuildItem->Next = mBuildingList;
          mBuildingList = CurrentBuildItem;
          //
          // If no more build items is pending or waiting for build,
          // wake up every child thread for exit.
          //
          if ((mPendingList == NULL) && (mWaitingList == NULL)) {
            mDone = 1;
            //
            // Make sure to wake up every child thread for exit
            //        
            ReleaseSemaphore (mSemaphoreHandle, mThreadNumber, NULL);
          }
          break;
        }
        PreviousBuildItem = CurrentBuildItem;
        CurrentBuildItem  = NextBuildItem;
      }
      if (CurrentBuildItem != NULL) {
        //
        // Display build item info
        //
        printf ("\t[Thread_%d] nmake -nologo -f %s all\n", ThreadId, CurrentBuildItem->Makefile);
        //
        // Prepare build task
        //
        sprintf (WorkingDir, "%s\\%s", mBuildDir, CurrentBuildItem->Processor);
        sprintf (LogFile, "%s\\%s_%s_%d.txt", mLogDir, CurrentBuildItem->BaseName, 
                 CurrentBuildItem->Processor, CurrentBuildItem->Index);
        sprintf (BuildCmd, "nmake -nologo -f %s all", CurrentBuildItem->Makefile);
        LeaveCriticalSection (&mCriticalSection);
        break;
      } else {
        LeaveCriticalSection (&mCriticalSection);
        //
        // All the build items in mWaitingList have source file conflict with 
        // mBuildingList. This rarely hapeens. Need wait for the build items in
        // mBuildingList to be finished by other child threads.
        //
        Sleep (1000);
        if (mError) {
          return 0;
        }
      }
    }
    
    //
    // Start to build the CurrentBuildItem
    //
    if (RunBuildTask (WorkingDir, LogFile, BuildCmd)) {
      //
      // Build failure
      //
      mError = 1;
      //
      // Make sure to wake up every child thread for exit
      //
      ReleaseSemaphore (mSemaphoreHandle, mThreadNumber, NULL);
      SetEvent(mEventHandle);

      return mError;
    } else {
      //
      // Build success
      //
      CurrentBuildItem->CompleteFlag = 1;
      
      EnterCriticalSection (&mCriticalSection);
      //
      // Move this build item from mBuildingList
      //
      if (mBuildingList == CurrentBuildItem) {
        mBuildingList = mBuildingList->Next;
      } else {
        NextBuildItem = mBuildingList;
        while (NextBuildItem->Next != CurrentBuildItem) {
          NextBuildItem = NextBuildItem->Next;
        }
        NextBuildItem->Next = CurrentBuildItem->Next;
      }
      //
      // Add this build item to mDoneList
      //
      CurrentBuildItem->Next = mDoneList;
      mDoneList = CurrentBuildItem;
      LeaveCriticalSection (&mCriticalSection);
      
      SetEvent(mEventHandle);
    }
  }
}

INT8
StartMultiThreadBuild (
  BUILD_ITEM  **BuildList,
  UINT32      ThreadNumber,
  INT8        *BuildDir
  )
/*++

Routine Description:
  
  Start multi-thread build for a specified build list

Arguments:
  
  BuildList     - build list for multi-thread build
  ThreadNumber  - thread number for multi-thread build
  BuildDir      - build dir

Returns:

  0             - Successfully finished the multi-thread build
  other value   - Build failure

--*/
{
  UINT32        Index;
  UINT32        Count;
  BUILD_ITEM    *PreviousBuildItem;
  BUILD_ITEM    *CurrentBuildItem;
  BUILD_ITEM    *NextBuildItem;
  HANDLE        *ThreadHandle;
  INT8          Cmd[MAX_PATH];
  
  mError        = 0;
  mDone         = 0;
  mThreadNumber = ThreadNumber;
  mBuildDir     = BuildDir;
  mPendingList  = *BuildList;
  *BuildList    = NULL;
  mWaitingList  = NULL;
  mBuildingList = NULL;
  mDoneList     = NULL;
  
  //
  // Do nothing when mPendingList is empty
  //
  if (mPendingList == NULL) {
    return 0;
  }
  
  //
  // Get build item count of mPendingList
  //
  Count = 0;
  CurrentBuildItem = mPendingList;
  while (CurrentBuildItem != NULL) {
    Count++;
    CurrentBuildItem = CurrentBuildItem->Next;
  }
  
  //
  // The semaphore is also used to wake up child threads for exit,
  // so need to make sure "maximum count" >= "thread number".
  //
  if (Count < ThreadNumber) {
    Count = ThreadNumber;
  }
  
  //
  // Init mSemaphoreHandle
  //
  mSemaphoreHandle = CreateSemaphore (
                       NULL,       // default security attributes
                       0,          // initial count
                       Count,      // maximum count
                       NULL        // unnamed semaphore
                       );
  if (mSemaphoreHandle == NULL) {
    Error (NULL, 0, 0, NULL, "failed to create semaphore");
    RestoreBuildList (BuildList);
    return 1;
  }  

  //
  // Init mEventHandle
  //
  mEventHandle = CreateEvent( 
                   NULL,     // default security attributes
                   FALSE,    // auto-reset event
                   TRUE,     // initial state is signaled
                   NULL      // object not named
                   ); 
  if (mEventHandle == NULL) { 
    Error (NULL, 0, 0, NULL, "failed to create event");
    CloseHandle (mSemaphoreHandle);
    RestoreBuildList (BuildList);
    return 1;
  }
  
  //
  // Init mCriticalSection
  //
  InitializeCriticalSection (&mCriticalSection);
  
  //
  // Create build item log dir
  //
  sprintf (mLogDir, "%s\\Log", mBuildDir);
  _mkdir (mLogDir);
  
  //
  // Create child threads for muti-thread build
  //
  ThreadHandle = malloc (ThreadNumber * sizeof (HANDLE));
  if (ThreadHandle == NULL) {
    Error (NULL, 0, 0, NULL, "failed to allocate memory");
    CloseHandle (mSemaphoreHandle);
    CloseHandle (mEventHandle);
    RestoreBuildList (BuildList);
    return 1;
  }
  for (Index = 0; Index < ThreadNumber; Index++) {
    ThreadHandle[Index] = CreateThread (
                            NULL,           // default security attributes
                            0,              // use default stack size
                            ThreadProc,     // thread function
                            (LPVOID)Index,  // argument to thread function: use Index as thread id
                            0,              // use default creation flags
                            NULL            // thread identifier not needed
                            );
    if (ThreadHandle[Index] == NULL) {
      Error (NULL, 0, 0, NULL, "failed to create Thread_%d", Index);
      mError       = 1;
      ThreadNumber = Index;
      //
      // Make sure to wake up every child thread for exit
      //
      ReleaseSemaphore (mSemaphoreHandle, ThreadNumber, NULL);
      break;
    }
  }
  
  //
  // Loop until error occurred or no more build items pending for build
  //
  for (;;) {
    WaitForSingleObject (mEventHandle, INFINITE);
    if (mError) {
      break;
    }
    Count = 0;
    
    EnterCriticalSection (&mCriticalSection);
    PreviousBuildItem = NULL;
    CurrentBuildItem  = mPendingList;
    while (CurrentBuildItem != NULL) {
      NextBuildItem = CurrentBuildItem->Next;
      if (CheckDependency (CurrentBuildItem->DependencyList)) {
        //
        // Move the current build item from mPendingList
        //
        if (PreviousBuildItem != NULL) {
          PreviousBuildItem->Next = NextBuildItem;
        } else {
          mPendingList = NextBuildItem;
        }
        //
        // Add the current build item to the head of mWaitingList
        //
        CurrentBuildItem->Next = mWaitingList;
        mWaitingList = CurrentBuildItem;
        Count++;
      } else {
        PreviousBuildItem = CurrentBuildItem;
      }
      CurrentBuildItem  = NextBuildItem;
    }
    LeaveCriticalSection (&mCriticalSection);
    
    ReleaseSemaphore (mSemaphoreHandle, Count, NULL);
    if (mPendingList == NULL) {
      break;
    }
  }

  //
  // Wait until all threads have terminated
  //
  WaitForMultipleObjects (ThreadNumber, ThreadHandle, TRUE, INFINITE);
  
  if (mError && (mBuildingList != NULL)) {
    //
    // Dump build failure log of the first build item which doesn't finish the build
    //
    printf ("\tnmake -nologo -f %s all\n", mBuildingList->Makefile);
    sprintf (Cmd, "type %s\\%s_%s_%d.txt 2>NUL", mLogDir, mBuildingList->BaseName,
             mBuildingList->Processor, mBuildingList->Index);
    _flushall ();
    if (system (Cmd)) {
      Error (NULL, 0, 0, NULL, "failed to run \"%s\"", Cmd);
    }
  }

  DeleteCriticalSection (&mCriticalSection);
  for (Index = 0; Index < ThreadNumber; Index++) {
    CloseHandle (ThreadHandle[Index]);
  }
  free (ThreadHandle);
  CloseHandle (mSemaphoreHandle);
  CloseHandle (mEventHandle);
  RestoreBuildList (BuildList);

  return mError;
}
