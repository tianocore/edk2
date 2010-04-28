/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:  

  FileSearch.c
  
Abstract:

  Module used to support file searches on the system.
  
--*/

#include <stdio.h>

#include "CommonUtils.h"
#include "FileSearch.h"
#include "UtilsMsgs.h"

//
// Internal file search flag for sanity checks
//
#define FILE_SEARCH_STARTED 0x8000
#define FILE_SEARCH_INITED  0x4000

static
BOOLEAN
FileSearchMeetsCriteria (
  FILE_SEARCH_DATA    *FSData
  );

/*****************************************************************************/
STATUS
FileSearchInit (
  FILE_SEARCH_DATA    *FSData
  )
{
  memset ((char *) FSData, 0, sizeof (FILE_SEARCH_DATA));
  FSData->Handle          = INVALID_HANDLE_VALUE;
  FSData->FileSearchFlags = FILE_SEARCH_INITED;
  FSData->FileName[0]     = 0;
  return STATUS_SUCCESS;
}

STATUS
FileSearchStart (
  FILE_SEARCH_DATA    *FSData,
  char                *FileMask,
  UINT32              SearchFlags
  )
{
  BOOLEAN Done;

  //
  // Save their flags, and set a flag to indicate that they called this
  // start function so we can perform extended checking in the other
  // routines we have in this module.
  //
  FSData->FileSearchFlags |= (SearchFlags | FILE_SEARCH_STARTED);
  FSData->FileName[0] = 0;

  //
  // Begin the search
  //
  FSData->Handle = FindFirstFile (FileMask, &(FSData->FindData));
  if (FSData->Handle == INVALID_HANDLE_VALUE) {
    return STATUS_ERROR;
  }
  //
  // Keep looping through until we find a file meeting the caller's
  // criteria per the search flags
  //
  Done = FALSE;
  while (!Done) {
    //
    // If we're done (we found a match) copy the file name found and return
    //
    Done = FileSearchMeetsCriteria (FSData);
    if (Done) {
      return STATUS_SUCCESS;
    }
    //
    // Go on to next file
    //
    if (!FindNextFile (FSData->Handle, &(FSData->FindData))) {
      return STATUS_NOT_FOUND;
    }
  }
  //
  // Not reached
  //
  return STATUS_NOT_FOUND;
}

//
// Find the next file meeting their criteria and return it.
//
STATUS
FileSearchFindNext (
  FILE_SEARCH_DATA    *FSData
  )
{
  BOOLEAN Done;

  Done = FALSE;
  while (!Done) {
    if (!FindNextFile (FSData->Handle, &(FSData->FindData))) {
      return STATUS_NOT_FOUND;
    }
    //
    // See if it matches their criteria
    //
    Done = FileSearchMeetsCriteria (FSData);
    if (Done) {
      return STATUS_SUCCESS;
    }
  }
  //
  // Not reached
  //
  return STATUS_NOT_FOUND;
}
//
// Perform any cleanup necessary to close down a search
//
STATUS
FileSearchDestroy (
  FILE_SEARCH_DATA    *FSData
  )
{
  if (FSData->Handle != INVALID_HANDLE_VALUE) {
    FindClose (FSData->Handle);
    FSData->Handle = INVALID_HANDLE_VALUE;
  }

  FSData->FileName[0]     = 0;
  FSData->FileSearchFlags = 0;
  return STATUS_SUCCESS;
}

static
BOOLEAN
FileSearchMeetsCriteria (
  FILE_SEARCH_DATA    *FSData
  )
{
  BOOLEAN     Status;
  STRING_LIST *StrList;
  UINT32      ExtLen;
  UINT32      FileNameLen;

  Status = FALSE;

  //
  // First clear the flag indicating this is neither a file or a
  // directory.
  //
  FSData->FileFlags &= ~(FILE_SEARCH_DIR | FILE_SEARCH_FILE);

  //
  // We found a file. See if it matches the user's search criteria. First
  // check for this being a directory, and they want directories, and
  // it's not "." and it's not ".."
  //
  if ((FSData->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
      (FSData->FileSearchFlags & FILE_SEARCH_DIR) &&
      (strcmp (FSData->FindData.cFileName, ".")) &&
      (strcmp (FSData->FindData.cFileName, ".."))
      ) {
    //
    // Assume we'll make it past this check
    //
    Status = TRUE;
    //
    // If they have a list of exclude directories, then check for those
    //
    StrList = FSData->ExcludeDirs;
    while (StrList != NULL) {
      if (_stricmp (FSData->FindData.cFileName, StrList->Str) == 0) {
        Status = FALSE;
        break;
      }

      StrList = StrList->Next;
    }
    //
    // If we didn't fail due to excluded directories, then set the dir flag
    //
    if (Status) {
      FSData->FileFlags |= FILE_SEARCH_DIR;
    }
    //
    // Else check for a file, and they want files....
    //
  } else if (((FSData->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) &&
           (FSData->FileSearchFlags & FILE_SEARCH_FILE)
          ) {
    //
    // See if it's in our list of excluded files
    //
    Status  = TRUE;
    StrList = FSData->ExcludeFiles;
    while (StrList != NULL) {
      if (_stricmp (FSData->FindData.cFileName, StrList->Str) == 0) {
        Status = FALSE;
        break;
      }

      StrList = StrList->Next;
    }

    if (Status) {
      //
      // See if it's in our list of excluded file extensions
      //
      FileNameLen = strlen (FSData->FindData.cFileName);
      StrList     = FSData->ExcludeExtensions;
      while (StrList != NULL) {
        ExtLen = strlen (StrList->Str);
        if (_stricmp (
              FSData->FindData.cFileName + FileNameLen - ExtLen,
              StrList->Str
              ) == 0) {
          Status = FALSE;
          break;
        }

        StrList = StrList->Next;
      }
    }

    if (Status) {
      FSData->FileFlags |= FILE_SEARCH_FILE;
    }
  }
  //
  // If it's a match, copy the filename into another field of the structure
  // for portability.
  //
  if (Status) {
    strcpy (FSData->FileName, FSData->FindData.cFileName);
  }

  return Status;
}
//
// Exclude a list of subdirectories.
//
STATUS
FileSearchExcludeDirs (
  FILE_SEARCH_DATA    *FSData,
  STRING_LIST         *StrList
  )
{
  FSData->ExcludeDirs = StrList;
  return STATUS_SUCCESS;
}

STATUS
FileSearchExcludeFiles (
  FILE_SEARCH_DATA    *FSData,
  STRING_LIST         *StrList
  )
{
  FSData->ExcludeFiles = StrList;
  return STATUS_SUCCESS;
}

STATUS
FileSearchExcludeExtensions (
  FILE_SEARCH_DATA    *FSData,
  STRING_LIST         *StrList
  )
{
  FSData->ExcludeExtensions = StrList;
  return STATUS_SUCCESS;
}
